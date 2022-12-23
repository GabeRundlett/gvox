#ifndef TESTS_SIMPLE_SCENE_H
#define TESTS_SIMPLE_SCENE_H

#include <stdio.h>
#include <gvox/gvox.h>

float sample_terrain(float x, float y, float z) {
    float r = (float)(rand() % 1000) * 0.001f;
    return sinf(x * 10) * 0.8f + sinf(y * 10) * 0.9f - z * 16.0f + 3.0f + r * 0.5f;
}

float sample_terrain_i(int xi, int yi, int zi, size_t sx, size_t sy, size_t sz) {
    float const x = ((float)xi) * (1.0f / (float)sx);
    float const y = ((float)yi) * (1.0f / (float)sy);
    float const z = ((float)zi) * (1.0f / (float)sz);
    return sample_terrain(x, y, z);
}

GVoxScene create_scene(size_t sx, size_t sy, size_t sz) {
    GVoxScene scene;
    scene.node_n = 1;
    scene.nodes = (GVoxSceneNode *)malloc(sizeof(GVoxSceneNode) * scene.node_n);
    scene.nodes[0].size_x = sx;
    scene.nodes[0].size_y = sy;
    scene.nodes[0].size_z = sz;
    size_t const voxel_n = scene.nodes[0].size_x * scene.nodes[0].size_y * scene.nodes[0].size_z;
    scene.nodes[0].voxels = (GVoxVoxel *)malloc(sizeof(GVoxVoxel) * voxel_n);

    for (size_t zi = 0; zi < sz; ++zi) {
        for (size_t yi = 0; yi < sy; ++yi) {
            for (size_t xi = 0; xi < sx; ++xi) {
                size_t const i = xi + yi * sx + zi * sx * sy;
                GVoxVoxel result = {.color = {0.0f, 0.0f, 0.0f}, .id = 0};
                scene.nodes[0].voxels[i] = result;
                float const val = sample_terrain_i((int32_t)xi, (int32_t)yi, (int32_t)zi, sx, sy, sz);
                if (val > -0.0f) {
                    // result.color.x = 0.33f;
                    // result.color.y = 0.32f;
                    // result.color.z = 0.30f;
                    result.color.x = 1.f;
                    result.color.y = 1.f;
                    result.color.z = 1.f;
                    result.id = 1;
                }
                scene.nodes[0].voxels[i] = result;
            }
        }
    }
    // for (size_t zi = 0; zi < sz; ++zi) {
    //     for (size_t yi = 0; yi < sy; ++yi) {
    //         for (size_t xi = 0; xi < sx; ++xi) {
    //             size_t const i = xi + yi * sx + zi * sx * sy;
    //             GVoxVoxel result = scene.nodes[0].voxels[i];
    //             if (result.id == 1) {
    //                 int si = 0;
    //                 for (si = 0; si < 6; ++si) {
    //                     float const val = sample_terrain_i((int32_t)xi, (int32_t)yi, (int32_t)zi, sx, sy, sz);
    //                     if (val < -0.0f) {
    //                         break;
    //                     }
    //                 }
    //                 if (si < 2) {
    //                     result.color.x = 0.3f;
    //                     result.color.y = 0.7f;
    //                     result.color.z = 0.1f;
    //                     result.id = 2;
    //                 } else if (si < 4) {
    //                     result.color.x = 0.6f;
    //                     result.color.y = 0.4f;
    //                     result.color.z = 0.1f;
    //                     result.id = 3;
    //                 } else {
    //                     float const r = (float)(rand() % 1000) * 0.001f;
    //                     if (r < 0.5f) {
    //                         result.color.x = 0.30f;
    //                         result.color.y = 0.29f;
    //                         result.color.z = 0.28f;
    //                         result.id = 4;
    //                     }
    //                 }
    //                 scene.nodes[0].voxels[i] = result;
    //             }
    //         }
    //     }
    // }
    return scene;
}

#endif
