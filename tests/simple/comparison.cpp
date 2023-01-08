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
#define TEST_GPU 1

using namespace daxa::types;
// using namespace daxa::math_operators;
#include "gpu.inl"

using Clock = std::chrono::high_resolution_clock;

struct Timer {
    Clock::time_point start = Clock::now();

    Timer() = default;
    Timer(Timer const &) = default;
    Timer(Timer &&) = default;
    auto operator=(Timer const &) -> Timer & = default;
    auto operator=(Timer &&) -> Timer & = default;

    ~Timer() {
        auto now = Clock::now();
        std::cout << "elapsed: " << std::chrono::duration<float>(now - start).count() << std::endl;
    }
};

void run_gpu_version(GVoxContext *gvox, GVoxScene const &scene);

auto main() -> int {
    GVoxContext *gvox = gvox_create_context();

#if TEST_ALL
    GVoxScene const scene = create_scene(256, 256, 256);
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
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gzip.gvox", "gzip");
        std::cout << "save zlib             | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_gzip.gvox");
        std::cout << "load gzip             | " << std::flush;
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
#elif TEST_GPU
    GVoxScene scene = []() {
        Timer const timer{};
        auto const size = CHUNK_AXIS_SIZE;
        return create_scene(size, size, size);
    }();
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_u32_palette");
        // gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
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

    {
        auto cpu_scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
        std::cout << "loaded CPU scene content:" << std::endl;
        print_voxels(cpu_scene);
        gvox_destroy_scene(cpu_scene);
    }

    std::cout << "\nloaded GPU scene content:" << std::endl;

    run_gpu_version(gvox, scene);

    gvox_destroy_scene(scene);
    // scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32.gvox");
    // std::cout << "\nloaded uncompressed scene content:" << std::endl;
    // print_voxels(scene);
    // gvox_destroy_scene(scene);
#else
    // auto scene = gvox_load_from_raw(gvox, "tests/simple/#phantom_mansion.vox", "magicavoxel");
    // gvox_save(gvox, scene, "tests/simple/phantom_mansion_plt.gvox", "gvox_u32_palette");
    auto scene = gvox_load_from_raw(gvox, "tests/simple/LostValleyArena.vxl", "ace_of_spades");
    // gvox_save(gvox, scene, "tests/simple/Arab2.vxl", "ace_of_spades");
    // gvox_destroy_scene(scene);
    // scene = gvox_load_from_raw(gvox, "tests/simple/Arab2.vxl", "ace_of_spades");
    gvox_save(gvox, scene, "tests/simple/arab.gvox", "gvox_u32_palette");
    gvox_destroy_scene(scene);
#endif

    gvox_destroy_context(gvox);
}

void run_gpu_version(GVoxContext *gvox, GVoxScene const &scene) {
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
    auto *gpu_input_ptr = new GpuInput{};
    GpuInput &gpu_input = *gpu_input_ptr;
    daxa::BufferId gpu_input_buffer = device.create_buffer(daxa::BufferInfo{
        .size = sizeof(GpuInput),
        .debug_name = "gpu_input_buffer",
    });
    daxa::BufferId gpu_compress_state_buffer = device.create_buffer(daxa::BufferInfo{
        .size = sizeof(GpuCompressState),
        .debug_name = "gpu_compress_state_buffer",
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
    size_t const sx = CHUNK_AXIS_SIZE;
    size_t const sy = CHUNK_AXIS_SIZE;
    size_t const sz = CHUNK_AXIS_SIZE;
    for (size_t zi = 0; zi < sz; ++zi) {
        for (size_t yi = 0; yi < sy; ++yi) {
            for (size_t xi = 0; xi < sx; ++xi) {
                auto const voxel_i = xi + yi * sx + zi * sx * sy;
                auto const &i_vox = scene.nodes[0].voxels[voxel_i];
                auto u32_voxel = 0u;
                auto const r = static_cast<uint32_t>(std::max(std::min(i_vox.color.x, 1.0f), 0.0f) * 255.0f);
                auto const g = static_cast<uint32_t>(std::max(std::min(i_vox.color.y, 1.0f), 0.0f) * 255.0f);
                auto const b = static_cast<uint32_t>(std::max(std::min(i_vox.color.z, 1.0f), 0.0f) * 255.0f);
                auto const i = i_vox.id;
                u32_voxel = u32_voxel | (r << 0x00);
                u32_voxel = u32_voxel | (g << 0x08);
                u32_voxel = u32_voxel | (b << 0x10);
                u32_voxel = u32_voxel | (i << 0x18);
                gpu_input.data[voxel_i] = u32_voxel;
            }
        }
    }
    daxa::CommandSubmitInfo submit_info;
    daxa::TaskList task_list = daxa::TaskList({
        .device = device,
    });
    auto task_gpu_input_buffer = task_list.create_task_buffer({.debug_name = "task_gpu_input_buffer"});
    task_list.add_runtime_buffer(task_gpu_input_buffer, gpu_input_buffer);
    auto task_gpu_compress_state_buffer = task_list.create_task_buffer({.debug_name = "task_gpu_compress_state_buffer"});
    task_list.add_runtime_buffer(task_gpu_compress_state_buffer, gpu_compress_state_buffer);
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
            auto *buffer_ptr = device.get_host_address_as<GpuInput>(staging_gpu_input_buffer);
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
            {task_gpu_compress_state_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_ONLY},
            {task_gpu_output_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_WRITE},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            auto cmd_list = task_runtime.get_command_list();
            cmd_list.set_pipeline(*gpu_comp_pipeline);
            cmd_list.push_constant(GpuCompPush{
                .gpu_input = device.get_device_address(gpu_input_buffer),
                .gpu_compress_state = device.get_device_address(gpu_compress_state_buffer),
                .gpu_output = device.get_device_address(gpu_output_buffer),
            });
            cmd_list.dispatch(PALETTE_REGION_AXIS_N, PALETTE_REGION_AXIS_N, PALETTE_REGION_AXIS_N);
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
    {
        Timer const timer{};
        task_list.submit(&submit_info);
        task_list.complete();
        task_list.execute();
        device.wait_idle();
    }
    auto *buffer_ptr = device.get_host_address_as<uint8_t>(staging_gpu_output_buffer);

    // auto &gpu_output = *reinterpret_cast<GpuOutput *>(buffer_ptr);
    // std::cout << "offset = " << gpu_output.offset << std::endl;

    if (buffer_ptr == nullptr) {
        device.wait_idle();
        device.collect_garbage();
        device.destroy_buffer(gpu_input_buffer);
        device.destroy_buffer(gpu_compress_state_buffer);
        device.destroy_buffer(gpu_output_buffer);
        device.destroy_buffer(staging_gpu_output_buffer);
        return;
    }

    GVoxScene const gpu_scene = gvox_parse(gvox, GVoxPayload{.size = 0, .data = buffer_ptr}, "gvox_u32_palette");

    print_voxels(gpu_scene);
    gvox_destroy_scene(gpu_scene);

    device.wait_idle();
    device.collect_garbage();
    device.destroy_buffer(gpu_input_buffer);
    device.destroy_buffer(gpu_compress_state_buffer);
    device.destroy_buffer(gpu_output_buffer);
    device.destroy_buffer(staging_gpu_output_buffer);
}
