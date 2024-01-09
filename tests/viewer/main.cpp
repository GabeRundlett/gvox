
#include <gvox/gvox.h>

#include <gvox/containers/raw.h>
#include <gvox/streams/input/file.h>
#include <gvox/streams/input/byte_buffer.h>

#include "../common/window.hpp"
#include "gvox/parsers/magicavoxel.h"

#include <iostream>
#include <array>
#include <chrono>
#include <format>
#include <filesystem>
#include <fstream>
#include <limits>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include <bit>
#include <cstddef>

#define HANDLE_RES(x, message)               \
    if ((x) != GVOX_SUCCESS) {               \
        std::cerr << (message) << std::endl; \
        return -1;                           \
    }

#define MAKE_COLOR_RGBA(red, green, blue, alpha) (uint32_t)(((uint8_t)(alpha) << 24) | ((uint8_t)(blue) << 16) | ((uint8_t)(green) << 8) | (uint8_t)(red))

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
                .type = GVOX_ATTRIBUTE_TYPE_ALBEDO_PACKED,
                .format = GVOX_STANDARD_FORMAT_B8G8R8_SRGB,
            },
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_ARBITRARY_INTEGER,
                .format = GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_RAW, GVOX_SINGLE_CHANNEL_BIT_COUNT(8)),
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

    auto *depth_voxel_desc = GvoxVoxelDesc{};
    {
        auto const attribs = std::array{
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_UNKNOWN,
                .format = GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_RAW, GVOX_SINGLE_CHANNEL_BIT_COUNT(16)),
            },
        };
        auto voxel_desc_info = GvoxVoxelDescCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
            .next = nullptr,
            .attribute_count = static_cast<uint32_t>(attribs.size()),
            .attributes = attribs.data(),
        };
        HANDLE_RES(gvox_create_voxel_desc(&voxel_desc_info, &depth_voxel_desc), "Failed to create voxel desc")
    }

    auto image = Image{.extent = GvoxExtent2D{1536, 1024}};
    auto depth_image = Image{.extent = image.extent};

    auto *raw_container = GvoxContainer{};
    {
        auto raw_container_conf = GvoxBoundedRaw3dContainerConfig{
            .voxel_desc = rgb_voxel_desc,
            // .extent = {2, image.extent.data},
            .extent = {image.extent.data[0], image.extent.data[1], 1},
            .pre_allocated_buffer = image.pixels.data(),
        };
        auto cont_info = GvoxContainerCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,
            .next = nullptr,
            .description = gvox_container_bounded_raw3d_description(),
            .cb_args = {
                .struct_type = {}, // ?
                .next = nullptr,
                .config = &raw_container_conf,
            },
        };
        HANDLE_RES(gvox_create_container(&cont_info, &raw_container), "Failed to create raw container")
    }

    auto *depth_container = GvoxContainer{};
    {
        auto raw_container_conf = GvoxBoundedRaw3dContainerConfig{
            .voxel_desc = depth_voxel_desc,
            // .extent = {2, depth_image.extent.data},
            .extent = {depth_image.extent.data[0], depth_image.extent.data[1], 1},
            .pre_allocated_buffer = depth_image.pixels.data(),
        };
        auto cont_info = GvoxContainerCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,
            .next = nullptr,
            .description = gvox_container_bounded_raw3d_description(),
            .cb_args = {
                .struct_type = {}, // ?
                .next = nullptr,
                .config = &raw_container_conf,
            },
        };
        HANDLE_RES(gvox_create_container(&cont_info, &depth_container), "Failed to create raw container")
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
            {2, offset.data},
            {2, extent.data},
        },
    };

    auto const FAR_DEPTH = std::numeric_limits<int16_t>::max();
    {
        fill_info.src_data = &FAR_DEPTH;
        fill_info.src_desc = depth_voxel_desc;
        fill_info.dst = depth_container;
        offset = {0, 0};
        extent = depth_image.extent;
        HANDLE_RES(gvox_fill(&fill_info), "Failed to do fill");
    }

    using Clock = std::chrono::high_resolution_clock;
    auto *file_input = GvoxInputStream{};

    {
        auto t0 = Clock::now();

        auto const input_mode = 1;
        switch (input_mode) {
        case 0: {
            auto config = GvoxFileInputStreamConfig{};
            config.filepath = "assets/nuke.vox";
            auto input_ci = GvoxInputStreamCreateInfo{};
            input_ci.struct_type = GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_INFO;
            input_ci.next = nullptr;
            input_ci.cb_args.config = &config;
            input_ci.description = gvox_input_stream_file_description();
            HANDLE_RES(gvox_create_input_stream(&input_ci, &file_input), "Failed to create (file) input stream");
        } break;
        case 1: {
            auto file = std::ifstream{"assets/nuke.vox", std::ios::binary};
            auto size = std::filesystem::file_size("assets/nuke.vox");
            auto bytes = std::vector<uint8_t>(size);
            file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size));
            auto config = GvoxByteBufferInputStreamConfig{.data = bytes.data(), .size = bytes.size()};
            auto input_ci = GvoxInputStreamCreateInfo{};
            input_ci.struct_type = GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_INFO;
            input_ci.next = nullptr;
            input_ci.cb_args.config = &config;
            input_ci.description = gvox_input_stream_byte_buffer_description();
            HANDLE_RES(gvox_create_input_stream(&input_ci, &file_input), "Failed to create (byte buffer) input stream");
        } break;
        }

        auto t1 = Clock::now();
        auto elapsed_s = std::chrono::duration<float>(t1 - t0).count();

        std::cout << elapsed_s << " s to open file" << '\n';
    }

    auto *file_parser = GvoxParser{};
    {
        auto parser_collection = GvoxParserDescriptionCollection{
            .struct_type = GVOX_STRUCT_TYPE_PARSER_DESCRIPTION_COLLECTION,
            .next = nullptr,
        };
        gvox_enumerate_standard_parser_descriptions(&parser_collection.descriptions, &parser_collection.description_n);
        HANDLE_RES(gvox_create_parser_from_input(&parser_collection, file_input, &file_parser), "Failed to create parser");
        // auto magicavoxel_config = MagicavoxelParserConfig{
        //     .object_name = "wardrobe",
        // };
        // auto parser_info = GvoxParserCreateInfo{
        //     .struct_type = GVOX_STRUCT_TYPE_PARSER_CREATE_INFO,
        //     .next = nullptr,
        //     .description = gvox_parser_magicavoxel_description(),
        //     .cb_args = {
        //         .struct_type = {}, // ?
        //         .next = nullptr,
        //         .input_stream = file_input,
        //         .config = &magicavoxel_config,
        //     },
        // };
        // HANDLE_RES(gvox_create_parser(&parser_info, &file_parser), "Failed to create parser");
    }

    for (uint32_t i = 0; i < 1; ++i) {
        gvox_input_seek(file_input, 0, GVOX_SEEK_ORIGIN_BEG);
        auto t0 = Clock::now();

        auto temp_offset = GvoxOffset2D{};
        auto temp_extent = GvoxExtent2D{.data = {1, 1}};
        auto temp_fill_info = fill_info;
        temp_fill_info.range = {
            {2, temp_offset.data},
            {2, temp_extent.data},
        };

        auto parse_iter_ci = GvoxParseIteratorCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_PARSE_ITERATOR_CREATE_INFO,
            .next = nullptr,
            .parser = file_parser,
        };
        auto iter_ci = GvoxIteratorCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_ITERATOR_CREATE_INFO,
            .next = &parse_iter_ci,
        };
        auto *input_iterator = GvoxIterator{};
        gvox_create_iterator(&iter_ci, &input_iterator);

        auto iter_value = GvoxIteratorValue{};
        size_t voxel_count = 0;
        auto sample = GvoxSample{
            .offset = {.axis_n = 0, .axis = nullptr},
            .dst_voxel_data = nullptr,
            .dst_voxel_desc = depth_voxel_desc,
        };
        auto sample_info = GvoxSampleInfo{
            .struct_type = GVOX_STRUCT_TYPE_SAMPLE_INFO,
            .next = nullptr,
            .src = nullptr,
            .samples = &sample,
            .sample_n = 1,
        };

        // uint32_t depth = 0;
        // auto indent = std::string{};

        auto advance_info = GvoxIteratorAdvanceInfo{
            .input_stream = file_input,
            .mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT,
        };

        while (true) {
            gvox_iterator_advance(input_iterator, &advance_info, &iter_value);

            // if (iter_value.tag == GVOX_ITERATOR_VALUE_TYPE_NODE_END) {
            //     depth -= 1;
            // }
            // indent = "";
            // for (uint32_t j = 0; j < depth; ++j) {
            //     indent += "  ";
            // }
            // if (iter_value.tag == GVOX_ITERATOR_VALUE_TYPE_NODE_BEGIN) {
            //     depth += 1;
            // }

            bool should_break = false;
            switch (iter_value.tag) {
            case GVOX_ITERATOR_VALUE_TYPE_NULL: {
                // std::cout << "NULL\n";
                should_break = true;
            } break;
            case GVOX_ITERATOR_VALUE_TYPE_LEAF: {
                // std::array<uint8_t, 4> const &col = *static_cast<std::array<uint8_t, 4> const *>(iter_value.voxel_data);
                // std::cout << std::format("{}VOXEL: pos={}, data={} (\033[48;2;{:0>3};{:0>3};{:0>3}m  \033[0m), desc={}\n",
                //                          indent, iter_value.range.offset, (void *)iter_value.voxel_data, static_cast<int>(col[0]), static_cast<int>(col[1]), static_cast<int>(col[2]), (void *)iter_value.voxel_desc);
                if (iter_value.range.offset.axis[0] > 0) {
                    continue;
                }
                offset.data[0] = ((iter_value.range.offset.axis[0] + 0) * 1 + 768);
                offset.data[1] = static_cast<int64_t>(image.extent.data[1]) - ((iter_value.range.offset.axis[2] + 1) * 1 + 0);
                extent.data[0] = iter_value.range.extent.axis[0] * 1;
                extent.data[1] = iter_value.range.extent.axis[2] * 1;

                auto current_depth = FAR_DEPTH;
                sample_info.src = depth_container;
                sample.offset = {.axis_n = 2, .axis = offset.data};
                sample.dst_voxel_data = &current_depth;

                HANDLE_RES(gvox_sample(&sample_info), "Failed to sample depth image");
                auto const &new_depth = iter_value.range.offset.axis[1];
                if (new_depth < current_depth) {
                    fill_info.src_data = iter_value.voxel_data;
                    fill_info.src_desc = iter_value.voxel_desc;
                    fill_info.dst = raw_container;
                    HANDLE_RES(gvox_fill(&fill_info), "Failed to do fill");

                    fill_info.src_data = &new_depth;
                    fill_info.src_desc = depth_voxel_desc;
                    fill_info.dst = depth_container;
                    HANDLE_RES(gvox_fill(&fill_info), "Failed to do fill");
                }

                // fill_info.src_data = iter_value.voxel_data;
                // fill_info.src_desc = iter_value.voxel_desc;
                // fill_info.dst = vdb_container;
                // HANDLE_RES(gvox_fill(&fill_info), "Failed to do fill");

                ++voxel_count;
                advance_info.mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT;
            } break;
            case GVOX_ITERATOR_VALUE_TYPE_NODE_BEGIN: {
                // std::cout << std::format("{}{{ offset={}, extent={}\n", indent, iter_value.range.offset, iter_value.range.extent);
                if (iter_value.range.offset.axis[0] > 0) {
                    advance_info.mode = GVOX_ITERATOR_ADVANCE_MODE_SKIP_BRANCH;
                } else {
                    advance_info.mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT;
                }
            } break;
            case GVOX_ITERATOR_VALUE_TYPE_NODE_END: {
                // std::cout << std::format("{}}} offset={}, extent={}\n", indent, iter_value.range.offset, iter_value.range.extent);
                advance_info.mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT;
            } break;
            }

            if (should_break) {
                break;
            }
        }

        gvox_destroy_iterator(input_iterator);
        auto t1 = Clock::now();
        auto elapsed_s = std::chrono::duration<float>(t1 - t0).count();

        std::cout << elapsed_s << " s (" << (static_cast<float>(voxel_count) / (elapsed_s * 1'000'000.0f)) << " MVx/s)" << '\n';
    }

    // return 0;

    fill_info.src_data = &voxel_data;
    fill_info.src_desc = rgb_voxel_desc;
    fill_info.range = {
        {2, offset.data},
        {2, extent.data},
    };

    auto const N_ITER = uint64_t{20000};
    bool const test_rect_speed = false;
    struct mfb_window *window = mfb_open("viewer", static_cast<uint32_t>(image.extent.data[0] * 1), static_cast<uint32_t>(image.extent.data[1] * 1));
    while (true) {
        if (test_rect_speed) {
            uint64_t voxel_n = 0;
            auto t0 = Clock::now();
            for (uint64_t i = 0; i < N_ITER; i++) {
                voxel_data = MAKE_COLOR_RGBA(fast_random() % 255, fast_random() % 255, fast_random() % 255, 255);
                offset.data[0] = fast_random() % static_cast<uint32_t>(image.extent.data[0]);
                offset.data[1] = fast_random() % static_cast<uint32_t>(image.extent.data[1]);
                extent.data[0] = std::min(static_cast<uint64_t>(fast_random() % (static_cast<uint32_t>(image.extent.data[0]) / 5)), image.extent.data[0] - static_cast<uint64_t>(offset.data[0]));
                extent.data[1] = std::min(static_cast<uint64_t>(fast_random() % (static_cast<uint32_t>(image.extent.data[1]) / 5)), image.extent.data[0] - static_cast<uint64_t>(offset.data[1]));
                voxel_n += extent.data[0] * extent.data[1];
                // rect_opt(&image, static_cast<int32_t>(offset.data[0]), static_cast<int32_t>(offset.data[1]), static_cast<int32_t>(extent.data[0]), static_cast<int32_t>(extent.data[1]), voxel_data);
                HANDLE_RES(gvox_fill(&fill_info), "Failed to do fill");
            }
            auto t1 = Clock::now();
            auto seconds = std::chrono::duration<double>(t1 - t0).count();
            auto voxel_size = (gvox_voxel_desc_size_in_bits(rgb_voxel_desc) + 7) / 8;
            std::cout << seconds << "s, aka about " << static_cast<double>(voxel_n) / 1'000'000'000.0 / seconds << " GVx/s, or about " << static_cast<double>(voxel_n * voxel_size / 1000) / 1'000'000.0 / seconds << " GB/s" << '\n';
        }
        if (mfb_update_ex(window, image.pixels.data(), static_cast<uint32_t>(image.extent.data[0]), static_cast<uint32_t>(image.extent.data[1])) < 0) {
            break;
        }
        if (!mfb_wait_sync(window)) {
            break;
        }
        // break;
    }

    gvox_destroy_input_stream(file_input);
    gvox_destroy_parser(file_parser);
    gvox_destroy_container(depth_container);
    gvox_destroy_container(raw_container);
    gvox_destroy_voxel_desc(depth_voxel_desc);
    gvox_destroy_voxel_desc(rgb_voxel_desc);
}
