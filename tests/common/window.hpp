#pragma once

#include <MiniFB.h>
#include <gvox/gvox.h>

struct Image {
    GvoxExtent2D extent{};
    std::vector<uint32_t> pixels = std::vector<uint32_t>(static_cast<size_t>(extent.data[0] * extent.data[1]));
};

inline void rect_opt(Image *image, int32_t x1, int32_t y1, int32_t width, int32_t height, uint32_t color) {
    if (width <= 0 || height <= 0) {
        return;
    }

    int32_t x2 = x1 + width - 1;
    int32_t y2 = y1 + height - 1;

    auto image_size_x = static_cast<int32_t>(image->extent.data[0]);
    auto image_size_y = static_cast<int32_t>(image->extent.data[1]);

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

    int32_t const clipped_width = x2 - x1 + 1;
    int32_t const next_row = image_size_x - clipped_width;
    uint32_t *pixel = image->pixels.data() + static_cast<ptrdiff_t>(y1) * image_size_x + x1;
    for (int y = y1; y <= y2; y++) {
        int32_t num_pixels = clipped_width;
        while (num_pixels-- != 0) {
            *pixel++ = color;
        }
        pixel += next_row;
    }
}
