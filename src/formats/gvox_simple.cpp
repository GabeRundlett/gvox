#include <gvox/gvox.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport)
#endif

struct Context {
    Context();
    ~Context();

    GVoxPayload create_payload(GVoxScene scene);
    void destroy_payload(GVoxPayload payload);
    GVoxScene parse_payload(GVoxPayload payload);
};

Context::Context() {
}

Context::~Context() {
}

GVoxPayload Context::create_payload(GVoxScene scene) {
    GVoxPayload result = {};
    result.size += sizeof(size_t);
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (!scene.nodes[node_i].voxels)
            continue;
        result.size += sizeof(size_t) * 3;
        result.size += scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z * sizeof(GVoxVoxel);
    }
    result.data = new uint8_t[result.size];
    uint8_t *buffer_ptr = (uint8_t *)result.data;
    memcpy(buffer_ptr, &scene.node_n, sizeof(scene.node_n));
    buffer_ptr += sizeof(scene.node_n);
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (!scene.nodes[node_i].voxels)
            continue;
        memcpy(buffer_ptr, &scene.nodes[node_i].size_x, sizeof(scene.nodes[node_i].size_x));
        buffer_ptr += sizeof(scene.nodes[node_i].size_x);
        memcpy(buffer_ptr, &scene.nodes[node_i].size_y, sizeof(scene.nodes[node_i].size_y));
        buffer_ptr += sizeof(scene.nodes[node_i].size_y);
        memcpy(buffer_ptr, &scene.nodes[node_i].size_z, sizeof(scene.nodes[node_i].size_z));
        buffer_ptr += sizeof(scene.nodes[node_i].size_z);
        size_t voxels_size = scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z * sizeof(GVoxVoxel);
        memcpy(buffer_ptr, scene.nodes[node_i].voxels, voxels_size);
        buffer_ptr += voxels_size;
    }
    return result;
}

void Context::destroy_payload(GVoxPayload payload) {
    delete[] payload.data;
}

GVoxScene Context::parse_payload(GVoxPayload payload) {
    GVoxScene result = {};
    uint8_t *buffer_ptr = (uint8_t *)payload.data;
    uint8_t *buffer_sentinel = (uint8_t *)payload.data + payload.size;
    result.node_n = *(size_t *)buffer_ptr;
    buffer_ptr += sizeof(result.node_n);
    result.nodes = (GVoxSceneNode *)malloc(sizeof(GVoxSceneNode) * result.node_n);
    size_t node_i = 0;
    while (buffer_ptr < buffer_sentinel) {
        result.nodes[node_i].size_x = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_x);
        result.nodes[node_i].size_y = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_y);
        result.nodes[node_i].size_z = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_z);
        size_t voxels_size = result.nodes[node_i].size_x * result.nodes[node_i].size_y * result.nodes[node_i].size_z * sizeof(GVoxVoxel);
        result.nodes[node_i].voxels = (GVoxVoxel *)malloc(voxels_size);
        memcpy(result.nodes[node_i].voxels, buffer_ptr, voxels_size);
        buffer_ptr += voxels_size;
        ++node_i;
    }
    return result;
}

extern "C" EXPORT void *gvox_format_create_context() {
    auto result = new Context{};
    return result;
}

extern "C" EXPORT void gvox_format_destroy_context(void *context_ptr) {
    auto self = reinterpret_cast<Context *>(context_ptr);
    delete self;
}

extern "C" EXPORT GVoxPayload gvox_format_create_payload(void *context_ptr, GVoxScene scene) {
    auto self = reinterpret_cast<Context *>(context_ptr);
    return self->create_payload(scene);
}

extern "C" EXPORT void gvox_format_destroy_payload(void *context_ptr, GVoxPayload payload) {
    auto self = reinterpret_cast<Context *>(context_ptr);
    self->destroy_payload(payload);
}

extern "C" EXPORT GVoxScene gvox_format_parse_payload(void *context_ptr, GVoxPayload payload) {
    auto self = reinterpret_cast<Context *>(context_ptr);
    return self->parse_payload(payload);
}
