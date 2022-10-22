#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gvox/gvox.h>

float sample_terrain(float x, float y, float z) {
    float r = (float)(rand() % 1000) * 0.001f;
    return sinf(x * 10) * 0.8f + sinf(y * 10) * 0.9f - z * 16.0f + 3.0f + r * 0.5f;
}

static const size_t sx = 128;
static const size_t sy = 128;
static const size_t sz = 128;

float sample_terrain_i(int xi, int yi, int zi) {
    float x = ((float)xi) * (1.0f / (float)sx);
    float y = ((float)yi) * (1.0f / (float)sy);
    float z = ((float)zi) * (1.0f / (float)sz);
    return sample_terrain(x, y, z);
}

GVoxScene create_test_scene() {
    GVoxScene scene;
    scene.node_n = 1;
    scene.nodes = (GVoxSceneNode *)malloc(sizeof(GVoxSceneNode) * scene.node_n);
    scene.nodes[0].size_x = sx;
    scene.nodes[0].size_y = sy;
    scene.nodes[0].size_z = sz;
    size_t const voxel_n = scene.nodes[0].size_x * scene.nodes[0].size_y * scene.nodes[0].size_z;
    scene.nodes[0].voxels = (GVoxVoxel *)malloc(sizeof(GVoxVoxel) * voxel_n);
    for (int zi = 0; zi < sz; ++zi) {
        for (int yi = 0; yi < sy; ++yi) {
            for (int xi = 0; xi < sx; ++xi) {
                size_t i = xi + yi * sx + zi * sx * sy;
                GVoxVoxel result = {.color = {0.0f, 0.0f, 0.0f}, .id = 0};
                scene.nodes[0].voxels[i] = result;
                float val = sample_terrain_i(xi, yi, zi);
                if (val > 0.0f) {
                    result.color.x = 0.3f;
                    result.color.y = 0.3f;
                    result.color.z = 0.3f;
                    result.id = 1;
                }
                scene.nodes[0].voxels[i] = result;
            }
        }
    }
    for (int zi = 0; zi < sz; ++zi) {
        for (int yi = 0; yi < sy; ++yi) {
            for (int xi = 0; xi < sx; ++xi) {
                size_t i = xi + yi * sx + zi * sx * sy;
                GVoxVoxel result = scene.nodes[0].voxels[i];
                if (result.id == 1) {
                    int si = 0;
                    for (si = 0; si < 6; ++si) {
                        float val = sample_terrain_i(xi, yi, zi + 1 + si);
                        if (val < 0.0f)
                            break;
                    }
                    if (si < 2) {
                        result.color.x = 0.3f;
                        result.color.y = 0.7f;
                        result.color.z = 0.1f;
                        result.id = 2;
                    } else if (si < 4) {
                        result.color.x = 0.6f;
                        result.color.y = 0.4f;
                        result.color.z = 0.1f;
                        result.id = 3;
                    }

                    scene.nodes[0].voxels[i] = result;
                }
            }
        }
    }
    return scene;
}

int main() {
    GVoxContext *gvox = gvox_create_context();

    GVoxScene scene = create_test_scene();
    gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_simple.gvox", "gvox_simple");
    gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
    gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32_delta.gvox", "gvox_u32_delta");
    gvox_save_raw(gvox, scene, "tests/simple/compare_scene0_magicavoxel.vox", "magicavoxel");
    gvox_destroy_scene(scene);

    gvox_destroy_context(gvox);
}
