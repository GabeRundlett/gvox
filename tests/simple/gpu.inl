#pragma once

#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <daxa/daxa.inl>

#define PALETTE_CHUNK_AXIS_SIZE 8
#define PALETTE_CHUNK_TOTAL_SIZE (PALETTE_CHUNK_AXIS_SIZE * PALETTE_CHUNK_AXIS_SIZE * PALETTE_CHUNK_AXIS_SIZE)

struct GpuInput {
    u32 data[PALETTE_CHUNK_TOTAL_SIZE];
};
DAXA_ENABLE_BUFFER_PTR(GpuInput)

// struct GpuCompressState {
//     u32 size;
// };

#define OUTPUT_COMPRESSED 1

struct GpuOutput {
#if OUTPUT_COMPRESSED
    u32 data[PALETTE_CHUNK_TOTAL_SIZE * 2];
#else
    u32 palette_size;
    u32 data[512];
#endif
};
DAXA_ENABLE_BUFFER_PTR(GpuOutput)

struct GpuCompPush {
    daxa_RWBufferPtr(GpuInput) gpu_input;
    daxa_RWBufferPtr(GpuOutput) gpu_output;
};
