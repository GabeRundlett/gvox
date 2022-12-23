#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include <gvox/gvox.h>

#include <chrono>
#include <iostream>

#include "print.h"
#include "scene.h"

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

    GVoxScene scene = create_scene(8, 8, 8);

    using namespace std::literals;
    std::this_thread::sleep_for(0.5s);

    {
        Timer const timer{};

        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_u32_palette");
        gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
        while (gvox_get_result(gvox) != GVOX_SUCCESS) {
            size_t msg_size = 0;
            gvox_get_result_message(gvox, nullptr, &msg_size);
            std::string msg;
            msg.resize(msg_size);
            gvox_get_result_message(gvox, nullptr, &msg_size);
            gvox_pop_result(gvox);
            return -1;
        }

        std::cout << "\ngvox_u32_palette | " << std::flush;
    }

    std::cout << "generated scene content:" << std::endl;
    print_voxels(scene);
    gvox_destroy_scene(scene);

    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32.gvox");
    std::cout << "\nloaded uncompressed scene content:" << std::endl;
    print_voxels(scene);
    gvox_destroy_scene(scene);

    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
    std::cout << "\nloaded compressed scene content:" << std::endl;
    print_voxels(scene);
    gvox_destroy_scene(scene);

    gvox_destroy_context(gvox);
    std::cout << std::flush;
    std::this_thread::sleep_for(0.5s);
}
