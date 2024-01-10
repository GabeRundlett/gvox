#include <gvox/gvox.h>
#include <gvox/containers/raw.h>

#include <array>
#include <span>
#include <thread>
#include <iostream>
#include <cstdint>
#include <memory>
#include <chrono>

#include "../common/window.hpp"

#define HANDLE_RES(x, message)                                         \
    {                                                                  \
        auto res = (x);                                                \
        if (res != GVOX_SUCCESS) {                                     \
            std::cerr << "(" << res << ") " << (message) << std::endl; \
            return;                                                    \
        }                                                              \
    }

namespace gvox {
    using VoxelDesc = std::unique_ptr<GvoxVoxelDesc_ImplT, decltype(&gvox_destroy_voxel_desc)>;
    auto create_voxel_desc(std::span<GvoxAttribute const> const attribs) -> VoxelDesc {
        auto voxel_desc_info = GvoxVoxelDescCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
            .next = nullptr,
            .attribute_count = static_cast<uint32_t>(attribs.size()),
            .attributes = attribs.data(),
        };
        auto *result = GvoxVoxelDesc{};
        gvox_create_voxel_desc(&voxel_desc_info, &result);
        return VoxelDesc{result, &gvox_destroy_voxel_desc};
    }

    using Container = std::unique_ptr<GvoxContainer_ImplT, decltype(&gvox_destroy_container)>;
    auto create_container(GvoxContainerDescription const &description, auto &&container_conf) -> Container {
        auto cont_info = GvoxContainerCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,
            .next = nullptr,
            .description = description,
            .cb_args = {
                .struct_type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_CB_ARGS,
                .next = nullptr,
                .config = &container_conf,
            },
        };
        auto *result = GvoxContainer{};
        gvox_create_container(&cont_info, &result);
        return Container{result, &gvox_destroy_container};
    }
} // namespace gvox

