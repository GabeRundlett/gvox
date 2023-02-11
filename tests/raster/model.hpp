#pragma once

#include <daxa/daxa.hpp>
#include <daxa/utils/math_operators.hpp>
#include <daxa/utils/task_list.hpp>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <stb_image.h>
// #include <cassert>

using namespace daxa::types;
#include "gpu.inl"

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
    daxa::BufferId normal_buffer;
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

void process_node(daxa::Device device, Model &model, aiNode *node, aiScene const *scene, std::filesystem::path const &rootdir, daxa::f32mat4x4 const &parent_transform) {
    using namespace daxa::math_operators;
    auto transform = *reinterpret_cast<f32mat4x4 *>(&node->mTransformation);
    transform = daxa::math_operators::transpose(transform) * parent_transform;
    for (u32 mesh_i = 0; mesh_i < node->mNumMeshes; ++mesh_i) {
        aiMesh *aimesh = scene->mMeshes[node->mMeshes[mesh_i]];
        model.meshes.push_back({});
        auto &o_mesh = model.meshes.back();
        o_mesh.modl_mat = transform;
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
        o_mesh.normal_buffer = device.create_buffer(daxa::BufferInfo{
            .size = static_cast<u32>(sizeof(Vertex) * o_mesh.verts.size() / 3),
            .debug_name = "normal_buffer",
        });
    }

    for (u32 i = 0; i < node->mNumChildren; ++i) {
        process_node(device, model, node->mChildren[i], scene, rootdir, transform);
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
    process_node(device, model, scene->mRootNode, scene, filepath.parent_path(), {f32vec4{1, 0, 0, 0}, f32vec4{0, 0, 1, 0}, f32vec4{0, -1, 0, 0}, f32vec4{0, 0, 0, 1}});
    auto texture_staging_buffers = std::vector<daxa::BufferId>{};
    for (auto &[key, texture] : model.textures) {
        i32 size_x{}, size_y{};
        if (key == "#default_texture") {
            texture->pixels = reinterpret_cast<u8 *>(default_texture_pixels.data());
            texture->size_x = static_cast<u32>(16);
            texture->size_y = static_cast<u32>(16);
        } else {
            texture->pixels = stbi_load(texture->path.string().c_str(), &size_x, &size_y, &texture->channel_n, 4);
            // assert(texture->pixels != nullptr && "Failed to load image");
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
