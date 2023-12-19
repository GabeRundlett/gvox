#include "gvox/core.h"
#include "gvox/stream.h"
#include <gvox/gvox.h>
#include <gvox/containers/raw3d.h>

#include <array>
#include <span>
#include <thread>
#include <iostream>

#include "../common/window.hpp"

#define HANDLE_RES(x, message)                                         \
    {                                                                  \
        auto res = (x);                                                \
        if (res != GVOX_SUCCESS) {                                     \
            std::cerr << "(" << res << ") " << (message) << std::endl; \
            return -1;                                                 \
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

auto main() -> int {
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
