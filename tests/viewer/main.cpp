#include "gvox/core.h"
#include <gvox/stream.h>
#include <gvox/format.h>
#include <gvox/gvox.h>

#include <gvox/containers/bounded_raw.h>
#include <gvox/streams/input/file.h>
#include <gvox/parsers/magicavoxel.h>

#include <iostream>
#include <array>
#include <chrono>
#include <format>

#include <MiniFB.h>

#include <fstream>
#include <vector>
#include <gvox/streams/input/byte_buffer.h>

#define HANDLE_RES(x, message)             \
    if (x != GVOX_SUCCESS) {               \
        std::cerr << message << std::endl; \
        return -1;                         \
    }

#define MAKE_COLOR_RGBA(red, green, blue, alpha) (uint32_t)(((uint8_t)(alpha) << 24) | ((uint8_t)(blue) << 16) | ((uint8_t)(green) << 8) | (uint8_t)(red))

struct Image {
    GvoxExtent2D extent{};
    std::vector<uint32_t> pixels = std::vector<uint32_t>(static_cast<size_t>(extent.x * extent.y));
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

auto xorshf96() -> uint32_t {
    thread_local uint32_t x = 123456789;
    thread_local uint32_t y = 362436069;
    thread_local uint32_t z = 521288629;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    auto t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;
    return z;
}

auto fast_random() -> uint32_t {
    return xorshf96();
}

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
        HANDLE_RES(gvox_create_voxel_desc(&voxel_desc_info, &rgb_voxel_desc), "Failed to create voxel desc")
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
            .description = gvox_container_bounded_raw_description(),
            .cb_args = {
                .struct_type = {}, // ?
                .next = nullptr,
                .config = &raw_container_conf,
            },
        };

        HANDLE_RES(gvox_create_container(&cont_info, &raw_container), "Failed to create raw container")
    }

    uint32_t voxel_data{};
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

    auto file_input = GvoxInputStream{};
    auto const input_mode = 1;
    switch (input_mode) {
    case 0: {
        auto config = GvoxFileInputStreamConfig{};
        config.filepath = "assets/test2.vox";
        auto input_ci = GvoxInputStreamCreateInfo{};
        input_ci.struct_type = GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_INFO;
        input_ci.next = nullptr;
        input_ci.cb_args.config = &config;
        input_ci.description = gvox_input_stream_file_description();
        HANDLE_RES(gvox_create_input_stream(&input_ci, &file_input), "Failed to create (file) input stream");
    } break;
    case 1: {
        auto file = std::ifstream{"assets/test2.vox", std::ios::binary};
        auto bytes = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        auto config = GvoxByteBufferInputStreamConfig{.data = bytes.data(), .size = bytes.size()};
        auto input_ci = GvoxInputStreamCreateInfo{};
        input_ci.struct_type = GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_INFO;
        input_ci.next = nullptr;
        input_ci.cb_args.config = &config;
        input_ci.description = gvox_input_stream_byte_buffer_description();
        HANDLE_RES(gvox_create_input_stream(&input_ci, &file_input), "Failed to create (byte buffer) input stream");
    } break;
    }

    auto file_parser = GvoxParser{};
    {
        auto parser_collection = GvoxParserDescriptionCollection{
            .struct_type = GVOX_STRUCT_TYPE_PARSER_DESCRIPTION_COLLECTION,
            .next = nullptr,
        };
        gvox_enumerate_standard_parser_descriptions(&parser_collection.descriptions, &parser_collection.description_n);
        HANDLE_RES(gvox_create_parser_from_input(&parser_collection, file_input, &file_parser), "Failed to create parser");
    }

    {
        auto *input_iterator = GvoxIterator{};

        auto temp_offset = GvoxOffset2D{};
        auto temp_extent = GvoxExtent2D{.x = 1, .y = 1};
        auto temp_fill_info = fill_info;
        temp_fill_info.range = {
            {2, &temp_offset.x},
            {2, &temp_extent.x},
        };

        auto iter_ci = GvoxIteratorCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_ITERATOR_CREATE_INFO,
            .next = nullptr,
            .parser = file_parser,
            .iterator_type = GVOX_ITERATOR_TYPE_INPUT,
        };
        gvox_create_iterator(&iter_ci, &input_iterator);

        auto depth_image = Image{.extent = image.extent};
        auto iter_value = GvoxIteratorValue{};

        uint32_t depth = 0;

        auto indent = std::string{};
        while (true) {
            gvox_iterator_next(input_iterator, file_input, &iter_value);

            // Skip volume utility?

            if (iter_value.tag == GVOX_ITERATOR_VALUE_TYPE_NODE_END) {
                depth -= 1;
            }
            indent = "";
            for (uint32_t i = 0; i < depth; ++i) {
                indent += "  ";
            }
            if (iter_value.tag == GVOX_ITERATOR_VALUE_TYPE_NODE_BEGIN) {
                depth += 1;
            }

            bool should_break = false;
            switch (iter_value.tag) {
            case GVOX_ITERATOR_VALUE_TYPE_NULL: {
                std::cout << "NULL\n";
                should_break = true;
            } break;
            case GVOX_ITERATOR_VALUE_TYPE_LEAF: {
                std::array<uint8_t, 4> const &col = *static_cast<std::array<uint8_t, 4> const *>(iter_value.voxel_data);
                std::cout << std::format("{}VOXEL: pos={}, data={} (\033[48;2;{:0>3};{:0>3};{:0>3}m  \033[0m), desc={}\n",
                                         indent, iter_value.range.offset, (void *)iter_value.voxel_data, static_cast<int>(col[0]), static_cast<int>(col[1]), static_cast<int>(col[2]), (void *)iter_value.voxel_desc);
            } break;
            case GVOX_ITERATOR_VALUE_TYPE_NODE_BEGIN: {
                std::cout << std::format("{}ENTER_VOLUME: offset={}, extent={}\n",
                                         indent, iter_value.range.offset, iter_value.range.extent);
            } break;
            case GVOX_ITERATOR_VALUE_TYPE_NODE_END: {
                std::cout << std::format("{}LEAVE_VOLUME: offset={}, extent={}\n",
                                         indent, iter_value.range.offset, iter_value.range.extent);
            } break;
            }
            if (should_break) {
                break;
            }
        }

        gvox_destroy_iterator(input_iterator);
    }

    // auto const N_ITER = uint64_t{20000};
    // bool test_rect_speed = false;
    // struct mfb_window *window = mfb_open("viewer", static_cast<uint32_t>(image.extent.x * 3), static_cast<uint32_t>(image.extent.y * 3));
    // while (true) {
    //     if (test_rect_speed) {
    //         uint64_t voxel_n = 0;
    //         using Clock = std::chrono::high_resolution_clock;
    //         auto t0 = Clock::now();
    //         for (uint64_t i = 0; i < N_ITER; i++) {
    //             voxel_data = MAKE_COLOR_RGBA(fast_random() % 255, fast_random() % 255, fast_random() % 255, 255);
    //             offset.x = fast_random() % 320;
    //             offset.y = fast_random() % 240;
    //             extent.x = std::min(static_cast<uint64_t>(fast_random() % (320 / 5)), static_cast<uint64_t>(320 - offset.x));
    //             extent.y = std::min(static_cast<uint64_t>(fast_random() % (240 / 5)), static_cast<uint64_t>(240 - offset.y));
    //             voxel_n += extent.x * extent.y;
    //             // rect_opt(&image, static_cast<int32_t>(offset.x), static_cast<int32_t>(offset.y), static_cast<int32_t>(extent.x), static_cast<int32_t>(extent.y), voxel_data);
    //             HANDLE_RES(gvox_fill(&fill_info), "Failed to do fill");
    //         }
    //         auto t1 = Clock::now();
    //         auto seconds = std::chrono::duration<double>(t1 - t0).count();
    //         auto voxel_size = (gvox_voxel_desc_size_in_bits(rgb_voxel_desc) + 7) / 8;
    //         std::cout << seconds << "s, aka about " << static_cast<double>(voxel_n) / 1'000'000'000.0 / seconds << " GVx/s, or about " << static_cast<double>(voxel_n * voxel_size / 1000) / 1'000'000.0 / seconds << " GB/s" << std::endl;
    //     }
    //     if (mfb_update_ex(window, image.pixels.data(), static_cast<uint32_t>(image.extent.x), static_cast<uint32_t>(image.extent.y)) < 0) {
    //         break;
    //     }
    //     if (!mfb_wait_sync(window)) {
    //         break;
    //     }
    // }

    gvox_destroy_input_stream(file_input);
    gvox_destroy_parser(file_parser);
    gvox_destroy_container(raw_container);
    gvox_destroy_voxel_desc(rgb_voxel_desc);
}
