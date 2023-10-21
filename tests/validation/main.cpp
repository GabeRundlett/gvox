#include "gvox/core.h"
#include <gvox/gvox.h>
#include <gvox/stream.h>
#include <gvox/format.h>

#include <array>
#include <iostream>

#define HANDLE_RES(x, message)                    \
    {                                             \
        auto actual_result = static_cast<int>(x); \
        if (actual_result < 0) {                  \
            std::cerr << (message) << std::endl;  \
            return actual_result;                 \
        }                                         \
    }

namespace tests {
    auto voxel_conversion() -> int {
        auto *desc0 = GvoxVoxelDesc{};
        auto *desc1 = GvoxVoxelDesc{};
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
            HANDLE_RES(gvox_create_voxel_desc(&voxel_desc_info, &desc0), "Failed to create voxel desc0")
            HANDLE_RES(gvox_create_voxel_desc(&voxel_desc_info, &desc1), "Failed to create voxel desc1")
        }

        uint32_t voxel0 = 0xff00ffff;
        uint32_t voxel1{};

        voxel1 = {};
        HANDLE_RES(gvox_translate_voxel(&voxel0, desc0, &voxel1, desc0), "Failed to translate voxel with same description (0)")
        HANDLE_RES(voxel1 == 0xff00ffff ? 0 : -1, "Bad conversion (0)")
        voxel1 = {};
        HANDLE_RES(gvox_translate_voxel(&voxel0, desc0, &voxel1, desc1), "Failed to translate voxel with same description (1)")
        HANDLE_RES(voxel1 == 0xff00ffff ? 0 : -1, "Bad conversion (1)")

        //

        return 0;
    }
} // namespace tests

auto main() -> int {
    HANDLE_RES(tests::voxel_conversion(), "Failed the voxel conversion test")

    std::cout << "Success!" << std::endl;
}
