#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <gvox/gvox.h>

#include <chrono>
#include <iostream>

#include "noise.hpp"
#include "print.h"

constexpr auto noise_conf = FractalNoiseConfig{
    .amplitude = 1.0f,
    .persistance = 0.5f,
    .scale = 2.0f,
    .lacunarity = 2.0f,
    .octaves = 4,
};

auto sample_terrain(float x, float y, float z) -> float {
    return fractal_noise(f32vec3{x, y, z}, noise_conf);
    // float r = (float)(rand() % 1000) * 0.001f;
    // return sinf(x * 10) * 0.8f + sinf(y * 10) * 0.9f - z * 16.0f + 3.0f + r * 0.5f;
}

static const size_t sx = 8;
static const size_t sy = 8;
static const size_t sz = 8;

auto sample_terrain_i(int xi, int yi, int zi) -> float {
    float const x = ((float)xi) * (1.0f / (float)sx);
    float const y = ((float)yi) * (1.0f / (float)sy);
    float const z = ((float)zi) * (1.0f / (float)sz);
    return sample_terrain(x, y, z);
}

auto create_test_scene() -> GVoxScene {
    GVoxScene scene;
    scene.node_n = 1;
    scene.nodes = (GVoxSceneNode *)std::malloc(sizeof(GVoxSceneNode) * scene.node_n);
    scene.nodes[0].size_x = sx;
    scene.nodes[0].size_y = sy;
    scene.nodes[0].size_z = sz;
    size_t const voxel_n = scene.nodes[0].size_x * scene.nodes[0].size_y * scene.nodes[0].size_z;
    scene.nodes[0].voxels = (GVoxVoxel *)std::malloc(sizeof(GVoxVoxel) * voxel_n);

    for (size_t zi = 0; zi < sz; ++zi) {
        for (size_t yi = 0; yi < sy; ++yi) {
            for (size_t xi = 0; xi < sx; ++xi) {
                size_t const i = xi + yi * sx + zi * sx * sy;
                GVoxVoxel result = {.color = {0.0f, 0.0f, 0.0f}, .id = 0};
                scene.nodes[0].voxels[i] = result;
                float const val = sample_terrain_i(static_cast<int32_t>(xi), static_cast<int32_t>(yi), static_cast<int32_t>(zi));
                if (val > -0.0f) {
                    result.color.x = 0.33f;
                    result.color.y = 0.32f;
                    result.color.z = 0.30f;
                    result.id = 1;
                }

                // result.id = rand() % 1000;
                // float r = (float)(result.id) * 0.001f;
                // result.color.x = r;
                // result.color.y = r;
                // result.color.z = r;

                scene.nodes[0].voxels[i] = result;
            }
        }
    }
    for (size_t zi = 0; zi < sz; ++zi) {
        for (size_t yi = 0; yi < sy; ++yi) {
            for (size_t xi = 0; xi < sx; ++xi) {
                size_t const i = xi + yi * sx + zi * sx * sy;
                GVoxVoxel result = scene.nodes[0].voxels[i];
                if (result.id == 1) {
                    int si = 0;
                    for (si = 0; si < 6; ++si) {
                        float const val = sample_terrain_i(static_cast<int32_t>(xi), static_cast<int32_t>(yi), static_cast<int32_t>(zi) + 1 + si);
                        if (val < -0.0f) {
                            break;
                        }
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
                    } else {
                        float const r = (float)(rand() % 1000) * 0.001f;
                        if (r < 0.5f) {
                            result.color.x = 0.30f;
                            result.color.y = 0.29f;
                            result.color.z = 0.28f;
                            result.id = 4;
                        }
                    }
                    scene.nodes[0].voxels[i] = result;
                }
            }
        }
    }
    return scene;
}

using Clock = std::chrono::high_resolution_clock;

struct Timer {
    Clock::time_point start = Clock::now();

    ~Timer() {
        auto now = Clock::now();
        std::cout << "elapsed: " << std::chrono::duration<float>(now - start).count() << std::endl;
    }
};

auto main() -> int {
    GVoxContext *gvox = gvox_create_context();
    // gvox_load_format(gvox, "gvox_simple_rs");

    GVoxScene scene = create_test_scene();

    // scene = gvox_load_raw(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_simple_rs");

    // {
    //     Timer timer{};
    //     gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_simple.gvox", "gvox_simple");
    //     std::cout << "gvox_simple      | ";
    // }
    // {
    //     Timer timer{};
    //     gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
    //     std::cout << "gvox_u32         | ";
    // }
    {
        Timer const timer{};

        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_u32_palette");
        while (gvox_get_result(gvox) != GVOX_SUCCESS) {
            size_t msg_size = 0;
            gvox_get_result_message(gvox, nullptr, &msg_size);
            std::string msg;
            msg.resize(msg_size);
            gvox_get_result_message(gvox, nullptr, &msg_size);
            gvox_pop_result(gvox);
            return -1;
        }

        std::cout << "gvox_u32_palette | ";
    }
    // {
    //     Timer timer{};
    //     gvox_save_raw(gvox, scene, "tests/simple/compare_scene0_magicavoxel.vox", "magicavoxel");
    //     std::cout << "magicavoxel      | ";
    // }

    printf("generated scene content:\n");
    print_voxels(scene);
    gvox_destroy_scene(scene);

    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
    printf("loaded scene content:\n");
    print_voxels(scene);
    gvox_destroy_scene(scene);

    // scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
    // gvox_destroy_scene(scene);

    gvox_destroy_context(gvox);
}
