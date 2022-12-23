#include <gvox/gvox.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>

#if GVOX_FORMAT_GVOX_U32_BUILT_STATIC
#define EXPORT
#else
#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport)
#endif
#endif

struct GVoxU32Context {
    GVoxU32Context();
    ~GVoxU32Context() = default;

    static auto create_payload(GVoxScene scene) -> GVoxPayload;
    static void destroy_payload(GVoxPayload payload);
    static auto parse_payload(GVoxPayload payload) -> GVoxScene;
};

GVoxU32Context::GVoxU32Context() = default;

auto GVoxU32Context::create_payload(GVoxScene scene) -> GVoxPayload {
    GVoxPayload result = {};
    result.size += sizeof(size_t);
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels == nullptr) {
            continue;
        }
        result.size += sizeof(size_t) * 3;
        result.size += scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z * sizeof(uint32_t);
    }
    result.data = new uint8_t[result.size];
    auto *buffer_ptr = (uint8_t *)result.data;
    std::memcpy(buffer_ptr, &scene.node_n, sizeof(scene.node_n));
    buffer_ptr += sizeof(scene.node_n);
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels == nullptr) {
            continue;
        }
        std::memcpy(buffer_ptr, &scene.nodes[node_i].size_x, sizeof(scene.nodes[node_i].size_x));
        buffer_ptr += sizeof(scene.nodes[node_i].size_x);
        std::memcpy(buffer_ptr, &scene.nodes[node_i].size_y, sizeof(scene.nodes[node_i].size_y));
        buffer_ptr += sizeof(scene.nodes[node_i].size_y);
        std::memcpy(buffer_ptr, &scene.nodes[node_i].size_z, sizeof(scene.nodes[node_i].size_z));
        buffer_ptr += sizeof(scene.nodes[node_i].size_z);
        size_t const voxels_n = scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z;
        size_t const voxels_size = voxels_n * sizeof(uint32_t);
        for (size_t voxel_i = 0; voxel_i < voxels_n; ++voxel_i) {
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
            std::memcpy(buffer_ptr + voxel_i * sizeof(uint32_t), &u32_voxel, sizeof(uint32_t));
        }
        buffer_ptr += voxels_size;
    }
    return result;
}

void GVoxU32Context::destroy_payload(GVoxPayload payload) {
    delete[] payload.data;
}

auto GVoxU32Context::parse_payload(GVoxPayload payload) -> GVoxScene {
    GVoxScene result = {};
    auto *buffer_ptr = (uint8_t *)payload.data;
    uint8_t *buffer_sentinel = (uint8_t *)payload.data + payload.size;
    result.node_n = *(size_t *)buffer_ptr;
    buffer_ptr += sizeof(result.node_n);
    result.nodes = (GVoxSceneNode *)std::malloc(sizeof(GVoxSceneNode) * result.node_n);
    size_t node_i = 0;
    while (buffer_ptr < buffer_sentinel) {
        result.nodes[node_i].size_x = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_x);
        result.nodes[node_i].size_y = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_y);
        result.nodes[node_i].size_z = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_z);
        size_t const voxels_n = result.nodes[node_i].size_x * result.nodes[node_i].size_y * result.nodes[node_i].size_z;
        size_t const voxels_size = voxels_n * sizeof(GVoxVoxel);
        result.nodes[node_i].voxels = (GVoxVoxel *)std::malloc(voxels_size);
        for (size_t voxel_i = 0; voxel_i < voxels_n; ++voxel_i) {
            uint32_t u32_voxel = 0;
            std::memcpy(&u32_voxel, buffer_ptr + voxel_i * sizeof(uint32_t), sizeof(uint32_t));
            float r = static_cast<float>((u32_voxel >> 0x00) & 0xff) / 255.0f;
            float g = static_cast<float>((u32_voxel >> 0x08) & 0xff) / 255.0f;
            float b = static_cast<float>((u32_voxel >> 0x10) & 0xff) / 255.0f;
            uint32_t const i = (u32_voxel >> 0x18) & 0xff;
            result.nodes[node_i].voxels[voxel_i] = GVoxVoxel{
                .color = {r, g, b},
                .id = i,
            };
        }
        buffer_ptr += voxels_size;
        ++node_i;
    }
    return result;
}

extern "C" EXPORT auto gvox_format_gvox_u32_create_context() -> void * {
    auto *result = new GVoxU32Context{};
    return result;
}

extern "C" EXPORT void gvox_format_gvox_u32_destroy_context(void *context_ptr) {
    auto *self = reinterpret_cast<GVoxU32Context *>(context_ptr);
    delete self;
}

extern "C" EXPORT auto gvox_format_gvox_u32_create_payload(void *context_ptr, GVoxScene scene) -> GVoxPayload {
    auto *self = reinterpret_cast<GVoxU32Context *>(context_ptr);
    return self->create_payload(scene);
}

extern "C" EXPORT void gvox_format_gvox_u32_destroy_payload(void *context_ptr, GVoxPayload payload) {
    auto *self = reinterpret_cast<GVoxU32Context *>(context_ptr);
    self->destroy_payload(payload);
}

extern "C" EXPORT auto gvox_format_gvox_u32_parse_payload(void *context_ptr, GVoxPayload payload) -> GVoxScene {
    auto *self = reinterpret_cast<GVoxU32Context *>(context_ptr);
    return self->parse_payload(payload);
}
