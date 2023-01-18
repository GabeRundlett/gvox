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

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stb_image.h>
#include <cassert>

using namespace daxa::types;
#include "gpu.inl"

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

struct Texture {
    std::filesystem::path path;
    daxa::ImageId image_id;
    u32 size_x, size_y;
    i32 channel_n;
    u8 *pixels;
};
struct Mesh {
    std::vector<Vertex> verts;
    std::vector<std::shared_ptr<Texture>> textures;
    f32mat4x4 modl_mat;
    daxa::BufferId vertex_buffer;
};
using TextureMap = std::unordered_map<std::string, std::shared_ptr<Texture>>;
struct Model {
    TextureMap textures;
    std::vector<Mesh> meshes;
};

void load_textures(Mesh &mesh, TextureMap &global_textures, aiMaterial *mat, std::filesystem::path const &rootdir) {
    auto texture_n = std::min(1u, mat->GetTextureCount(aiTextureType_DIFFUSE));
    if (texture_n == 0) {
        mesh.textures.push_back(global_textures.at("#default_texture"));
    } else {
        for (u32 i = 0; i < texture_n; i++) {
            aiString str;
            mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
            auto path = (rootdir / str.C_Str()).make_preferred();
            auto texture_iter = global_textures.find(path.string());
            if (texture_iter != global_textures.end()) {
                mesh.textures.push_back(texture_iter->second);
            } else {
                mesh.textures.push_back(global_textures[path.string()] = std::make_shared<Texture>(Texture{.path = path}));
            }
        }
    }
}

void process_node(daxa::Device device, Model &model, aiNode *node, aiScene const *scene, std::filesystem::path const &rootdir) {
    for (u32 mesh_i = 0; mesh_i < node->mNumMeshes; ++mesh_i) {
        aiMesh *aimesh = scene->mMeshes[node->mMeshes[mesh_i]];
        model.meshes.push_back({});
        auto &o_mesh = model.meshes.back();
        o_mesh.modl_mat = *reinterpret_cast<f32mat4x4 *>(&node->mTransformation);
        o_mesh.modl_mat = daxa::math_operators::transpose(o_mesh.modl_mat);
        auto &verts = o_mesh.verts;
        verts.reserve(aimesh->mNumFaces * 3);
        for (usize face_i = 0; face_i < aimesh->mNumFaces; ++face_i) {
            for (u32 index_i = 0; index_i < 3; ++index_i) {
                auto vert_i = aimesh->mFaces[face_i].mIndices[index_i];
                auto pos = aimesh->mVertices[vert_i];
                auto tex = aimesh->mTextureCoords[0][vert_i];
                verts.push_back({
                    .pos = {pos.x, pos.y, pos.z},
                    .tex = {tex.x, tex.y},
                });
            }
        }
        aiMaterial *material = scene->mMaterials[aimesh->mMaterialIndex];
        load_textures(o_mesh, model.textures, material, rootdir);
        o_mesh.vertex_buffer = device.create_buffer(daxa::BufferInfo{
            .size = static_cast<u32>(sizeof(Vertex) * o_mesh.verts.size()),
            .debug_name = "vertex_buffer",
        });
    }
    for (u32 i = 0; i < node->mNumChildren; ++i) {
        process_node(device, model, node->mChildren[i], scene, rootdir);
    }
}

static auto default_texture_pixels = std::array<u32, 16 * 16>{
    // clang-format off
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,

    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,

    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,

    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,  0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    // clang-format on
};

