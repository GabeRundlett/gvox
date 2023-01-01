#pragma once

#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <daxa/daxa.inl>

#define CHUNK_AXIS_SIZE 8
#define CHUNK_TOTAL_SIZE (CHUNK_AXIS_SIZE * CHUNK_AXIS_SIZE * CHUNK_AXIS_SIZE)

#define PALETTE_CHUNK_AXIS_SIZE 8
#define PALETTE_CHUNK_TOTAL_SIZE (PALETTE_CHUNK_AXIS_SIZE * PALETTE_CHUNK_AXIS_SIZE * PALETTE_CHUNK_AXIS_SIZE)

#define PALETTE_CHUNK_AXIS_N (CHUNK_AXIS_SIZE / PALETTE_CHUNK_AXIS_SIZE)
#define PALETTE_CHUNK_TOTAL_N (PALETTE_CHUNK_AXIS_N * PALETTE_CHUNK_AXIS_N * PALETTE_CHUNK_AXIS_N)

struct GpuInput {
    u32 data[CHUNK_TOTAL_SIZE];
};
DAXA_ENABLE_BUFFER_PTR(GpuInput)

struct GpuCompressState {
    u32 current_size;
};
DAXA_ENABLE_BUFFER_PTR(GpuCompressState)

struct GpuOutput {
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
