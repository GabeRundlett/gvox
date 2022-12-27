#include "gpu.inl"

DAXA_USE_PUSH_CONSTANT(GpuCompPush)

#define INPUT deref(daxa_push_constant.gpu_input)
#define OUTPUT deref(daxa_push_constant.gpu_output)

#define WARP_SIZE 32
#define PALETTE_CHUNK_AXIS_SIZE 8
#define PALETTE_CHUNK_TOTAL_SIZE (PALETTE_CHUNK_AXIS_SIZE * PALETTE_CHUNK_AXIS_SIZE * PALETTE_CHUNK_AXIS_SIZE)

#if PALETTE_CHUNK_TOTAL_SIZE > (WARP_SIZE * WARP_SIZE)
#error "why tho?"
#endif

#define SUBGROUP_N (PALETTE_CHUNK_TOTAL_SIZE / WARP_SIZE)

shared u32 subgroup_mins[SUBGROUP_N];
shared u32 palette_result[PALETTE_CHUNK_TOTAL_SIZE];
shared u32 palette_barrier;
shared u32 palette_size;

u32 ceil_log2(u32 x) {
    u32 t[5] = u32[5](
        0x00000000,
        0xFFFF0000,
        0x0000FF00,
        0x000000F0,
        0x0000000C);

    u32 y = (((x & (x - 1)) == 0) ? 0 : 1);
    i32 j = 32;

    for (u32 i = 0; i < 5; i++) {
        i32 k = (((x & t[i]) == 0) ? 0 : j);
        y += u32(k);
        x >>= k;
        j >>= 1;
    }

    return y;
}

layout(local_size_x = PALETTE_CHUNK_AXIS_SIZE, local_size_y = PALETTE_CHUNK_AXIS_SIZE, local_size_z = PALETTE_CHUNK_AXIS_SIZE) in;
void main() {
    u32 voxel_i =
        gl_GlobalInvocationID.x +
        gl_GlobalInvocationID.y * PALETTE_CHUNK_AXIS_SIZE +
        gl_GlobalInvocationID.z * PALETTE_CHUNK_AXIS_SIZE * PALETTE_CHUNK_AXIS_SIZE;
    u32 my_voxel = INPUT.data[voxel_i];
    u32 my_palette_index = 0;

    if (voxel_i == 0) {
        palette_size = 0;
        palette_barrier = 0;
    }

    for (u32 algo_i = 0; algo_i < PALETTE_CHUNK_TOTAL_SIZE; ++algo_i) {
        groupMemoryBarrier();

        // make the voxel value of my_voxel to be "considered" only
        // if my_palette_index == 0
        u32 least_in_my_subgroup = subgroupMin(my_voxel | (0xffffffff * u32(my_palette_index != 0)));
        subgroupBarrier();
        if (gl_SubgroupInvocationID == 0) {
            subgroup_mins[gl_SubgroupID] = least_in_my_subgroup;
        }

        groupMemoryBarrier();

        /// Do an atomic min between subgroups
        const u32 LOG2_SUBGROUP_N = 4;
        for (u32 i = 0; i < LOG2_SUBGROUP_N; ++i) {
            if (voxel_i < SUBGROUP_N / (2 << i)) {
                atomicMin(
                    subgroup_mins[(voxel_i * 2) * (1 << i)],
                    subgroup_mins[(voxel_i * 2 + 1) * (1 << i)]);
            }
        }

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

        groupMemoryBarrier();
        if (voxel_i == 0) {
            palette_barrier = 0;
        }
        if (absolute_min == my_voxel) {
            // this should be `palette_size - 1`, but we'll do that later
            // so that we can test whether we have already written out
            my_palette_index = palette_size;
        }
    }

    groupMemoryBarrier();

    u32 bits_per_variant = ceil_log2(palette_size);

    if (my_palette_index == 0) {
        // This should never happen... Maybe look into
        // finding a way to crash the GPU if this gets hit.
        // This also means that the for-loop reached `PALETTE_CHUNK_TOTAL_SIZE`
        // iterations, which would probably be noticable
        // based on the performance of compression.
        OUTPUT.data[voxel_i] = 0;
    } else if (palette_size > 1) {
        u32 result = palette_result[my_palette_index - 1];
        u32 mask = (~0) >> (33 - bits_per_variant);
        u32 bit_index = voxel_i * bits_per_variant;
        u32 byte_index = bit_index / 8;
        u32 bit_offset = bit_index - byte_index * 8;
        u32 data_index = (byte_index / 4);
        i32 data_offset = i32(byte_index - data_index * 4) * 8;
        u32 data = (my_palette_index & mask) << bit_offset;
        // clang-format off
        atomicAnd(OUTPUT.data[data_index + 0], (~mask) << data_offset);
        atomicAnd(OUTPUT.data[data_index + 1], (~mask) << (data_offset - 32));
        atomicOr (OUTPUT.data[data_index + 0],   data  << data_offset);
        atomicOr (OUTPUT.data[data_index + 1],   data  << (data_offset - 32));
        // clang-format on
        result = (my_palette_index - 1) << 0x18;
    }

    if (voxel_i == 0) {
        // round up to nearest byte
        u32 compressed_size = (bits_per_variant * PALETTE_CHUNK_TOTAL_SIZE + 7) / 8;
        // round up to the nearest uint32_t, and add an extra
        compressed_size = ((compressed_size + 3) / 4 + 1) * 4;
        // add the size of the palette
        compressed_size += palette_size * 4;

        OUTPUT.palette_size = compressed_size;
    }
}
