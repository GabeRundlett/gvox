#pragma once

#define DAXA_ENABLE_IMAGE_OVERLOADS_BASIC 1
#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <daxa/daxa.inl>

struct Vertex {
    f32vec3 pos;
    f32vec2 tex;
    u32 rotation;
};
DAXA_ENABLE_BUFFER_PTR(Vertex)

struct GpuInput {
    u32vec3 size;
};
DAXA_ENABLE_BUFFER_PTR(GpuInput)

struct RasterPush {
    daxa_BufferPtr(GpuInput) gpu_input;
    daxa_BufferPtr(Vertex) vertex_buffer;
    daxa_RWBufferPtr(daxa_u32) voxel_buffer;
    daxa_Image2Df32 texture_id;
    daxa_SamplerId texture_sampler;
};

struct PreprocessPush {
    f32mat4x4 modl_mat;
    daxa_BufferPtr(GpuInput) gpu_input;
    daxa_BufferPtr(Vertex) vertex_buffer;
};

#define VERTS(i) deref(daxa_push_constant.vertex_buffer[i])
#define INPUT deref(daxa_push_constant.gpu_input)
#define VOXELS(i) deref(daxa_push_constant.voxel_buffer[i])
