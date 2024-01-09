#include <gvox/gvox.h>
#include "types.hpp"
#include "utils/tracy.hpp"
#include "utils/handle.hpp"

#include <bit>
#include <algorithm>
#include <cstdint>
#include <cstring>

namespace {
    constexpr auto is_packed_multi_channel_attribute(GvoxAttributeType type) -> bool {
        return static_cast<uint32_t>(type) > static_cast<uint32_t>(GVOX_ATTRIBUTE_TYPE_UNKNOWN_PACKED);
    }

    // namespace float_conv {
    //     constexpr auto from_unorm(uint32_t bit_n, uint32_t data) -> float {
    //         uint32_t const mask = (1 << bit_n) - 1;
    //         return static_cast<float>(data & mask) / static_cast<float>(mask);
    //     }
    //     constexpr auto to_unorm(uint32_t bit_n, float data) -> uint32_t {
    //         uint32_t const mask = (1 << bit_n) - 1;
    //         return static_cast<uint32_t>(data * static_cast<float>(mask)) & mask;
    //     }
    //     constexpr auto from_snorm(uint32_t bit_n, uint32_t data) -> float {
    //         return from_unorm(bit_n, data) * 2.0f - 1.0f;
    //     }
    //     constexpr auto to_snorm(uint32_t bit_n, float data) -> uint32_t {
    //         return to_unorm(bit_n, data * 0.5f + 0.5f);
    //     }
    //     constexpr auto from_srgb(uint32_t bit_n, uint32_t data) -> float {
    //         if (std::is_constant_evaluated()) {
    //             return gcem::pow(from_unorm(bit_n, data), 2.2f);
    //         } else {
    //             return std::pow(from_unorm(bit_n, data), 2.2f);
    //         }
    //     }
    //     constexpr auto to_srgb(uint32_t bit_n, float data) -> uint32_t {
    //         if (std::is_constant_evaluated()) {
    //             return to_unorm(bit_n, gcem::pow(data, 2.2f));
    //         } else {
    //             return to_unorm(bit_n, std::pow(data, 2.2f));
    //         }
    //     }
    // } // namespace float_conv
} // namespace

