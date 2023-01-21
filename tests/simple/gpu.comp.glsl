#include "gpu.inl"

DAXA_USE_PUSH_CONSTANT(GpuCompPush)

#define WARP_SIZE 32
#if PALETTE_REGION_TOTAL_SIZE > (WARP_SIZE * WARP_SIZE)
#error "why tho?"
#endif

#define SUBGROUP_N (PALETTE_REGION_TOTAL_SIZE / WARP_SIZE)

shared u32 subgroup_mins[SUBGROUP_N];
shared u32 palette_result[PALETTE_REGION_TOTAL_SIZE];
shared u32 palette_barrier;
shared u32 palette_size;
shared u32 output_offset;

u32 ceil_log2(u32 x) {
    u32 t[6] = u32[6](
        0x00000000,
        0xFFFF0000,
        0x0000FF00,
        0x000000F0,
        0x0000000C,
        0x00000002);

    u32 y = (((x & (x - 1)) == 0) ? 0 : 1);
    i32 j = 32;

    for (u32 i = 0; i < 6; i++) {
        i32 k = (((x & t[i]) == 0) ? 0 : j);
        y += u32(k);
        x >>= k;
        j >>= 1;
    }

    return y;
}

layout(local_size_x = PALETTE_REGION_AXIS_SIZE, local_size_y = PALETTE_REGION_AXIS_SIZE, local_size_z = PALETTE_REGION_AXIS_SIZE) in;
void main() {
    u32 voxel_index =
        gl_GlobalInvocationID.x +
        gl_GlobalInvocationID.y * CHUNK_AXIS_SIZE +
        gl_GlobalInvocationID.z * CHUNK_AXIS_SIZE * CHUNK_AXIS_SIZE;
    u32 palette_region_voxel_index =
        gl_LocalInvocationID.x +
        gl_LocalInvocationID.y * PALETTE_REGION_AXIS_SIZE +
        gl_LocalInvocationID.z * PALETTE_REGION_AXIS_SIZE * PALETTE_REGION_AXIS_SIZE;
    u32 palette_region_index =
        gl_WorkGroupID.x +
        gl_WorkGroupID.y * PALETTE_REGION_AXIS_N +
        gl_WorkGroupID.z * PALETTE_REGION_AXIS_N * PALETTE_REGION_AXIS_N;

    u32 my_voxel = INPUT.data[voxel_index];
    u32 my_palette_index = 0;

    if (palette_region_voxel_index == 0) {
        if (palette_region_index == 0) {
            OUTPUT.node_n = 1;
            OUTPUT.node_header.node_full_size = 0;
            OUTPUT.node_header.region_count_x = PALETTE_REGION_AXIS_N;
            OUTPUT.node_header.region_count_y = PALETTE_REGION_AXIS_N;
            OUTPUT.node_header.region_count_z = PALETTE_REGION_AXIS_N;
            atomicExchange(COMPRESS_STATE.current_size, 0);
        }

        palette_size = 0;
        palette_barrier = 0;
    }

    for (u32 algo_i = 0; algo_i < PALETTE_REGION_TOTAL_SIZE; ++algo_i) {
        barrier();
        memoryBarrier();
        memoryBarrierShared();
        groupMemoryBarrier();

        // make the voxel value of my_voxel to be "considered" only
        // if my_palette_index == 0
        u32 least_in_my_subgroup = subgroupMin(my_voxel | (0xffffffff * u32(my_palette_index != 0)));
        subgroupBarrier();
        if (gl_SubgroupInvocationID == 0) {
            subgroup_mins[gl_SubgroupID] = least_in_my_subgroup;
        }

        barrier();
        memoryBarrier();
        memoryBarrierShared();
        groupMemoryBarrier();

        /// Do an atomic min between subgroups
        const u32 LOG2_SUBGROUP_N = 4;
        for (u32 i = 0; i < LOG2_SUBGROUP_N; ++i) {
            if (palette_region_voxel_index < SUBGROUP_N / (2 << i)) {
                atomicMin(
                    subgroup_mins[(palette_region_voxel_index * 2) * (1 << i)],
                    subgroup_mins[(palette_region_voxel_index * 2 + 1) * (1 << i)]);
            }
        }

        barrier();
        memoryBarrier();
        memoryBarrierShared();
        groupMemoryBarrier();

        u32 absolute_min = subgroup_mins[0];

        // In the case that all voxels have been added to the palette,
        // this will be true for all threads. In such a case, our
        // palette_index will be non-zero, and we should break.
        if (absolute_min == my_voxel) {
            if (my_palette_index != 0) {
                // As I just said, we should break in this case. This
                // means the full palette has already been constructed!
                break;
            }
            if (subgroupElect()) {
                u32 already_accounted_for = atomicExchange(palette_barrier, 1);
                if (already_accounted_for == 0) {
                    // We're the thread to write to the palette
                    palette_result[palette_size] = my_voxel;
                    palette_size++;
                }
            }
        }

        barrier();
        memoryBarrier();
        memoryBarrierShared();
        groupMemoryBarrier();

        if (palette_region_voxel_index == 0) {
            palette_barrier = 0;
        }
        if (absolute_min == my_voxel) {
            // this should be `palette_size - 1`, but we'll do that later
            // so that we can test whether we have already written out
            my_palette_index = palette_size;
        }
    }

    barrier();
    memoryBarrier();
    memoryBarrierShared();
    groupMemoryBarrier();

    if (palette_size > 367) {
        u32 compressed_size = PALETTE_REGION_TOTAL_SIZE;
        if (palette_region_voxel_index == 0) {
            u32 var = atomicAdd(COMPRESS_STATE.current_size, compressed_size);
            atomicExchange(output_offset, var);
            atomicExchange(OUTPUT.node_header.node_full_size, (var + compressed_size) * 4);
            OUTPUT.region_headers[palette_region_index].variant_n = palette_size;
            OUTPUT.region_headers[palette_region_index].blob_offset = var * 4;
        }

        barrier();
        memoryBarrier();
        memoryBarrierShared();
        groupMemoryBarrier();

        OUTPUT.data[COMPRESS_STATE.current_size - compressed_size + palette_region_voxel_index] = my_voxel;
    } else {
        u32 v_data_offset = palette_size;
        u32 bits_per_variant = ceil_log2(palette_size);
        if (palette_region_voxel_index == 0) {
            // in sizeof u32
            if (palette_size > 1) {
                // round up to nearest byte
                u32 compressed_size = (bits_per_variant * PALETTE_REGION_TOTAL_SIZE + 7) / 8;
                // round up to the nearest uint32_t, and add an extra
                compressed_size = (compressed_size + 3) / 4 + 1;
                // add the size of the palette data
                compressed_size += v_data_offset;
                u32 var = atomicAdd(COMPRESS_STATE.current_size, compressed_size);
                atomicExchange(output_offset, var);
                atomicExchange(OUTPUT.node_header.node_full_size, (var + compressed_size) * 4);
                OUTPUT.region_headers[palette_region_index].variant_n = palette_size;
                OUTPUT.region_headers[palette_region_index].blob_offset = var * 4;
                for (u32 i = 0; i < palette_size; ++i) {
                    OUTPUT.data[var + i] = palette_result[i];
                }
            } else {
                OUTPUT.region_headers[palette_region_index].variant_n = 1;
                OUTPUT.region_headers[palette_region_index].blob_offset = my_voxel;
            }
        }

        barrier();
        memoryBarrier();
        memoryBarrierShared();
        groupMemoryBarrier();

        if (palette_size > 1) {
            v_data_offset += output_offset;
            u32 mask = (~0u) >> (32 - bits_per_variant);
            u32 bit_index = palette_region_voxel_index * bits_per_variant;
            u32 data_index = bit_index / 32;
            u32 data_offset = bit_index - data_index * 32;
            u32 data = (my_palette_index - 1) & mask;
            // clang-format off
            atomicAnd(OUTPUT.data[v_data_offset + data_index + 0], ~(mask << data_offset));
            atomicOr (OUTPUT.data[v_data_offset + data_index + 0],   data << data_offset);
            if (data_offset + bits_per_variant > 32) {
                u32 shift = bits_per_variant - ((data_offset + bits_per_variant) & 0x1f);
                atomicAnd(OUTPUT.data[v_data_offset + data_index + 1], ~(mask >> shift));
                atomicOr (OUTPUT.data[v_data_offset + data_index + 1],   data >> shift);
            }
            // clang-format on
        }
    }
}