void open_model(daxa::Device device, Model &model, std::filesystem::path const &filepath) {
    Assimp::Importer import{};
    aiScene const *scene = import.ReadFile(filepath.string(), aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        return;
    }
    model.textures["#default_texture"] = std::make_shared<Texture>();
    process_node(device, model, scene->mRootNode, scene, filepath.parent_path());
    auto texture_staging_buffers = std::vector<daxa::BufferId>{};
    for (auto &[key, texture] : model.textures) {
        i32 size_x{}, size_y{};
        if (key == "#default_texture") {
            texture->pixels = reinterpret_cast<u8 *>(default_texture_pixels.data());
            texture->size_x = static_cast<u32>(16);
            texture->size_y = static_cast<u32>(16);
        } else {
            texture->pixels = stbi_load(texture->path.string().c_str(), &size_x, &size_y, &texture->channel_n, 4);
            assert(texture->pixels != nullptr && "Failed to load image");
            texture->size_x = static_cast<u32>(size_x);
            texture->size_y = static_cast<u32>(size_y);
        }
        texture->image_id = device.create_image({
            .format = daxa::Format::R8G8B8A8_SRGB,
            .size = {texture->size_x, texture->size_y, 1},
            .mip_level_count = 4,
            .usage = daxa::ImageUsageFlagBits::SHADER_READ_ONLY | daxa::ImageUsageFlagBits::TRANSFER_SRC | daxa::ImageUsageFlagBits::TRANSFER_DST,
            .debug_name = "image",
        });
        auto sx = static_cast<u32>(texture->size_x);
        auto sy = static_cast<u32>(texture->size_y);
        auto src_channel_n = 4u;
        auto dst_channel_n = 4u;
        auto data = texture->pixels;
        auto &image_id = texture->image_id;
        usize image_size = sx * sy * sizeof(u8) * dst_channel_n;
        auto texture_staging_buffer = device.create_buffer({
            .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
            .size = static_cast<u32>(image_size),
            .debug_name = "texture_staging_buffer",
        });
        u8 *staging_buffer_ptr = device.get_host_address_as<u8>(texture_staging_buffer);
        for (usize i = 0; i < sx * sy; ++i) {
            usize src_offset = i * src_channel_n;
            usize dst_offset = i * dst_channel_n;
            for (usize ci = 0; ci < std::min(src_channel_n, dst_channel_n); ++ci) {
                staging_buffer_ptr[ci + dst_offset] = data[ci + src_offset];
            }
        }
        auto cmd_list = device.create_command_list({
            .debug_name = "cmd_list",
        });
        cmd_list.pipeline_barrier({
            .awaited_pipeline_access = daxa::AccessConsts::HOST_WRITE,
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_READ,
        });
        cmd_list.pipeline_barrier_image_transition({
            .awaited_pipeline_access = daxa::AccessConsts::HOST_WRITE,
            .waiting_pipeline_access = daxa::AccessConsts::TRANSFER_WRITE,
            .before_layout = daxa::ImageLayout::UNDEFINED,
            .after_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .image_slice = {
                .base_mip_level = 0,
                .level_count = 4,
                .base_array_layer = 0,
                .layer_count = 1,
            },
            .image_id = image_id,
        });
        cmd_list.copy_buffer_to_image({
            .buffer = texture_staging_buffer,
            .image = image_id,
            .image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
            .image_offset = {0, 0, 0},
            .image_extent = {sx, sy, 1},
        });
        cmd_list.complete();
        device.submit_commands({
            .command_lists = {std::move(cmd_list)},
        });
        texture_staging_buffers.push_back(texture_staging_buffer);
        daxa::TaskList mip_task_list = daxa::TaskList({
            .device = device,
            .debug_name = "mip task list",
        });
        auto task_mip_image = mip_task_list.create_task_image({.initial_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL, .debug_name = "task_mip_image"});
        mip_task_list.add_runtime_image(task_mip_image, image_id);
        for (u32 i = 0; i < 3; ++i) {
            mip_task_list.add_task({
                .used_images = {
                    {task_mip_image, daxa::TaskImageAccess::TRANSFER_READ, daxa::ImageMipArraySlice{.base_mip_level = i}},
                    {task_mip_image, daxa::TaskImageAccess::TRANSFER_WRITE, daxa::ImageMipArraySlice{.base_mip_level = i + 1}},
                },
                .task = [=, &device](daxa::TaskRuntime const &runtime) {
                    auto cmd_list = runtime.get_command_list();
                    auto images = runtime.get_images(task_mip_image);
                    for (auto image_id : images) {
                        auto image_info = device.info_image(image_id);
                        auto mip_size = std::array<i32, 3>{std::max<i32>(1, static_cast<i32>(image_info.size.x)), std::max<i32>(1, static_cast<i32>(image_info.size.y)), std::max<i32>(1, static_cast<i32>(image_info.size.z))};
                        for (u32 j = 0; j < i; ++j) {
                            mip_size = {std::max<i32>(1, mip_size[0] / 2), std::max<i32>(1, mip_size[1] / 2), std::max<i32>(1, mip_size[2] / 2)};
                        }
                        auto next_mip_size = std::array<i32, 3>{std::max<i32>(1, mip_size[0] / 2), std::max<i32>(1, mip_size[1] / 2), std::max<i32>(1, mip_size[2] / 2)};
                        cmd_list.blit_image_to_image({
                            .src_image = image_id,
                            .src_image_layout = daxa::ImageLayout::TRANSFER_SRC_OPTIMAL, // TODO: get from TaskRuntime
                            .dst_image = image_id,
                            .dst_image_layout = daxa::ImageLayout::TRANSFER_DST_OPTIMAL,
                            .src_slice = {
                                .image_aspect = image_info.aspect,
                                .mip_level = i,
                                .base_array_layer = 0,
                                .layer_count = 1,
                            },
                            .src_offsets = {{{0, 0, 0}, {mip_size[0], mip_size[1], mip_size[2]}}},
                            .dst_slice = {
                                .image_aspect = image_info.aspect,
                                .mip_level = i + 1,
                                .base_array_layer = 0,
                                .layer_count = 1,
                            },
                            .dst_offsets = {{{0, 0, 0}, {next_mip_size[0], next_mip_size[1], next_mip_size[2]}}},
                            .filter = daxa::Filter::LINEAR,
                        });
                    }
                },
                .debug_name = "mip_level_" + std::to_string(i),
            });
        }
        mip_task_list.add_task({
            .used_images = {
                {task_mip_image, daxa::TaskImageAccess::SHADER_READ_ONLY, daxa::ImageMipArraySlice{.base_mip_level = 0, .level_count = 4}},
            },
            .task = [](daxa::TaskRuntime const &) {},
            .debug_name = "Transition",
        });
        auto submit_info = daxa::CommandSubmitInfo{};
        mip_task_list.submit(&submit_info);
        mip_task_list.complete();
        mip_task_list.execute();
    }
    device.wait_idle();
    for (auto texture_staging_buffer : texture_staging_buffers) {
        device.destroy_buffer(texture_staging_buffer);
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
    gpu_input.size = {64, 64, 64};
    Model model;
    open_model(device, model, "assets/suzanne.dae");
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
        .size = static_cast<u32>(sizeof(Voxel) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z),
        .debug_name = "voxel_buffer",
    });
    daxa::BufferId staging_voxel_buffer = device.create_buffer({
        .memory_flags = daxa::MemoryFlagBits::HOST_ACCESS_RANDOM,
        .size = static_cast<u32>(sizeof(Voxel) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z),
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
                .size = sizeof(Voxel) * gpu_input.size.x * gpu_input.size.y * gpu_input.size.z,
                .clear_value = {},
            });
        },
        .debug_name = "Clear Output",
    });
    task_list.add_task({
        .used_buffers = {
            {task_gpu_input_buffer, daxa::TaskBufferAccess::SHADER_READ_ONLY},
            {task_vertex_buffer, daxa::TaskBufferAccess::COMPUTE_SHADER_READ_WRITE},
        },
        .task = [&](daxa::TaskRuntime task_runtime) {
            for (auto const &mesh : model.meshes) {
                auto cmd_list = task_runtime.get_command_list();
                cmd_list.set_pipeline(*preprocess_pipeline);
                cmd_list.push_constant(PreprocessPush{
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
                    .modl_mat = mesh.modl_mat,
                    .gpu_input = device.get_device_address(gpu_input_buffer),
                    .vertex_buffer = device.get_device_address(mesh.vertex_buffer),
                    .voxel_buffer = device.get_device_address(voxel_buffer),
                    .texture_id = mesh.textures[0]->image_id,
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
    for (size_t i = 0; i < node.size_x * node.size_y * node.size_z; ++i) {
        node.voxels[i].color.x = buffer_ptr[i].col.x;
        node.voxels[i].color.y = buffer_ptr[i].col.y;
        node.voxels[i].color.z = buffer_ptr[i].col.z;
        node.voxels[i].id = buffer_ptr[i].id;
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
