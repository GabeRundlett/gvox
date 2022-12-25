#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <gvox/gvox.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "print.h"
#include "scene.h"

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/task_list.hpp>

#define TEST_ALL 0

using namespace daxa::types;
// using namespace daxa::math_operators;
#include "gpu.inl"

using Clock = std::chrono::high_resolution_clock;

struct Timer {
    Clock::time_point start = Clock::now();

    Timer() = default;
    Timer(Timer const &) = default;
    Timer(Timer &&) = default;
    Timer &operator=(Timer const &) = default;
    Timer &operator=(Timer &&) = default;

    ~Timer() {
        auto now = Clock::now();
        std::cout << "elapsed: " << std::chrono::duration<float>(now - start).count() << std::endl;
    }
};

void run_gpu_version(GVoxScene const &scene);

auto main() -> int {
    GVoxContext *gvox = gvox_create_context();

#if TEST_ALL
    GVoxScene const scene = create_scene(64, 64, 64);
    GVoxScene loaded_scene;
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_raw.gvox", "gvox_raw");
        std::cout << "save gvox_raw         | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_raw.gvox");
        std::cout << "load gvox_raw         | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
        std::cout << "save gvox_u32         | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32.gvox");
        std::cout << "load gvox_u32         | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_u32_palette");
        std::cout << "save gvox_u32_palette | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
        std::cout << "load gvox_u32_palette | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_zlib.gvox", "zlib");
        std::cout << "save zlib             | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_zlib.gvox");
        std::cout << "load zlib             | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_magicavoxel.gvox", "magicavoxel");
        std::cout << "save magicavoxel      | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_magicavoxel.gvox");
        std::cout << "load magicavoxel      | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);

    gvox_destroy_scene(scene);
#else
    GVoxScene scene = []() {
        Timer const timer{};
        auto const size = 8 * 1;
        return create_scene(size, size, size);
    }();
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_u32_palette");
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
        while (gvox_get_result(gvox) != GVOX_SUCCESS) {
            size_t msg_size = 0;
            gvox_get_result_message(gvox, nullptr, &msg_size);
            std::string msg;
            msg.resize(msg_size);
            gvox_get_result_message(gvox, nullptr, &msg_size);
            gvox_pop_result(gvox);
            gvox_destroy_scene(scene);
            return -1;
        }
    }
    // std::cout << "generated scene content:" << std::endl;
    // print_voxels(scene);
    run_gpu_version(scene);

    gvox_destroy_scene(scene);
    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32.gvox");
    std::cout << "\nloaded uncompressed scene content:" << std::endl;
    print_voxels(scene);
    gvox_destroy_scene(scene);
    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
    std::cout << "\nloaded compressed scene content:" << std::endl;
    print_voxels(scene);
    gvox_destroy_scene(scene);

#endif

    gvox_destroy_context(gvox);
}

