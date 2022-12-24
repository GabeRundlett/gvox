#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <gvox/gvox.h>

#include <chrono>
#include <iostream>
#include <thread>

#include "print.h"
#include "scene.h"

#define TEST_ALL 1

using Clock = std::chrono::high_resolution_clock;

struct Timer {
    Clock::time_point start = Clock::now();

    Timer() = default;
    Timer(Timer const &) = default;
    Timer(Timer &&) = default;
    Timer& operator=(Timer const &) = default;
    Timer& operator=(Timer &&) = default;

    ~Timer() {
        auto now = Clock::now();
        std::cout << "elapsed: " << std::chrono::duration<float>(now - start).count() << std::endl;
    }
};

auto main() -> int {
    GVoxContext *gvox = gvox_create_context();

#if TEST_ALL
    GVoxScene const scene = create_scene(256, 256, 256);
    GVoxScene loaded_scene;
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_raw.gvox", "gvox_raw");
        std::cout << "save gvox_raw         | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_raw.gvox");
        std::cout << "load gvox_raw         | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
        std::cout << "save gvox_u32         | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32.gvox");
        std::cout << "load gvox_u32         | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_u32_palette");
        std::cout << "save gvox_u32_palette | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
        std::cout << "load gvox_u32_palette | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_zlib.gvox", "zlib");
        std::cout << "save zlib             | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_zlib.gvox");
        std::cout << "load zlib             | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_magicavoxel.gvox", "magicavoxel");
        std::cout << "save magicavoxel      | " << std::flush;
    }
    {
        Timer const timer{};
        loaded_scene = gvox_load(gvox, "tests/simple/compare_scene0_magicavoxel.gvox");
        std::cout << "load magicavoxel      | " << std::flush;
    }
    gvox_destroy_scene(loaded_scene);

    gvox_destroy_scene(scene);
#else
    GVoxScene scene = []() {
        Timer const timer{};
        auto const size = 8 * 64;
        return create_scene(size, size, size);
    }();
    {
        Timer const timer{};
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_u32_palette");
        // gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
        while (gvox_get_result(gvox) != GVOX_SUCCESS) {
            size_t msg_size = 0;
            gvox_get_result_message(gvox, nullptr, &msg_size);
            std::string msg;
            msg.resize(msg_size);
            gvox_get_result_message(gvox, nullptr, &msg_size);
            gvox_pop_result(gvox);
            gvox_destroy_scene(scene);
            return -1;
        }
    }
    // std::cout << "generated scene content:" << std::endl;
    // print_voxels(scene);
    gvox_destroy_scene(scene);
    // scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32.gvox");
    // std::cout << "\nloaded uncompressed scene content:" << std::endl;
    // print_voxels(scene);
    // gvox_destroy_scene(scene);
    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
    std::cout << "\nloaded compressed scene content:" << std::endl;
    // print_voxels(scene);
    gvox_destroy_scene(scene);
#endif

    gvox_destroy_context(gvox);
}
