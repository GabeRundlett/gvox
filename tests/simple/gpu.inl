#pragma once

#define DAXA_ENABLE_SHADER_NO_NAMESPACE 1
#include <daxa/daxa.inl>

struct GpuInput {
    u32 data[512];
};
DAXA_ENABLE_BUFFER_PTR(GpuInput)

struct GpuOutput {
    u32 data[512];
    u32 palette_size;
};
DAXA_ENABLE_BUFFER_PTR(GpuOutput)

struct GpuCompPush {
    daxa_RWBufferPtr(GpuInput) gpu_input;
    daxa_RWBufferPtr(GpuOutput) gpu_output;
};
