#include "gvox/format.h"
#include <gvox/gvox.h>

#include <gvox/containers/raw.h>

#include <daxa/daxa.hpp>

#define HANDLE_RES(x)        \
    if (x != GVOX_SUCCESS) { \
        return -1;           \
    }

auto main() -> int {
    auto *rgb_voxel_desc = GvoxVoxelDesc{};
    {
        auto const attribs = std::array{
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_ALBEDO,
                .format = GVOX_FORMAT_R8G8B8_SRGB,
            },
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_NORMAL,
                .format = GVOX_FORMAT_R12G12B12_OCT_UNIT,
            },
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_UNKNOWN,
                .format = GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_LINEAR, 3, 8, 12, 4, 0),
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

    auto *raw_container = GvoxContainer{};
    {
        auto raw_container_conf = GvoxRawContainerConfig{
            .voxel_desc = rgb_voxel_desc,
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
        HANDLE_RES(gvox_get_standard_container_description("raw", &cont_info.description))

        HANDLE_RES(gvox_create_container(&cont_info, &raw_container))
    }

    {
        uint32_t voxel_data = 0x00ff00ff;
        auto offset = GvoxOffset3D{0, 0, 0};
        auto extent = GvoxExtent3D{256, 256, 256};
        auto fill_info = GvoxFillInfo{
            .struct_type = GVOX_STRUCT_TYPE_FILL_INFO,
            .next = nullptr,
            .dst = raw_container,
            .range = {
                {3, &offset.x},
                {3, &extent.x},
            },
            .data = &voxel_data,
            .data_voxel_desc = rgb_voxel_desc,
        };
        HANDLE_RES(gvox_fill(&fill_info));
    }

    {
        uint32_t size_x = 16;
        uint32_t size_y = 16;

        std::cout.fill('0');
        for (uint32_t yi = 0; yi < size_y; ++yi) {
            for (uint32_t xi = 0; xi < size_x; ++xi) {
                auto voxel_data = std::array<uint8_t, 3>{};
                // auto offset = GvoxOffset3D{static_cast<int32_t>(xi), static_cast<int32_t>(yi), 0};
                // auto sample_info = GvoxSampleInfo{
                //     .struct_type = GVOX_STRUCT_TYPE_SAMPLE_INFO,
                //     .next = nullptr,
                //     .src = raw_container,
                //     .src_channel_index = 0,
                //     .src_voxel_desc = rgb_voxel_desc,
                //     .offset = {3, &offset.x},
                //     .dst = &voxel_data,
                //     .dst_format = GVOX_FORMAT_R8G8B8_UNORM,
                // };
                // HANDLE_RES(gvox_sample(&sample_info));

                auto r = static_cast<uint32_t>(voxel_data[0]);
                auto g = static_cast<uint32_t>(voxel_data[1]);
                auto b = static_cast<uint32_t>(voxel_data[2]);
                // r = std::min(std::max(r, 0u), 255u);
                // g = std::min(std::max(g, 0u), 255u);
                // b = std::min(std::max(b, 0u), 255u);
                std::cout << "\033[48;2;" << std::setw(3) << r << ";" << std::setw(3) << g << ";" << std::setw(3) << b << "m  ";
            }
            std::cout << "\033[0m\n";
        }
        std::cout << "\033[0m" << std::flush;
    }

    gvox_destroy_container(raw_container);
    gvox_destroy_voxel_desc(rgb_voxel_desc);
}
