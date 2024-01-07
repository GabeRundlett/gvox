
#include <gvox/stream.h>
#include <gvox/format.h>
#include <gvox/gvox.h>

#include <gvox/containers/raw.h>
#include <gvox/streams/input/file.h>
#include <gvox/streams/input/byte_buffer.h>

#include "../common/window.hpp"
#include "bvh.hpp"
#include "common/math.hpp"

#include <iostream>
#include <array>
#include <chrono>
#include <format>
#include <vector>
#include <string>
#include <cstdint>

#define HANDLE_RES(x, message)               \
    if ((x) != GVOX_SUCCESS) {               \
        std::cerr << (message) << std::endl; \
        return -1;                           \
    }

#define MAKE_COLOR_RGBA(red, green, blue, alpha) (uint32_t)(((uint8_t)(alpha) << 24) | ((uint8_t)(blue) << 16) | ((uint8_t)(green) << 8) | (uint8_t)(red))

template <GvoxOffsetOrExtentType T>
struct std::formatter<T> : std::formatter<std::string> {
    auto format(T p, format_context &ctx) const {
        auto str = std::string{"("};
        if (p.axis_n > 0) {
            str += std::format("{}", p.axis[0]);
        }
        for (uint32_t i = 1; i < p.axis_n; ++i) {
            str += std::format(", {}", p.axis[i]);
        }
        str += ")";
        return formatter<string>::format(str, ctx);
    }
};

struct VoxelVolume {
    std::array<uint64_t, 64 * 64> bitmask;
    Vec3 bounds_min;
    Vec3 bounds_max;
};
void intersect_voxels(VoxelVolume const &voxels, Ray const &ray, IntersectionRecord &ir) {
    intersect_aabb(ray, ir, voxels.bounds_min, voxels.bounds_min);
}

auto main() -> int {
    auto image = Image{.extent = GvoxExtent2D{512, 512}};
    auto depth_image = Image{.extent = image.extent};

    auto tris = std::array<Triangle, 128>{};
    for (auto &tri : tris) {
        auto r0 = Vec3(fast_rand_float(), fast_rand_float(), fast_rand_float());
        auto r1 = Vec3(fast_rand_float(), fast_rand_float(), fast_rand_float());
        auto r2 = Vec3(fast_rand_float(), fast_rand_float(), fast_rand_float());
        tri[0] = r0 * 9 - Vec3(5);
        tri[1] = tri[0] + r1;
        tri[2] = tri[0] + r2;
    }
    Bvh bvh = build_bvh(tris);

    // auto voxels = VoxelVolume{};
    // voxels.bounds_min = Vec3(-1);
    // voxels.bounds_max = Vec3(+1);
    // for (uint32_t zi = 0; zi < 64; ++zi) {
    //     for (uint32_t yi = 0; yi < 64; ++yi) {
    //         auto &voxel = voxels.bitmask[yi + zi * 64];
    //         voxel = 0;
    //         for (uint32_t xi = 0; xi < 64; ++xi) {
    //             auto p = Vec3(static_cast<float>(xi) * 1.0f / 63.0f - 0.5f,
    //                           static_cast<float>(yi) * 1.0f / 63.0f - 0.5f,
    //                           static_cast<float>(zi) * 1.0f / 63.0f - 0.5f);
    //             if (mag_sq(p) < 1.0f) {
    //                 voxel |= (1 << xi);
    //             }
    //         }
    //     }
    // }

    struct mfb_window *window = mfb_open("viewer", static_cast<uint32_t>(image.extent.data[0] * 1), static_cast<uint32_t>(image.extent.data[1] * 1));

    using Clock = std::chrono::steady_clock;
    auto start = Clock::now();

    while (true) {
        auto t0 = Clock::now();
        auto t = std::chrono::duration<float>(t0 - start).count();

        auto camPos = Vec3(sin(t) * 0.5f, cos(t) * 0.5f, -18.0f);
        auto p0 = Vec3(-1, +1, -16.0f); // bottom-left
        auto p1 = Vec3(+1, +1, -16.0f); // bottom-right
        auto p2 = Vec3(-1, -1, -16.0f); // top-left
        auto ray =Ray{};

        rect_opt(&image, 0, 0, image.extent.data[0], image.extent.data[1], MAKE_COLOR_RGBA(0, 0, 0, 0));

        auto t1 = Clock::now();

        for (uint32_t yi = 0; yi < image.extent.data[1]; ++yi) {
            for (uint32_t xi = 0; xi < image.extent.data[0]; ++xi) {
                float uv_x = static_cast<float>(xi) / static_cast<float>(image.extent.data[0]);
                float uv_y = static_cast<float>(yi) / static_cast<float>(image.extent.data[1]);
                Vec3 pixelPos = p0 + (p1 - p0) * uv_x + (p2 - p0) * uv_y;
                ray.o = camPos;
                ray.d = normalize(pixelPos - ray.o);
                auto ir = IntersectionRecord{};
                ir.t = MAX_DEPTH;
                // color the image.
                // for (auto const &tri : tris) {
                //     intersect_tri(ray, ir, tri, intersection_record);
                // }
                intersect_bvh(bvh, tris, ray, ir, 0);
                // intersect_voxels(voxels, ray, ir);
                if (ir.t != MAX_DEPTH) {
                    // auto col = (ir.nrm * 0.5f + Vec3(0.5f)) * 250.0f;
                    auto col = Vec3(255);
                    set_opt(&image, xi, yi, MAKE_COLOR_RGBA(uint8_t(col.x), uint8_t(col.y), uint8_t(col.z), 0));
                }
            }
        }

        auto t2 = Clock::now();
        // auto total_elapsed_ms = std::chrono::duration<float, std::milli>(t2 - t0).count();
        // std::cout << total_elapsed_ms << "ms per frame. ";
        auto rendering_elapsed_ms = std::chrono::duration<float, std::milli>(t2 - t1).count();
        std::cout << rendering_elapsed_ms << "ms for rendering\r";

        if (mfb_update_ex(window, image.pixels.data(), static_cast<uint32_t>(image.extent.data[0]), static_cast<uint32_t>(image.extent.data[1])) < 0) {
            break;
        }
        if (!mfb_wait_sync(window)) {
            break;
        }
    }
    std::cout << std::endl;
}
