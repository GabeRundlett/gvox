#include "gvox_simple.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

GVoxPayload gvox_simple_create_payload(GVoxScene scene) {
    GVoxPayload result = {0};
    // printf("creating gvox_simple payload from the %zu nodes at %p\n", scene.node_n, (void *)scene.nodes);
    result.size += sizeof(size_t);
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (!scene.nodes[node_i].voxels)
            continue;
        result.size += sizeof(size_t) * 3;
        result.size += scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z * sizeof(GVoxVoxel);
    }
    result.data = malloc(result.size);
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

void gvox_simple_destroy_payload(GVoxPayload payload) {
    // printf("destroying gvox_simple payload at %p with size %zu\n", payload.data, payload.size);
    free(payload.data);
}

GVoxScene gvox_simple_parse_payload(GVoxPayload payload) {
    GVoxScene result = {0};
    // printf("parsing gvox_simple payload at %p with size %zu\n", payload.data, payload.size);
    uint8_t *buffer_ptr = (uint8_t *)payload.data;
    uint8_t *buffer_sentinel = (uint8_t *)payload.data + payload.size;
    result.node_n = *(size_t *)buffer_ptr;
    buffer_ptr += sizeof(result.node_n);
    result.nodes = malloc(sizeof(GVoxSceneNode) * result.node_n);
    size_t node_i = 0;
    while (buffer_ptr < buffer_sentinel) {
        result.nodes[node_i].size_x = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_x);
        result.nodes[node_i].size_y = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_y);
        result.nodes[node_i].size_z = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_z);
        size_t voxels_size = result.nodes[node_i].size_x * result.nodes[node_i].size_y * result.nodes[node_i].size_z * sizeof(GVoxVoxel);
        result.nodes[node_i].voxels = malloc(voxels_size);
        memcpy(result.nodes[node_i].voxels, buffer_ptr, voxels_size);
        buffer_ptr += voxels_size;
        ++node_i;
    }
    return result;
}
