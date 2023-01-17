#pragma once

#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <daxa/daxa.inl>

struct Vertex {
    f32vec3 pos;
    f32vec3 col;
};
DAXA_ENABLE_BUFFER_PTR(Vertex)

struct GpuInput {
    u32vec3 size;
};
DAXA_ENABLE_BUFFER_PTR(GpuInput)

struct Voxel {
    f32vec3 col;
};
DAXA_ENABLE_BUFFER_PTR(Voxel)

struct RasterPush {
    daxa_BufferPtr(GpuInput) gpu_input;
    daxa_BufferPtr(Vertex) vertex_buffer;
    daxa_RWBufferPtr(Voxel) voxel_buffer;
    u32 out_z;
};

#define VERTS(i) deref(daxa_push_constant.vertex_buffer[i])
#define INPUT deref(daxa_push_constant.gpu_input)
#define VOXELS(i) deref(daxa_push_constant.voxel_buffer[i])
