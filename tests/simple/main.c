#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gvox/gvox.h>

void print_voxels(GVoxScene scene) {
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (!scene.nodes[node_i].voxels)
            continue;
        // size_t const voxel_n = scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z;
        // float const inv_voxel_n = 1.0f / (float)voxel_n;

        // uint32_t min_val = 100000;
        // uint32_t max_val = 0;

        // for (size_t zi = 0; zi < scene.nodes[node_i].size_z; ++zi) {
        //     for (size_t yi = 0; yi < scene.nodes[node_i].size_y; ++yi) {
        //         for (size_t xi = 0; xi < scene.nodes[node_i].size_x; ++xi) {
        //             size_t i = xi + yi * scene.nodes[node_i].size_x + zi * scene.nodes[node_i].size_x * scene.nodes[node_i].size_y;
        //             GVoxVoxel vox = scene.nodes[node_i].voxels[i];
        //             min_val = min_val > vox.id ? vox.id : min_val;
        //             max_val = max_val < vox.id ? vox.id : max_val;
        //         }
        //     }
        // }

        // float scl = 1.0f / (float)(max_val - min_val);
        // float off = -(float)min_val;

        for (size_t zi = 0; zi < scene.nodes[node_i].size_z; ++zi) {
            for (size_t yi = 0; yi < scene.nodes[node_i].size_y; ++yi) {
                for (size_t xi = 0; xi < scene.nodes[node_i].size_x; ++xi) {
                    size_t i = xi + (scene.nodes[node_i].size_y - 1 - yi) * scene.nodes[node_i].size_x + (scene.nodes[node_i].size_z - 1 - zi) * scene.nodes[node_i].size_x * scene.nodes[node_i].size_y;
                    GVoxVoxel vox = scene.nodes[node_i].voxels[i];
                    // float brightness = (float)vox.id;
                    // brightness = (brightness + off) * scl;
                    float brightness = (vox.color.x + vox.color.y + vox.color.z) * 1.0f / 3.0f;
                    if (brightness < 0.0f)
                        brightness = 0.0f;
                    if (brightness > 1.0f)
                        brightness = 1.0f;
                    char c = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"[(size_t)(brightness * 69.0f)];
                    if (vox.id == 0)
                        c = '.';
                    fputc(c, stdout);
                }
                fputc('\n', stdout);
            }
            fputc('\n', stdout);
        }
    }
}

float sample_terrain(float x, float y, float z) {
    float r = (float)(rand() % 1000) * 0.001f;
    return sinf(x * 10) * 0.8f + sinf(y * 10) * 0.9f - z * 16.0f + 3.0f + r * 0.5f;
}

static const size_t sx = 64;
static const size_t sy = 64;
static const size_t sz = 64;

float sample_terrain_i(int xi, int yi, int zi) {
    float x = ((float)xi) * (1.0f / (float)sx);
    float y = ((float)yi) * (1.0f / (float)sy);
    float z = ((float)zi) * (1.0f / (float)sz);
    return sample_terrain(x, y, z);
}

GVoxScene create_test_scene() {
    GVoxScene scene;
    scene.node_n = 1;
    scene.nodes = malloc(sizeof(GVoxSceneNode) * scene.node_n);
    scene.nodes[0].size_x = sx;
    scene.nodes[0].size_y = sy;
    scene.nodes[0].size_z = sz;
    size_t const voxel_n = scene.nodes[0].size_x * scene.nodes[0].size_y * scene.nodes[0].size_z;
    scene.nodes[0].voxels = malloc(sizeof(GVoxVoxel) * voxel_n);
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

void test_manually_create_gvox_simple(GVoxContext *gvox) {
    GVoxScene scene = create_test_scene();
    // print_voxels(scene);
    gvox_save(gvox, scene, "tests/simple/test.gvox", "gvox_simple");
    gvox_destroy_scene(scene);
}

void test_load_gvox_simple(GVoxContext *gvox) {
    GVoxScene scene;
    scene = gvox_load(gvox, "tests/simple/test.gvox");
    // print_voxels(scene);
    gvox_destroy_scene(scene);
}

void test_load_magicavoxel(GVoxContext *gvox) {
    GVoxScene scene;
    scene = gvox_load_raw(gvox, "tests/simple/test.vox", "magicavoxel");
    // print_voxels(scene);
    gvox_destroy_scene(scene);
}

void test_save_magicavoxel(GVoxContext *gvox) {
    GVoxScene scene = create_test_scene();
    // print_voxels(scene);
    gvox_save_raw(gvox, scene, "tests/simple/test.vox", "magicavoxel");
    gvox_destroy_scene(scene);
}

int main(void) {
    GVoxContext *gvox = gvox_create_context();

    test_manually_create_gvox_simple(gvox);
    test_save_magicavoxel(gvox);
    // test_load_gvox_simple(gvox);
    // test_load_magicavoxel(gvox);

    gvox_destroy_context(gvox);
}
