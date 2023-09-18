

#include "gvox/format.h"
#include <gvox/gvox.h>

#include <gvox/containers/bounded_raw.h>

#include <iostream>
#include <array>
#include <chrono>

#include <MiniFB.h>

#define HANDLE_RES(x)        \
    if (x != GVOX_SUCCESS) { \
        return -1;           \
    }

#define MAKE_COLOR_RGBA(red, green, blue, alpha) (uint32_t)(((uint8_t)(alpha) << 24) | ((uint8_t)(blue) << 16) | ((uint8_t)(green) << 8) | (uint8_t)(red))

struct Image {
    GvoxExtent2D extent{};
    std::vector<uint32_t> pixels = std::vector<uint32_t>(extent.x * extent.y);
};

inline void rect_opt(Image *image, int32_t x1, int32_t y1, int32_t width, int32_t height, uint32_t color) {
    if (width <= 0 || height <= 0) {
        return;
    }

    int32_t x2 = x1 + width - 1;
    int32_t y2 = y1 + height - 1;

    auto image_size_x = static_cast<int32_t>(image->extent.x);
    auto image_size_y = static_cast<int32_t>(image->extent.y);

    if (x1 >= image_size_x || x2 < 0 ||
        y1 >= image_size_y || y2 < 0) {
        return;
    }

    if (x1 < 0) {
        x1 = 0;
    }
    if (y1 < 0) {
        y1 = 0;
    }
    if (x2 >= image_size_x) {
        x2 = image_size_x - 1;
    }
    if (y2 >= image_size_y) {
        y2 = image_size_y - 1;
    }

    int32_t clipped_width = x2 - x1 + 1;
    int32_t next_row = image_size_x - clipped_width;
    uint32_t *pixel = image->pixels.data() + static_cast<ptrdiff_t>(y1) * image_size_x + x1;
    for (int y = y1; y <= y2; y++) {
        int32_t num_pixels = clipped_width;
        while (num_pixels-- != 0) {
            *pixel++ = color;
        }
        pixel += next_row;
    }
}

auto main() -> int {
    auto *rgb_voxel_desc = GvoxVoxelDesc{};
    {
        auto const attribs = std::array{
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_ALBEDO,
                .format = GVOX_FORMAT_R8G8B8A8_SRGB,
            },
        };
        auto voxel_desc_info = GvoxVoxelDescCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
            .next = nullptr,
            .attribute_count = static_cast<uint32_t>(attribs.size()),
            .attributes = attribs.data(),
        };
        HANDLE_RES(gvox_create_voxel_desc(&voxel_desc_info, &rgb_voxel_desc))
    }

    auto image = Image{.extent = GvoxExtent2D{320, 240}};

    auto *raw_container = GvoxContainer{};
    {
        auto raw_container_conf = GvoxBoundedRawContainerConfig{
            .voxel_desc = rgb_voxel_desc,
            .extent = {2, &image.extent.x},
            .pre_allocated_buffer = image.pixels.data(),
        };

        auto cont_info = GvoxContainerCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,
            .next = nullptr,
            .description = {},
            .cb_args = {
                .struct_type = {}, // ?
                .next = nullptr,
                .config = &raw_container_conf,
            },
        };
        HANDLE_RES(gvox_get_standard_container_description("bounded_raw", &cont_info.description))

        HANDLE_RES(gvox_create_container(&cont_info, &raw_container))
    }

    uint64_t voxel_data{};
    GvoxOffset2D offset{};
    GvoxExtent2D extent{};
    auto fill_info = GvoxFillInfo{
        .struct_type = GVOX_STRUCT_TYPE_FILL_INFO,
        .next = nullptr,
        .src_data = &voxel_data,
        .src_desc = rgb_voxel_desc,
        .dst = raw_container,
        .range = {
            {2, &offset.x},
            {2, &extent.x},
        },
    };

    auto const N_ITER = uint64_t{20000};

    struct mfb_window *window = mfb_open("viewer", image.extent.x * 2, image.extent.y * 2);

    srand(0);

    voxel_data = MAKE_COLOR_RGBA(rand() % 255, rand() % 255, rand() % 255, 255);
    offset.x = rand() % 320;
    offset.y = rand() % 240;
    extent.x = std::min<uint64_t>(rand() % (320 / 5), 320 - offset.x);
    extent.y = std::min<uint64_t>(rand() % (240 / 5), 240 - offset.y);
    HANDLE_RES(gvox_fill(&fill_info));

    do {
        uint64_t voxel_n = 0;
        using Clock = std::chrono::high_resolution_clock;
        auto t0 = Clock::now();

        for (int i = 0; i < N_ITER; i++) {
            voxel_data = MAKE_COLOR_RGBA(rand() % 255, rand() % 255, rand() % 255, 255);
            offset.x = rand() % 320;
            offset.y = rand() % 240;
            extent.x = std::min<uint64_t>(rand() % (320 / 5), 320 - offset.x);
            extent.y = std::min<uint64_t>(rand() % (240 / 5), 240 - offset.y);
            voxel_n += extent.x * extent.y;
            // rect_opt(&image, offset.x, offset.y, extent.x, extent.y, voxel_data);
            HANDLE_RES(gvox_fill(&fill_info));
        }
        auto t1 = Clock::now();
        auto seconds = std::chrono::duration<double>(t1 - t0).count();
        auto voxel_size = (gvox_voxel_desc_size_in_bits(rgb_voxel_desc) + 7) / 8;
        std::cout << seconds << "s, aka about " << static_cast<double>(voxel_n) / 1'000'000'000.0 / seconds << " GVx/s, or about " << static_cast<double>(voxel_n * voxel_size / 1000) / 1'000'000.0 / seconds << " GB/s" << std::endl;

        if (mfb_update_ex(window, image.pixels.data(), image.extent.x, image.extent.y) < 0)
            break;
    } while (mfb_wait_sync(window));

    gvox_destroy_container(raw_container);
    gvox_destroy_voxel_desc(rgb_voxel_desc);
}
