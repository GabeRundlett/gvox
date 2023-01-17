#include <cstdlib>
#include <cstring>
#include <cmath>

#include <gvox/gvox.h>

#include <chrono>
#include <iostream>
#include <thread>

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/task_list.hpp>

using namespace daxa::types;
#include "gpu.inl"

void run_gpu_version(GVoxContext *gvox, GVoxScene const &scene);

auto print_voxels(Voxel *voxels, size_t size_x, size_t size_y, size_t size_z) {
    for (size_t yi = 0; yi < size_y; yi += 1) {
        for (size_t zi = 0; zi < size_z; zi += 1) {
            for (size_t xi = 0; xi < size_x; xi += 1) {
                size_t const i = xi + yi * size_x + zi * size_x * size_y;
                auto &voxel = voxels[i];
                printf("\033[48;2;%03d;%03d;%03dm  ", (uint32_t)(voxel.col.x * 255), (uint32_t)(voxel.col.y * 255), (uint32_t)(voxel.col.z * 255));
            }
            printf("\033[0m ");
        }
        printf("\033[0m\n");
    }
}

auto main() -> int {
    GVoxContext *gvox = gvox_create_context();

    auto daxa_ctx = daxa::create_context({.enable_validation = false});
    auto device = daxa_ctx.create_device({});
    daxa::PipelineManager pipeline_manager = daxa::PipelineManager({
        .device = device,
        .shader_compile_options = {
            .root_paths = {
                "tests/raster",
                DAXA_SHADER_INCLUDE_DIR,
            },
            .language = daxa::ShaderLanguage::GLSL,
            .enable_debug_info = true,
        },
        .debug_name = "pipeline_manager",
    });
    auto *gpu_input_ptr = new GpuInput{};
    GpuInput &gpu_input = *gpu_input_ptr;
    gpu_input.size = {8, 8, 8};
    u32 vert_n = 3;
    auto *verts = new Vertex[vert_n];
    verts[0].pos = f32vec3{-0.9f, +0.9f, +0.1f};
    verts[0].col = f32vec3{1.0f, 0.0f, 0.0f};
    verts[1].pos = f32vec3{+0.9f, +0.9f, +0.9f};
    verts[1].col = f32vec3{0.0f, 1.0f, 0.0f};
    verts[2].pos = f32vec3{+0.9f, -0.9f, +0.9f};
    verts[2].col = f32vec3{0.0f, 0.0f, 1.0f};
    daxa::BufferId gpu_input_buffer = device.create_buffer(daxa::BufferInfo{
        .size = sizeof(GpuInput),
        .debug_name = "gpu_input_buffer",
    });
    daxa::BufferId vertex_buffer = device.create_buffer(daxa::BufferInfo{
        .size = static_cast<u32>(sizeof(Vertex) * vert_n),
        .debug_name = "vertex_buffer",
    });
    daxa::BufferId voxel_buffer = device.create_buffer(daxa::BufferInfo{
        .size = static_cast<u32>(sizeof(Voxel) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z),
        .debug_name = "voxel_buffer",
    });
    daxa::BufferId staging_voxel_buffer = device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
        .size = static_cast<u32>(sizeof(Voxel) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z),
        .debug_name = "staging_voxel_buffer",
    });
    std::shared_ptr<daxa::RasterPipeline> raster_pipeline = [&]() {
        auto result = pipeline_manager.add_raster_pipeline({
            .vertex_shader_info = {
                .source = daxa::ShaderFile{"raster.glsl"},
                .compile_options = {.defines = {{"RASTER_VERT", "1"}}},
            },
            .fragment_shader_info = {
                .source = daxa::ShaderFile{"raster.glsl"},
                .compile_options = {.defines = {{"RASTER_FRAG", "1"}}},
            },
            .push_constant_size = sizeof(RasterPush),
            .debug_name = "raster_pipeline",
        });
        if (result.is_err()) {
            std::cout << result.to_string() << std::endl;
        }
        return result.value();
    }();
    daxa::CommandSubmitInfo submit_info;
    daxa::TaskList task_list = daxa::TaskList({
        .device = device,
    });
    auto task_gpu_input_buffer = task_list.create_task_buffer({.debug_name = "task_gpu_input_buffer"});
    task_list.add_runtime_buffer(task_gpu_input_buffer, gpu_input_buffer);
    auto task_vertex_buffer = task_list.create_task_buffer({.debug_name = "task_vertex_buffer"});
    task_list.add_runtime_buffer(task_vertex_buffer, vertex_buffer);
    auto task_voxel_buffer = task_list.create_task_buffer({.debug_name = "task_voxel_buffer"});
    task_list.add_runtime_buffer(task_voxel_buffer, voxel_buffer);
    auto task_staging_voxel_buffer = task_list.create_task_buffer({.debug_name = "task_staging_voxel_buffer"});
    task_list.add_runtime_buffer(task_staging_voxel_buffer, staging_voxel_buffer);
    task_list.add_task({
        .used_buffers = {
            {task_gpu_input_buffer, daxa::TaskBufferAccess::TRANSFER_WRITE},
            {task_vertex_buffer, daxa::TaskBufferAccess::TRANSFER_WRITE},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            auto cmd_list = task_runtime.get_command_list();
            {
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
            }
            {
                auto staging_vertex_buffer = device.create_buffer({
                    .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                    .size = static_cast<u32>(sizeof(Vertex) * vert_n),
                    .debug_name = "staging_vertex_buffer",
                });
                cmd_list.destroy_buffer_deferred(staging_vertex_buffer);
                auto *buffer_ptr = device.get_host_address_as<Vertex>(staging_vertex_buffer);
                std::memcpy(buffer_ptr, verts, sizeof(Vertex) * vert_n);
                cmd_list.copy_buffer_to_buffer({
                    .src_buffer = staging_vertex_buffer,
                    .dst_buffer = vertex_buffer,
                    .size = sizeof(Vertex) * vert_n,
                });
            }
        },
        .debug_name = "Input Transfer",
    });
    task_list.add_task({
        .used_buffers = {
            {task_voxel_buffer, daxa::TaskBufferAccess::TRANSFER_WRITE},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            auto cmd_list = task_runtime.get_command_list();
            cmd_list.clear_buffer({
                .buffer = voxel_buffer,
                .offset = 0,
                .size = sizeof(Voxel) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z,
                .clear_value = {},
            });
        },
        .debug_name = "Clear Output",
    });
    task_list.add_task({
        .used_buffers = {
            {task_gpu_input_buffer, daxa::TaskBufferAccess::SHADER_READ_ONLY},
            {task_vertex_buffer, daxa::TaskBufferAccess::VERTEX_SHADER_READ_ONLY},
            {task_voxel_buffer, daxa::TaskBufferAccess::FRAGMENT_SHADER_WRITE_ONLY},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            auto cmd_list = task_runtime.get_command_list();
            cmd_list.set_pipeline(*raster_pipeline);
            cmd_list.begin_renderpass({.render_area = {.width = gpu_input.size.x, .height = gpu_input.size.y}});
            for (u32 zi = 0; zi < gpu_input.size.z; ++zi) {
                cmd_list.push_constant(RasterPush{
                    .gpu_input = device.get_device_address(gpu_input_buffer),
                    .vertex_buffer = device.get_device_address(vertex_buffer),
                    .voxel_buffer = device.get_device_address(voxel_buffer),
                    .out_z = zi,
                });
                cmd_list.draw({.vertex_count = 3});
            }
            cmd_list.end_renderpass();
        },
        .debug_name = "draw",
    });
    task_list.add_task({
        .used_buffers = {
            {task_voxel_buffer, daxa::TaskBufferAccess::TRANSFER_READ},
            {task_staging_voxel_buffer, daxa::TaskBufferAccess::TRANSFER_WRITE},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            auto cmd_list = task_runtime.get_command_list();
            cmd_list.copy_buffer_to_buffer({
                .src_buffer = voxel_buffer,
                .dst_buffer = staging_voxel_buffer,
                .size = sizeof(Voxel) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z,
            });
        },
        .debug_name = "Output Transfer",
    });
    task_list.submit(&submit_info);
    task_list.complete();
    task_list.execute();
    device.wait_idle();
    auto *buffer_ptr = device.get_host_address_as<Voxel>(staging_voxel_buffer);
    if (buffer_ptr == nullptr) {
        device.collect_garbage();
        device.destroy_buffer(gpu_input_buffer);
        device.destroy_buffer(vertex_buffer);
        device.destroy_buffer(voxel_buffer);
        device.destroy_buffer(staging_voxel_buffer);
        return -1;
    }
    print_voxels(buffer_ptr, gpu_input.size.x, gpu_input.size.y, gpu_input.size.z);
    device.collect_garbage();
    device.destroy_buffer(gpu_input_buffer);
    device.destroy_buffer(vertex_buffer);
    device.destroy_buffer(voxel_buffer);
    device.destroy_buffer(staging_voxel_buffer);

    gvox_destroy_context(gvox);
}
