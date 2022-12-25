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

layout(local_size_x = PALETTE_CHUNK_AXIS_SIZE, local_size_y = PALETTE_CHUNK_AXIS_SIZE, local_size_z = PALETTE_CHUNK_AXIS_SIZE) in;
void main() {
    u32 voxel_i = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * PALETTE_CHUNK_AXIS_SIZE + gl_GlobalInvocationID.z * PALETTE_CHUNK_AXIS_SIZE * PALETTE_CHUNK_AXIS_SIZE;
    u32 my_voxel = INPUT.data[voxel_i];

    groupMemoryBarrier();

    u32 least_in_my_subgroup = subgroupMin(my_voxel);
    subgroupBarrier();
    if (gl_SubgroupInvocationID == 0) {
        subgroup_mins[gl_SubgroupID] = least_in_my_subgroup;
    }

    /// Do an atomic min between subgroups

    groupMemoryBarrier();

    if (voxel_i == 0)
        OUTPUT.palette_size = 54;
}
