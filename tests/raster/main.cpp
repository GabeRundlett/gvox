#include "model.hpp"

#include <cstdlib>
#include <cstring>
#include <cmath>

#include <gvox/gvox.h>

#include <chrono>
#include <iostream>
#include <thread>
#include <filesystem>

#include <daxa/daxa.hpp>
#include <daxa/utils/pipeline_manager.hpp>
#include <daxa/utils/math_operators.hpp>
#include <daxa/utils/task_list.hpp>

using namespace daxa::types;
#include "gpu.inl"

// auto print_voxels(Voxel *voxels, size_t size_x, size_t size_y, size_t size_z) {
//     for (size_t yi = 0; yi < size_y; yi += 1) {
//         for (size_t zi = 0; zi < size_z; zi += 1) {
//             for (size_t xi = 0; xi < size_x; xi += 1) {
//                 size_t const i = xi + yi * size_x + zi * size_x * size_y;
//                 auto &voxel = voxels[i];
//                 printf("\033[48;2;%03d;%03d;%03dm  ", (uint32_t)(voxel.col.x * 255), (uint32_t)(voxel.col.y * 255), (uint32_t)(voxel.col.z * 255));
//             }
//             printf("\033[0m ");
//         }
//         printf("\033[0m\n");
//     }
// }

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
    gpu_input.size = {512, 512, 512};
    Model model;
    // open_model(device, model, "assets/suzanne.dae");
    open_model(device, model, "C:/dev/projects/cpp/mesh-renderer/assets/suzanne.dae");
    daxa::SamplerId texture_sampler = device.create_sampler({
        .magnification_filter = daxa::Filter::LINEAR,
        .minification_filter = daxa::Filter::LINEAR,
        .address_mode_u = daxa::SamplerAddressMode::REPEAT,
        .address_mode_v = daxa::SamplerAddressMode::REPEAT,
        .address_mode_w = daxa::SamplerAddressMode::REPEAT,
        .debug_name = "texture_sampler",
    });
    daxa::BufferId gpu_input_buffer = device.create_buffer(daxa::BufferInfo{
        .size = sizeof(GpuInput),
        .debug_name = "gpu_input_buffer",
    });
    daxa::BufferId voxel_buffer = device.create_buffer(daxa::BufferInfo{
        .size = static_cast<u32>(sizeof(u32) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z),
        .debug_name = "voxel_buffer",
    });
    daxa::BufferId staging_voxel_buffer = device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
        .size = static_cast<u32>(sizeof(u32) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z),
        .debug_name = "staging_voxel_buffer",
    });
    std::shared_ptr<daxa::ComputePipeline> preprocess_pipeline = [&]() {
        auto result = pipeline_manager.add_compute_pipeline({
            .shader_info = {
                .source = daxa::ShaderFile{"preprocess.glsl"},
            },
            .push_constant_size = sizeof(PreprocessPush),
            .debug_name = "preprocess_pipeline",
        });
        if (result.is_err()) {
            std::cout << result.to_string() << std::endl;
        }
        return result.value();
    }();
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
            // .raster = {
            //     .conservative_raster_info = daxa::ConservativeRasterInfo{
            //         .mode = daxa::ConservativeRasterizationMode::OVERESTIMATE,
            //         .size = 0.01f,
            //     },
            // },
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
    auto task_image_id = task_list.create_task_image({.debug_name = "task_image_id"});
    for (auto const &mesh : model.meshes) {
        task_list.add_runtime_buffer(task_vertex_buffer, mesh.vertex_buffer);
        task_list.add_runtime_image(task_image_id, mesh.textures[0]->image_id);
    }
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
                usize vert_n = 0;
                for (auto const &mesh : model.meshes) {
                    vert_n += mesh.verts.size();
                }
                auto staging_vertex_buffer = device.create_buffer({
                    .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
                    .size = static_cast<u32>(sizeof(Vertex) * vert_n),
                    .debug_name = "staging_vertex_buffer",
                });
                cmd_list.destroy_buffer_deferred(staging_vertex_buffer);
                auto *buffer_ptr = device.get_host_address_as<Vertex>(staging_vertex_buffer);
                usize vert_offset = 0;
                for (auto const &mesh : model.meshes) {
                    std::memcpy(buffer_ptr + vert_offset, mesh.verts.data(), sizeof(Vertex) * mesh.verts.size());
                    cmd_list.copy_buffer_to_buffer({
                        .src_buffer = staging_vertex_buffer,
                        .src_offset = sizeof(Vertex) * vert_offset,
                        .dst_buffer = mesh.vertex_buffer,
                        .size = sizeof(Vertex) * mesh.verts.size(),
                    });
                    vert_offset += mesh.verts.size();
                }
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
                .size = sizeof(u32) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z,
                .clear_value = {},
            });
        },
        .debug_name = "Clear Output",
    });
    task_list.add_task({
        .used_buffers = {
            {task_gpu_input_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_ONLY},
            {task_vertex_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_WRITE},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            for (auto const &mesh : model.meshes) {
                auto cmd_list = task_runtime.get_command_list();
                cmd_list.set_pipeline(*preprocess_pipeline);
                cmd_list.push_constant(PreprocessPush{
                    .modl_mat = mesh.modl_mat,
                    .gpu_input = device.get_device_address(gpu_input_buffer),
                    .vertex_buffer = device.get_device_address(mesh.vertex_buffer),
                });
                cmd_list.dispatch(static_cast<u32>(mesh.verts.size() / 3));
            }
        },
        .debug_name = "Preprocess Verts",
    });
    task_list.add_task({
        .used_buffers = {
            {task_gpu_input_buffer, daxa::TaskBufferAccess::SHADER_READ_ONLY},
            {task_vertex_buffer, daxa::TaskBufferAccess::VERTEX_SHADER_READ_ONLY},
            {task_voxel_buffer, daxa::TaskBufferAccess::FRAGMENT_SHADER_WRITE_ONLY},
        },
        .used_images = {
            {task_image_id, daxa::TaskImageAccess::SHADER_READ_ONLY, daxa::ImageMipArraySlice{.base_mip_level = 0, .level_count = 4}},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            auto cmd_list = task_runtime.get_command_list();
            cmd_list.set_pipeline(*raster_pipeline);
            cmd_list.begin_renderpass({.render_area = {.width = gpu_input.size.x, .height = gpu_input.size.y}});
            for (auto const &mesh : model.meshes) {
                cmd_list.push_constant(RasterPush{
                    .gpu_input = device.get_device_address(gpu_input_buffer),
                    .vertex_buffer = device.get_device_address(mesh.vertex_buffer),
                    .voxel_buffer = device.get_device_address(voxel_buffer),
                    .texture_id = mesh.textures[0]->image_id.default_view(),
                    .texture_sampler = texture_sampler,
                });
                cmd_list.draw({.vertex_count = static_cast<u32>(mesh.verts.size())});
            }
            cmd_list.end_renderpass();
        },
        .debug_name = "Raster to Voxels",
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
                .size = sizeof(u32) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z,
            });
        },
        .debug_name = "Output Transfer",
    });
    task_list.submit(&submit_info);
    task_list.complete();
    task_list.execute();
    device.wait_idle();
    auto *buffer_ptr = device.get_host_address_as<u32>(staging_voxel_buffer);
    if (buffer_ptr == nullptr) {
        device.collect_garbage();
        device.destroy_buffer(gpu_input_buffer);
        for (auto &mesh : model.meshes) {
            device.destroy_buffer(mesh.vertex_buffer);
        }
        device.destroy_buffer(voxel_buffer);
        device.destroy_buffer(staging_voxel_buffer);
        return -1;
    }
    // print_voxels(buffer_ptr, gpu_input.size.x, gpu_input.size.y, gpu_input.size.z);
    GVoxScene scene;
    scene.node_n = 1;
    scene.nodes = (GVoxSceneNode *)malloc(sizeof(GVoxSceneNode) * scene.node_n);
    auto &node = scene.nodes[0];
    node.size_x = gpu_input.size.x;
    node.size_y = gpu_input.size.y;
    node.size_z = gpu_input.size.z;
    size_t const voxel_n = node.size_x * node.size_y * node.size_z;
    node.voxels = (GVoxVoxel *)malloc(sizeof(GVoxVoxel) * voxel_n);
    for (size_t voxel_i = 0; voxel_i < node.size_x * node.size_y * node.size_z; ++voxel_i) {
        auto u32_voxel = buffer_ptr[voxel_i];
        float r = static_cast<float>((u32_voxel >> 0x00) & 0xff) / 255.0f;
        float g = static_cast<float>((u32_voxel >> 0x08) & 0xff) / 255.0f;
        float b = static_cast<float>((u32_voxel >> 0x10) & 0xff) / 255.0f;
        uint32_t const i = (u32_voxel >> 0x18) & 0xff;
        node.voxels[voxel_i] = GVoxVoxel{
            .color = {r, g, b},
            .id = i,
        };
    }
    gvox_save(gvox, &scene, "C:/dev/projects/cpp/gvox_engine/assets/minecraft.gvox", "gvox_u32_palette");
    gvox_destroy_scene(&scene);
    device.collect_garbage();
    device.destroy_buffer(gpu_input_buffer);
    for (auto &mesh : model.meshes) {
        device.destroy_buffer(mesh.vertex_buffer);
    }
    for (auto &[key, texture] : model.textures) {
        device.destroy_image(texture->image_id);
    }
    device.destroy_sampler(texture_sampler);
    device.destroy_buffer(voxel_buffer);
    device.destroy_buffer(staging_voxel_buffer);
    gvox_destroy_context(gvox);
}
