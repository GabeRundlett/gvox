#include <gvox/gvox.h>

#include <cstdint>
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
                    .type = GVOX_ATTRIBUTE_TYPE_ALBEDO_PACKED,
                    .format = GVOX_STANDARD_FORMAT_R8G8B8_SRGB,
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

        uint32_t voxel0 = 0x00ff00ff;
        uint32_t voxel1{};
        uint32_t voxel2{};

        voxel1 = {};
        auto mapping0 = GvoxAttributeMapping{.dst_index = 0, .src_index = 0};
        HANDLE_RES(gvox_translate_voxel(&voxel0, desc0, &voxel1, desc0, &mapping0, 1), "Failed to translate voxel with same description (0)")
        HANDLE_RES(voxel1 == 0x00ff00ff ? 0 : -1, "Bad conversion (0)")
        voxel1 = {};
        auto mapping1 = GvoxAttributeMapping{.dst_index = 0, .src_index = 0};
        HANDLE_RES(gvox_translate_voxel(&voxel0, desc0, &voxel1, desc1, &mapping1, 1), "Failed to translate voxel with same description (1)")
        HANDLE_RES(voxel1 == 0x00ff00ff ? 0 : -1, "Bad conversion (1)")

        auto *desc2 = GvoxVoxelDesc{};
        {
            auto const attribs = std::array{
                GvoxAttribute{
                    .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                    .next = nullptr,
                    .type = GVOX_ATTRIBUTE_TYPE_ARBITRARY_INTEGER,
                    .format = GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_RAW, GVOX_SINGLE_CHANNEL_BIT_COUNT(8)),
                },
                GvoxAttribute{
                    .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                    .next = nullptr,
                    .type = GVOX_ATTRIBUTE_TYPE_ALBEDO_PACKED,
                    .format = GVOX_STANDARD_FORMAT_R8G8B8_SRGB,
                },
            };
            auto voxel_desc_info = GvoxVoxelDescCreateInfo{
                .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
                .next = nullptr,
                .attribute_count = static_cast<uint32_t>(attribs.size()),
                .attributes = attribs.data(),
            };
            HANDLE_RES(gvox_create_voxel_desc(&voxel_desc_info, &desc2), "Failed to create voxel desc2")
        }

        voxel1 = {};
        auto mapping2a = GvoxAttributeMapping{.dst_index = 1, .src_index = 0};
        HANDLE_RES(gvox_translate_voxel(&voxel0, desc0, &voxel1, desc2, &mapping2a, 1), "Failed to translate voxels (2a)")
        HANDLE_RES(voxel1 == 0xff00ff00 ? 0 : -1, "Bad conversion (2a)")

        auto mapping2b = GvoxAttributeMapping{.dst_index = 0, .src_index = 1};
        HANDLE_RES(gvox_translate_voxel(&voxel1, desc2, &voxel2, desc0, &mapping2b, 1), "Failed to translate voxels (2b)")
        HANDLE_RES(voxel2 == voxel0 ? 0 : -1, "Bad conversion (2b)")

        gvox_destroy_voxel_desc(desc2);
        gvox_destroy_voxel_desc(desc1);
        gvox_destroy_voxel_desc(desc0);

        return 0;
    }
} // namespace tests

auto main() -> int {
    HANDLE_RES(tests::voxel_conversion(), "Failed the voxel conversion test")

    std::cout << "Success!" << '\n';
}