void multi_thread_test() {
    auto image = Image{.extent = GvoxExtent2D{256, 256}};

    auto rgb_voxel_desc = gvox::create_voxel_desc(std::array{
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
    });
    auto const chunk_n = 8;
    // auto raw_container = gvox::create_container(gvox_container_raw3d_description(), GvoxRaw3dContainerConfig{.voxel_desc = rgb_voxel_desc.get()});
    auto chunk_data = std::vector<uint32_t>{};
    auto chunk_extent = GvoxExtent3D{chunk_n * 64, chunk_n * 64, chunk_n * 64};
    chunk_data.resize(chunk_extent.data[0] * chunk_extent.data[1] * chunk_extent.data[2]);
    auto raw_container = gvox::create_container(gvox_container_bounded_raw3d_description(), GvoxBoundedRaw3dContainerConfig{.voxel_desc = rgb_voxel_desc.get(), .extent = chunk_extent, .pre_allocated_buffer = chunk_data.data()});
    using Clock = std::chrono::high_resolution_clock;
    auto t0 = Clock::now();
    {
        struct ChunkgenContext {
            gvox::Container container = {nullptr, &gvox_destroy_container};
            GvoxOffset3D offset = {};
        };
        auto gen_voxel = [](GvoxOffset3D const &pos) -> uint32_t {
            auto r = uint32_t{0};
            auto g = uint32_t{0};
            auto b = uint32_t{0};
            auto x = static_cast<float>(pos.data[0] - 32);
            auto y = static_cast<float>(pos.data[1] - 32);
            r = static_cast<uint32_t>((sinf(x * 0.3f) * 0.5f + 0.5f) * 150.0f);
            g = static_cast<uint32_t>((sinf(y * 0.5f) * 0.5f + 0.5f) * 150.0f);
            b = static_cast<uint32_t>((sinf(sqrtf(x * x + y * y)) * 0.5f + 0.5f) * 250.0f);
            return (r << 0x10) | (g << 0x08) | (b << 0x00) | (255u << 0x18);
            // return 0;
        };
        auto chunkgen = [&rgb_voxel_desc, &gen_voxel, &chunk_data, &chunk_extent, &raw_container](ChunkgenContext &ctx) {
            // ctx.container = gvox::create_container(gvox_container_raw3d_description(), GvoxRaw3dContainerConfig{.voxel_desc = rgb_voxel_desc.get()});
            uint32_t voxel_data{0xff4080ff};
            auto offset = ctx.offset;
            auto extent = GvoxExtent3D{1, 1, 1};
            auto fill_info = GvoxFillInfo{
                .struct_type = GVOX_STRUCT_TYPE_FILL_INFO,
                .src_data = &voxel_data,
                .src_desc = rgb_voxel_desc.get(),
                .dst = raw_container.get(),
                // .dst = ctx.container.get(),
                .range = {{3, offset.data}, {3, extent.data}},
            };
            for (uint32_t zi = 0; zi < 64; ++zi) {
                for (uint32_t yi = 0; yi < 64; ++yi) {
                    for (uint32_t xi = 0; xi < 64; ++xi) {
                        offset.data[0] = ctx.offset.data[0] + xi;
                        offset.data[1] = ctx.offset.data[1] + yi;
                        offset.data[2] = ctx.offset.data[2] + zi;
                        voxel_data = gen_voxel(offset);
                        // HANDLE_RES(gvox_fill(&fill_info), "Failed to fill");
                        auto index = offset.data[0] + offset.data[1] * chunk_extent.data[0] + offset.data[2] * chunk_extent.data[0] * chunk_extent.data[1];
                        chunk_data[index] = voxel_data;
                    }
                }
            }
        };
        auto chunks = std::vector<ChunkgenContext>{};
        chunks.resize(chunk_n * chunk_n * chunk_n);
        for (uint32_t zi = 0; zi < chunk_n; ++zi) {
            for (uint32_t yi = 0; yi < chunk_n; ++yi) {
                for (uint32_t xi = 0; xi < chunk_n; ++xi) {
                    chunks[xi + yi * chunk_n + zi * chunk_n * chunk_n].offset = GvoxOffset3D{64 * xi, 64 * yi, 64 * zi};
                }
            }
        }
        bool multi_threaded = true && !GVOX_TEST_BUILT_FOR_WEB;
        if (multi_threaded) {
            auto chunks_per_thread = (chunks.size() + 15) / 16;
            auto thread_n = (chunks.size() + chunks_per_thread - 1) / chunks_per_thread;
            chunks_per_thread = (chunks.size() + thread_n - 1) / thread_n;
            auto threads = std::vector<std::thread>{};
            threads.reserve(thread_n);
            auto gen_span = [&chunkgen](std::span<ChunkgenContext> chunks_span) { for (auto &chunk : chunks_span) { chunkgen(chunk); } };
            for (uint32_t i = 0; i < thread_n - 1; ++i) {
                threads.emplace_back([=, &chunks]() { gen_span(std::span(chunks.begin() + i * chunks_per_thread, chunks.begin() + (i + 1) * chunks_per_thread)); });
            }
            gen_span(std::span(chunks.begin() + (thread_n - 1) * chunks_per_thread, chunks.end()));
            for (auto &thread : threads) {
                thread.join();
            }
        } else {
            for (auto &chunk : chunks) {
                chunkgen(chunk);
            }
        }
        // auto child_containers = std::vector<GvoxContainer>{};
        // auto ranges = std::vector<GvoxRange>{};
        // auto offsets = std::vector<GvoxOffset>{};
        // auto base_offset = GvoxOffset3D{0, 0, 0};
        // auto base_extent = GvoxExtent3D{64, 64, 64};
        // for (auto &chunk : chunks) {
        //     child_containers.push_back(chunk.container.get());
        //     ranges.push_back(GvoxRange{{3, chunk.offset.data}, {3, base_extent.data}});
        //     offsets.push_back({3, base_offset.data});
        // }
        // auto move_info = GvoxMoveInfo{
        //     .struct_type = GVOX_STRUCT_TYPE_MOVE_INFO,
        //     .src_containers = child_containers.data(),
        //     .src_container_ranges = ranges.data(),
        //     .src_dst_offsets = offsets.data(),
        //     .src_container_n = static_cast<uint32_t>(child_containers.size()),
        //     .dst = raw_container.get(),
        // };
        // HANDLE_RES(gvox_move(&move_info), "Failed to move");
    }
    auto t1 = Clock::now();
    auto elapsed_ns = std::chrono::duration<float, std::nano>(t1 - t0).count();
    auto elapsed_s = std::chrono::duration<float>(t1 - t0).count();
    auto voxel_n = chunk_n * chunk_n * chunk_n * 64 * 64 * 64;
    auto voxel_nf = static_cast<float>(voxel_n);
    std::cout << elapsed_s << "s total. " << voxel_n << " voxels.\n"
              << elapsed_ns / voxel_nf << "ns per voxel | " << voxel_nf / 1'000'000.0f / elapsed_s << " M voxels/s\n";

    for (uint32_t yi = 0; yi < image.extent.data[1]; ++yi) {
        for (uint32_t xi = 0; xi < image.extent.data[0]; ++xi) {
            auto voxel_data = uint32_t{0};
            auto offset = GvoxOffset3D{.data = {xi, yi, 0}};
            auto sample = GvoxSample{
                .offset = {.axis_n = 3, .axis = offset.data},
                .dst_voxel_data = &voxel_data,
                .dst_voxel_desc = rgb_voxel_desc.get(),
            };
            auto sample_info = GvoxSampleInfo{
                .struct_type = GVOX_STRUCT_TYPE_SAMPLE_INFO,
                .next = nullptr,
                .src = raw_container.get(),
                .samples = &sample,
                .sample_n = 1,
            };
            HANDLE_RES(gvox_sample(&sample_info), "Failed to sample");
            rect_opt(&image, static_cast<int32_t>(offset.data[0]), static_cast<int32_t>(offset.data[1]), 1, 1, voxel_data);
        }
    }

    struct mfb_window *window = mfb_open("viewer", static_cast<uint32_t>(image.extent.data[0] * 3), static_cast<uint32_t>(image.extent.data[1] * 3));
    while (true) {
        if (mfb_update_ex(window, image.pixels.data(), static_cast<uint32_t>(image.extent.data[0]), static_cast<uint32_t>(image.extent.data[1])) < 0) {
            break;
        }
        if (!mfb_wait_sync(window)) {
            break;
        }
    }
}