auto gvox_create_voxel_desc(GvoxVoxelDescCreateInfo const *info, GvoxVoxelDesc *handle) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    HANDLE_NEW(VoxelDesc, VOXEL_DESC);

    if (info->struct_type != GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    auto &result = *reinterpret_cast<IMPL_STRUCT_NAME(VoxelDesc) *>(*handle);

    for (size_t i = 0; i < info->attribute_count; ++i) {
        auto const &in_attrib = info->attributes[i];
        if (in_attrib.struct_type != GVOX_STRUCT_TYPE_ATTRIBUTE) {
            return GVOX_ERROR_BAD_STRUCT_TYPE;
        }
        auto const format_desc = std::bit_cast<FormatDescriptor>(in_attrib.format);
        auto attribute_size = uint32_t{};
        if (is_packed_multi_channel_attribute(in_attrib.type)) {
            for (uint32_t packed_attrib_i = 0; packed_attrib_i < format_desc.packed.component_count + 1; ++packed_attrib_i) {
                switch (packed_attrib_i) {
                case 0: attribute_size += format_desc.packed.d0_bit_count + 1; break;
                case 1: attribute_size += format_desc.packed.d1_bit_count + 1; break;
                case 2: attribute_size += format_desc.packed.d2_bit_count + 1; break;
                default: break;
                }
            }
        } else {
            attribute_size = format_desc.single.bit_count + 1;
        }
        result.attributes.push_back({
            .bit_count = attribute_size,
            .bit_offset = result.bit_count, // currently accumulating the size and therefore is the offset!
            .type = in_attrib.type,
            .format_desc = format_desc,
        });
        result.bit_count += attribute_size;
    }

    return GVOX_SUCCESS;
}
void gvox_destroy_voxel_desc(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB {
    destroy_handle(handle);
}

auto gvox_voxel_desc_size_in_bits(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB->uint32_t {
    auto &voxel_desc = *reinterpret_cast<IMPL_STRUCT_NAME(VoxelDesc) *>(handle);
    return voxel_desc.bit_count;
}
auto gvox_voxel_desc_attribute_count(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB->uint32_t {
    auto &voxel_desc = *reinterpret_cast<IMPL_STRUCT_NAME(VoxelDesc) *>(handle);
    return static_cast<uint32_t>(voxel_desc.attributes.size());
}

auto gvox_voxel_desc_compare(GvoxVoxelDesc desc_a, GvoxVoxelDesc desc_b) GVOX_FUNC_ATTRIB->uint8_t {
    if (desc_a == desc_b) {
        return 2;
    }
    if (desc_a->attributes.size() != desc_b->attributes.size()) {
        return 0;
    }
    for (size_t i = 0; i < desc_a->attributes.size(); ++i) {
        if (memcmp(&desc_a->attributes[i], &desc_b->attributes[i], sizeof(Attribute)) != 0) {
            return 0;
        }
    }
    return 1;
}

// auto gvox_translate_format(void const *src_data, GvoxFormat src_format, void *dst_data, GvoxFormat dst_format) GVOX_FUNC_ATTRIB->GvoxResult {
//     return convert_formats(src_data, std::bit_cast<FormatDescriptor>(src_format), dst_data, std::bit_cast<FormatDescriptor>(dst_format));
// }

auto gvox_translate_voxel(void const *src_data, GvoxVoxelDesc src_desc, void *dst_data, GvoxVoxelDesc dst_desc, GvoxAttributeMapping const *attrib_mapping, uint32_t attrib_mapping_n) GVOX_FUNC_ATTRIB->GvoxResult {
    for (uint32_t mapping_i = 0; mapping_i < attrib_mapping_n; ++mapping_i) {
        auto const &mapping = attrib_mapping[mapping_i];

        auto const &src_attrib = src_desc->attributes[mapping.src_index];
        auto const &src_format = src_attrib.format_desc;

        auto const &dst_attrib = dst_desc->attributes[mapping.dst_index];
        auto const &dst_format = dst_attrib.format_desc;

        // NOTE: Attributes can't be larger than 64 bits, but they could be on non-byte
        // boundaries, and so we'll want to copy them into temp storage before shifting
        // them around to the desired output attribute description.
        // auto temp_attrib_storage = std::array<uint8_t, 10>{};

        if (is_packed_multi_channel_attribute(dst_attrib.type)) {
            if (is_packed_multi_channel_attribute(src_attrib.type)) {
                if (src_format.packed.component_count != dst_format.packed.component_count) {
                    return GVOX_ERROR_INVALID_ARGUMENT;
                }

                auto src_begin_bit = src_attrib.bit_offset;
                auto src_end_bit = src_begin_bit + src_attrib.bit_count;
                const auto *src_real_byte_begin = static_cast<uint8_t const *>(src_data) + src_begin_bit / 8;
                // auto src_real_byte_end = static_cast<uint8_t const *>(src_data) + (src_end_bit + 7) / 8;
                const auto *src_full_byte_begin = static_cast<uint8_t const *>(src_data) + (src_begin_bit + 7) / 8;
                const auto *src_full_byte_end = static_cast<uint8_t const *>(src_data) + src_end_bit / 8;

                auto dst_begin_bit = dst_attrib.bit_offset;
                // auto dst_end_bit = dst_begin_bit + dst_attrib.bit_count;
                auto *dst_real_byte_begin = static_cast<uint8_t *>(dst_data) + dst_begin_bit / 8;
                auto *dst_full_byte_begin = static_cast<uint8_t *>(dst_data) + (dst_begin_bit + 7) / 8;

                if (std::bit_cast<uint32_t>(src_format) == std::bit_cast<uint32_t>(dst_format)) {
                    // same format-format copy.

                    // optimization for when the bits line-up at byte boundaries
                    if (src_full_byte_begin != src_full_byte_end && (src_begin_bit & 7) == (dst_begin_bit & 7)) {
                        std::copy(src_full_byte_begin, src_full_byte_end, dst_full_byte_begin);
                    }

                    if ((dst_begin_bit % 8) != 0 || (src_begin_bit % 8) != 0 || (src_end_bit % 8) != 0) {
                        // TODO: Copy over bit-wise on voxel edges.
                        return GVOX_ERROR_UNKNOWN;
                    }
                } else {
                    // constexpr auto SWIZZLE_MASK = ~GVOX_CREATE_FORMAT(0, GVOX_PACKED_ENCODING(1, GVOX_SWIZZLE_MODE_REVERSE, 0, 0, 0));
                    auto const src_fmt_ = std::bit_cast<GvoxFormat>(src_format);
                    auto const dst_fmt_ = std::bit_cast<GvoxFormat>(dst_format);
                    // std::copy(src_real_byte_begin, src_real_byte_end, temp_attrib_storage.begin());

                    if ((src_fmt_ == GVOX_STANDARD_FORMAT_R8G8B8_SRGB && dst_fmt_ == GVOX_STANDARD_FORMAT_B8G8R8_SRGB) ||
                        (src_fmt_ == GVOX_STANDARD_FORMAT_B8G8R8_SRGB && dst_fmt_ == GVOX_STANDARD_FORMAT_R8G8B8_SRGB)) {
                        // swizzle RGB
                        if (src_real_byte_begin == src_full_byte_begin && dst_real_byte_begin == dst_full_byte_begin) {
                            dst_real_byte_begin[0] = src_real_byte_begin[2];
                            dst_real_byte_begin[1] = src_real_byte_begin[1];
                            dst_real_byte_begin[2] = src_real_byte_begin[0];
                        } else {
                            return GVOX_ERROR_UNKNOWN;
                        }
                    } else {
                        return GVOX_ERROR_UNKNOWN;
                    }
                }
            } else {
                // TODO: What should we do here?
                return GVOX_ERROR_UNKNOWN;

                // auto const &src1_attrib = src_desc->attributes[mapping.src1_index];
                // auto const &src1_format = src1_attrib.format_desc;

                // auto const &src2_attrib = src_desc->attributes[mapping.src2_index];
                // auto const &src2_format = src2_attrib.format_desc;
            }
        } else {
            if (is_packed_multi_channel_attribute(src_attrib.type)) {
                return GVOX_ERROR_INVALID_ARGUMENT;
            }
            // TODO: implement
        }
    }

    return GVOX_SUCCESS;
}
