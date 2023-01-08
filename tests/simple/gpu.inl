#pragma once

#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <daxa/daxa.inl>

#define CHUNK_AXIS_SIZE 8
#define CHUNK_TOTAL_SIZE (CHUNK_AXIS_SIZE * CHUNK_AXIS_SIZE * CHUNK_AXIS_SIZE)

#define PALETTE_REGION_AXIS_SIZE 8
#define PALETTE_REGION_TOTAL_SIZE (PALETTE_REGION_AXIS_SIZE * PALETTE_REGION_AXIS_SIZE * PALETTE_REGION_AXIS_SIZE)

#define PALETTE_REGION_AXIS_N (CHUNK_AXIS_SIZE / PALETTE_REGION_AXIS_SIZE)
#define PALETTE_REGION_TOTAL_N (PALETTE_REGION_AXIS_N * PALETTE_REGION_AXIS_N * PALETTE_REGION_AXIS_N)

struct GpuInput {
    u32 data[CHUNK_TOTAL_SIZE];
};
DAXA_ENABLE_BUFFER_PTR(GpuInput)

struct GpuCompressState {
    u32 current_size;
};
DAXA_ENABLE_BUFFER_PTR(GpuCompressState)

struct RegionHeader {
    u32 variant_n;
    u32 blob_offset; // if variant_n == 1, this is just the voxel
};

struct NodeHeader {
    u32 node_full_size;

    u32 region_count_x;
    u32 region_count_y;
    u32 region_count_z;
};

struct GpuOutput {
    u32 node_n;
    NodeHeader node_header;
    RegionHeader region_headers[PALETTE_REGION_TOTAL_N];
    u32 data[CHUNK_TOTAL_SIZE * 2];
};
DAXA_ENABLE_BUFFER_PTR(GpuOutput)

struct GpuCompPush {
    daxa_RWBufferPtr(GpuInput) gpu_input;
    daxa_RWBufferPtr(GpuCompressState) gpu_compress_state;
    daxa_RWBufferPtr(GpuOutput) gpu_output;
};

#define INPUT deref(daxa_push_constant.gpu_input)
#define COMPRESS_STATE deref(daxa_push_constant.gpu_compress_state)
#define OUTPUT deref(daxa_push_constant.gpu_output)
