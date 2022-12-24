#include <gvox/gvox.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#if GVOX_FORMAT_GVOX_RAW_BUILT_STATIC
#define EXPORT
#else
#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport)
#endif
#endif

struct GVoxRawContext {
    GVoxRawContext();
    ~GVoxRawContext() = default;

    static auto create_payload(GVoxScene scene) -> GVoxPayload;
    static void destroy_payload(GVoxPayload payload);
    static auto parse_payload(GVoxPayload payload) -> GVoxScene;
};

GVoxRawContext::GVoxRawContext() = default;

auto GVoxRawContext::create_payload(GVoxScene scene) -> GVoxPayload {
    GVoxPayload result = {};
    result.size += sizeof(size_t);
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels == nullptr) {
            continue;
        }
        result.size += sizeof(size_t) * 3;
        result.size += scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z * sizeof(GVoxVoxel);
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
        size_t const voxels_size = scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z * sizeof(GVoxVoxel);
        std::memcpy(buffer_ptr, scene.nodes[node_i].voxels, voxels_size);
        buffer_ptr += voxels_size;
    }
    return result;
}

void GVoxRawContext::destroy_payload(GVoxPayload payload) {
    delete[] payload.data;
}

auto GVoxRawContext::parse_payload(GVoxPayload payload) -> GVoxScene {
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
        size_t const voxels_size = result.nodes[node_i].size_x * result.nodes[node_i].size_y * result.nodes[node_i].size_z * sizeof(GVoxVoxel);
        result.nodes[node_i].voxels = (GVoxVoxel *)std::malloc(voxels_size);
        std::memcpy(result.nodes[node_i].voxels, buffer_ptr, voxels_size);
        buffer_ptr += voxels_size;
        ++node_i;
    }
    return result;
}

extern "C" EXPORT auto gvox_format_gvox_raw_create_context() -> void * {
    auto *result = new GVoxRawContext{};
    return result;
}

extern "C" EXPORT void gvox_format_gvox_raw_destroy_context(void *context_ptr) {
    auto *self = reinterpret_cast<GVoxRawContext *>(context_ptr);
    delete self;
}

extern "C" EXPORT auto gvox_format_gvox_raw_create_payload(void *context_ptr, GVoxScene scene) -> GVoxPayload {
    auto *self = reinterpret_cast<GVoxRawContext *>(context_ptr);
    return self->create_payload(scene);
}

extern "C" EXPORT void gvox_format_gvox_raw_destroy_payload(void *context_ptr, GVoxPayload payload) {
    auto *self = reinterpret_cast<GVoxRawContext *>(context_ptr);
    self->destroy_payload(payload);
}

extern "C" EXPORT auto gvox_format_gvox_raw_parse_payload(void *context_ptr, GVoxPayload payload) -> GVoxScene {
    auto *self = reinterpret_cast<GVoxRawContext *>(context_ptr);
    return self->parse_payload(payload);
}