void iterator_test() {
    auto image = Image{.extent = GvoxExtent2D{256, 256}};

    auto rgb_voxel_desc = gvox::create_voxel_desc(std::array{
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
    });

    auto raw_container = gvox::create_container(gvox_container_raw3d_description(), GvoxRaw3dContainerConfig{.voxel_desc = rgb_voxel_desc.get()});

    // Construct the container from multiple containers via multi-threading
    {
        auto raw_container_chunk_a = gvox::create_container(gvox_container_raw3d_description(), GvoxRaw3dContainerConfig{.voxel_desc = rgb_voxel_desc.get()});
        auto raw_container_chunk_b = gvox::create_container(gvox_container_raw3d_description(), GvoxRaw3dContainerConfig{.voxel_desc = rgb_voxel_desc.get()});

        auto child_containers = std::array{
            raw_container_chunk_a.get(),
            raw_container_chunk_b.get(),
        };

        auto base_offset = GvoxOffset3D{0, 0, 0};
        auto base_extent = GvoxExtent3D{64, 64, 64};

        auto offset_a = GvoxOffset3D{0, 0, 0};
        auto offset_b = GvoxOffset3D{64, 0, 0};
        auto offsets = std::array{
            GvoxOffset{3, offset_a.data},
            GvoxOffset{3, offset_b.data},
        };
        auto ranges = std::array{
            GvoxRange{{3, base_offset.data}, {3, base_extent.data}},
            GvoxRange{{3, base_offset.data}, {3, base_extent.data}},
        };

        auto thread_a = std::thread([&]() {
            // process chunk A
            uint32_t voxel_data{0xff4080ff};
            auto offset = base_offset;
            auto extent = base_extent;
            auto fill_info = GvoxFillInfo{
                .struct_type = GVOX_STRUCT_TYPE_FILL_INFO,
                .next = nullptr,
                .src_data = &voxel_data,
                .src_desc = rgb_voxel_desc.get(),
                .dst = raw_container_chunk_a.get(),
                .range = {{3, offset.data}, {3, extent.data}},
            };
            HANDLE_RES(gvox_fill(&fill_info), "Failed to fill a");
        });
        auto thread_b = std::thread([&]() {
            // process chunk B
            uint32_t voxel_data{0xffff4090};
            auto offset = base_offset;
            auto extent = base_extent;
            auto fill_info = GvoxFillInfo{
                .struct_type = GVOX_STRUCT_TYPE_FILL_INFO,
                .next = nullptr,
                .src_data = &voxel_data,
                .src_desc = rgb_voxel_desc.get(),
                .dst = raw_container_chunk_b.get(),
                .range = {{3, offset.data}, {3, extent.data}},
            };
            for (uint32_t i = 0; extent.data[0] > 1; ++i) {
                offset.data[0] = i;
                extent.data[0] = 64 - i * 2;
                offset.data[1] = i;
                extent.data[1] = 64 - i * 2;
                voxel_data = (voxel_data & ~0x00ff00ffu) | ((i * 23) << 0) | ((i * 31) << 16);
                HANDLE_RES(gvox_fill(&fill_info), "Failed to fill b");
            }
        });

        thread_a.join();
        thread_b.join();

        auto move_info = GvoxMoveInfo{
            .struct_type = GVOX_STRUCT_TYPE_MOVE_INFO,
            .next = nullptr,
            .src_containers = child_containers.data(),
            .src_container_ranges = ranges.data(),
            .src_dst_offsets = offsets.data(),
            .src_container_n = child_containers.size(),
            .dst = raw_container.get(),
        };
        HANDLE_RES(gvox_move(&move_info), "Failed to move");
    }

    {
        using Clock = std::chrono::steady_clock;
        auto t0 = Clock::now();

        auto timer_iter_n = uint32_t{20};
        for (uint32_t timer_i = 0; timer_i < timer_iter_n; ++timer_i) {
            if (true) {
                auto cont_iter_ci = GvoxContainerIteratorCreateInfo{
                    .struct_type = GVOX_STRUCT_TYPE_CONTAINER_ITERATOR_CREATE_INFO,
                    .next = nullptr,
                    .container = raw_container.get(),
                };
                auto iter_ci = GvoxIteratorCreateInfo{
                    .struct_type = GVOX_STRUCT_TYPE_ITERATOR_CREATE_INFO,
                    .next = &cont_iter_ci,
                };
                auto *iterator = GvoxIterator{};
                gvox_create_iterator(&iter_ci, &iterator);

                auto iter_value = GvoxIteratorValue{};
                auto advance_info = GvoxIteratorAdvanceInfo{
                    .mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT,
                };

                while (true) {
                    gvox_iterator_advance(iterator, &advance_info, &iter_value);
                    bool should_break = false;
                    switch (iter_value.tag) {
                    case GVOX_ITERATOR_VALUE_TYPE_NULL: {
                        should_break = true;
                    } break;
                    case GVOX_ITERATOR_VALUE_TYPE_LEAF: {
                        auto xi = iter_value.range.offset.axis[0];
                        auto yi = iter_value.range.offset.axis[1];
                        auto zi = iter_value.range.offset.axis[2];

                        if (xi < image.extent.data[0] && yi < image.extent.data[1] && zi == 0) {
                            uint32_t voxel_data = *(uint32_t const *)iter_value.voxel_data;
                            rect_opt(&image, static_cast<int32_t>(xi), static_cast<int32_t>(yi), 1, 1, voxel_data);
                        }
                    } break;
                    case GVOX_ITERATOR_VALUE_TYPE_NODE_BEGIN: {
                        auto should_skip = false;
                        if (should_skip) {
                            advance_info.mode = GVOX_ITERATOR_ADVANCE_MODE_SKIP_BRANCH;
                        } else {
                            advance_info.mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT;
                        }
                    } break;
                    case GVOX_ITERATOR_VALUE_TYPE_NODE_END: {
                        advance_info.mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT;
                    } break;
                    }

                    if (should_break) {
                        break;
                    }
                }

                gvox_destroy_iterator(iterator);
            } else {
                for (uint32_t yi = 0; yi < image.extent.data[1]; ++yi) {
                    for (uint32_t xi = 0; xi < image.extent.data[0]; ++xi) {
                        auto voxel_data = uint32_t{0};
                        auto offset = GvoxOffset3D{.data = {xi, yi, 0}};
                        auto sample = GvoxSample{
                            .offset = {.axis_n = 3, .axis = offset.data},
                            .dst_voxel_data = &voxel_data,
                            .dst_voxel_desc = rgb_voxel_desc.get(),
                        };
                        auto sample_info = GvoxSampleInfo{
                            .struct_type = GVOX_STRUCT_TYPE_SAMPLE_INFO,
                            .next = nullptr,
                            .src = raw_container.get(),
                            .samples = &sample,
                            .sample_n = 1,
                        };
                        HANDLE_RES(gvox_sample(&sample_info), "Failed to sample");
                        rect_opt(&image, static_cast<int32_t>(offset.data[0]), static_cast<int32_t>(offset.data[1]), 1, 1, voxel_data);
                    }
                }
            }
        }
        auto t1 = Clock::now();
        auto elapsed_ms = std::chrono::duration<float, std::milli>(t1 - t0).count();

        std::cout << elapsed_ms / timer_iter_n << " ms to iterate container" << '\n';
    }

    // Try iterating over the container

    struct mfb_window *window = mfb_open("viewer", static_cast<uint32_t>(image.extent.data[0] * 3), static_cast<uint32_t>(image.extent.data[1] * 3));
    while (true) {
        if (mfb_update_ex(window, image.pixels.data(), static_cast<uint32_t>(image.extent.data[0]), static_cast<uint32_t>(image.extent.data[1])) < 0) {
            break;
        }
        if (!mfb_wait_sync(window)) {
            break;
        }
    }
}

auto main() -> int {
    multi_thread_test();
    // iterator_test();
}