void run_gpu_version(GVoxScene const &scene) {
    auto daxa_ctx = daxa::create_context({});
    auto device = daxa_ctx.create_device({});
    daxa::PipelineManager pipeline_manager = daxa::PipelineManager({
        .device = device,
        .shader_compile_options = {
            .root_paths = {
                "tests/simple",
                DAXA_SHADER_INCLUDE_DIR,
            },
            .language = daxa::ShaderLanguage::GLSL,
            .enable_debug_info = true,
        },
        .debug_name = "pipeline_manager",
    });
    GpuInput gpu_input = {};
    daxa::BufferId gpu_input_buffer = device.create_buffer(daxa::BufferInfo{
        .size = sizeof(GpuInput),
        .debug_name = "gpu_input_buffer",
    });
    daxa::BufferId gpu_output_buffer = device.create_buffer(daxa::BufferInfo{
        .size = sizeof(GpuOutput),
        .debug_name = "gpu_output_buffer",
    });
    daxa::BufferId staging_gpu_output_buffer = device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
        .size = sizeof(GpuOutput),
        .debug_name = "staging_gpu_output_buffer",
    });
    std::shared_ptr<daxa::ComputePipeline> gpu_comp_pipeline = [&]() {
        auto result = pipeline_manager.add_compute_pipeline({
            .shader_info = {.source = daxa::ShaderFile{"gpu.comp.glsl"}},
            .push_constant_size = sizeof(GpuCompPush),
            .debug_name = "gpu_comp_pipeline",
        });
        if (result.is_err()) {
            std::cout << result.to_string() << std::endl;
        }
        return result.value();
    }();
    uint32_t sx = 8;
    uint32_t sy = 8;
    uint32_t sz = 8;
    uint32_t min_voxel = 0xffffffff;
    for (size_t zi = 0; zi < sz; ++zi) {
        for (size_t yi = 0; yi < sy; ++yi) {
            for (size_t xi = 0; xi < sx; ++xi) {
                uint32_t const node_i = 0;
                uint32_t const voxel_i = xi + yi * sx + zi * sx * sy;
                auto const &i_vox = scene.nodes[node_i].voxels[voxel_i];
                uint32_t u32_voxel = 0;
                uint32_t const r = static_cast<uint32_t>(std::max(std::min(i_vox.color.x, 1.0f), 0.0f) * 255.0f);
                uint32_t const g = static_cast<uint32_t>(std::max(std::min(i_vox.color.y, 1.0f), 0.0f) * 255.0f);
                uint32_t const b = static_cast<uint32_t>(std::max(std::min(i_vox.color.z, 1.0f), 0.0f) * 255.0f);
                uint32_t const i = i_vox.id;
                u32_voxel = u32_voxel | (r << 0x00);
                u32_voxel = u32_voxel | (g << 0x08);
                u32_voxel = u32_voxel | (b << 0x10);
                u32_voxel = u32_voxel | (i << 0x18);
                gpu_input.data[voxel_i] = u32_voxel;
                if (u32_voxel < min_voxel)
                    min_voxel = u32_voxel;
            }
        }
    }
    daxa::CommandSubmitInfo submit_info;
    daxa::TaskList task_list = daxa::TaskList({
        .device = device,
    });
    auto task_gpu_input_buffer = task_list.create_task_buffer({.debug_name = "task_gpu_input_buffer"});
    task_list.add_runtime_buffer(task_gpu_input_buffer, gpu_input_buffer);
    auto task_gpu_output_buffer = task_list.create_task_buffer({.debug_name = "task_gpu_output_buffer"});
    task_list.add_runtime_buffer(task_gpu_output_buffer, gpu_output_buffer);
    auto task_staging_gpu_output_buffer = task_list.create_task_buffer({.debug_name = "task_staging_gpu_output_buffer"});
    task_list.add_runtime_buffer(task_staging_gpu_output_buffer, staging_gpu_output_buffer);
    task_list.add_task({
        .used_buffers = {
            {task_gpu_input_buffer, daxa::TaskBufferAccess::TRANSFER_WRITE},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            auto cmd_list = task_runtime.get_command_list();
            auto staging_gpu_input_buffer = device.create_buffer({
                .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                .size = sizeof(GpuInput),
                .debug_name = "staging_gpu_input_buffer",
            });
            cmd_list.destroy_buffer_deferred(staging_gpu_input_buffer);
            GpuInput *buffer_ptr = device.get_host_address_as<GpuInput>(staging_gpu_input_buffer);
            *buffer_ptr = gpu_input;
            cmd_list.copy_buffer_to_buffer({
                .src_buffer = staging_gpu_input_buffer,
                .dst_buffer = gpu_input_buffer,
                .size = sizeof(GpuInput),
            });
        },
        .debug_name = "Input Transfer",
    });
    task_list.add_task({
        .used_buffers = {
            {task_gpu_input_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_ONLY},
            {task_gpu_output_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_WRITE},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            auto cmd_list = task_runtime.get_command_list();
            cmd_list.set_pipeline(*gpu_comp_pipeline);
            cmd_list.push_constant(GpuCompPush{
                .gpu_input = device.get_device_address(gpu_input_buffer),
                .gpu_output = device.get_device_address(gpu_output_buffer),
            });
            cmd_list.dispatch(1, 1, 1);
        },
        .debug_name = "gpu",
    });
    task_list.add_task({
        .used_buffers = {
            {task_gpu_output_buffer, daxa::TaskBufferAccess::TRANSFER_READ},
            {task_staging_gpu_output_buffer, daxa::TaskBufferAccess::TRANSFER_WRITE},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            auto cmd_list = task_runtime.get_command_list();
            cmd_list.copy_buffer_to_buffer({
                .src_buffer = gpu_output_buffer,
                .dst_buffer = staging_gpu_output_buffer,
                .size = sizeof(GpuOutput),
            });
        },
        .debug_name = "Output Transfer",
    });
    task_list.submit(&submit_info);
    task_list.complete();
    task_list.execute();
    device.wait_idle();
    GpuOutput *buffer_ptr = device.get_host_address_as<GpuOutput>(staging_gpu_output_buffer);
    if (!buffer_ptr) {
        goto cleanup;
    }
    GpuOutput &gpu_output = *buffer_ptr;
    std::cout << "GPU: " << std::hex << min_voxel << ", " << gpu_output.palette_size << std::dec << std::endl;
    GVoxScene gpu_scene;
    gpu_scene.node_n = 1;
    gpu_scene.nodes = (GVoxSceneNode *)malloc(sizeof(GVoxSceneNode) * gpu_scene.node_n);
    gpu_scene.nodes[0].size_x = sx;
    gpu_scene.nodes[0].size_y = sy;
    gpu_scene.nodes[0].size_z = sz;
    size_t const voxel_n = gpu_scene.nodes[0].size_x * gpu_scene.nodes[0].size_y * gpu_scene.nodes[0].size_z;
    gpu_scene.nodes[0].voxels = (GVoxVoxel *)malloc(sizeof(GVoxVoxel) * voxel_n);
    for (size_t zi = 0; zi < sz; ++zi) {
        for (size_t yi = 0; yi < sy; ++yi) {
            for (size_t xi = 0; xi < sx; ++xi) {
                uint32_t const node_i = 0;
                size_t const voxel_i = xi + yi * sx + zi * sx * sy;
                uint32_t u32_voxel = gpu_output.data[voxel_i];
                float r = static_cast<float>((u32_voxel >> 0x00) & 0xff) / 255.0f;
                float g = static_cast<float>((u32_voxel >> 0x08) & 0xff) / 255.0f;
                float b = static_cast<float>((u32_voxel >> 0x10) & 0xff) / 255.0f;
                uint32_t const i = (u32_voxel >> 0x18) & 0xff;
                gpu_scene.nodes[node_i].voxels[voxel_i] = GVoxVoxel{
                    .color = {r, g, b},
                    .id = i,
                };
            }
        }
    }
    print_voxels(gpu_scene);
    gvox_destroy_scene(gpu_scene);

cleanup:
    device.wait_idle();
    device.collect_garbage();
    device.destroy_buffer(gpu_input_buffer);
    device.destroy_buffer(gpu_output_buffer);
    device.destroy_buffer(staging_gpu_output_buffer);
}
