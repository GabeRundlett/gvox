#include <gvox/gvox.h>

#include <vector>
#include <array>
#include <cmath>
#include <bit>

#include "gvox/core.h"
#include "gvox/format.h"
#include "types.hpp"
#include "utils/handle.hpp"

#include <gcem.hpp>
#include <entt/entt.hpp>

struct GvoxChainStruct {
    GvoxStructType struct_type;
    void const *next;
};

auto gvox_fill(GvoxFillInfo const *info) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;

    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->struct_type != GVOX_STRUCT_TYPE_FILL_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    // issue the fill call
    return info->dst->desc.fill(info->dst->self, info->src_data, info->src_desc, info->range);
}

auto gvox_blit_prepare(GvoxParser parser) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;

    if (parser == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }

    return GVOX_ERROR_UNKNOWN;
}

auto gvox_blit(GvoxBlitInfo const *info) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;

    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->struct_type != GVOX_STRUCT_TYPE_BLIT_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    return GVOX_ERROR_UNKNOWN;
}

namespace float_conv {
    constexpr auto from_unorm(uint32_t bit_n, uint32_t data) -> float {
        uint32_t mask = (1 << bit_n) - 1;
        return static_cast<float>(data & mask) / static_cast<float>(mask);
    }
    constexpr auto to_unorm(uint32_t bit_n, float data) -> uint32_t {
        uint32_t mask = (1 << bit_n) - 1;
        return static_cast<uint32_t>(data * static_cast<float>(mask)) & mask;
    }

    constexpr auto from_snorm(uint32_t bit_n, uint32_t data) -> float {
        return from_unorm(bit_n, data) * 2.0f - 1.0f;
    }
    constexpr auto to_snorm(uint32_t bit_n, float data) -> uint32_t {
        return to_unorm(bit_n, data * 0.5f + 0.5f);
    }

    constexpr auto from_srgb(uint32_t bit_n, uint32_t data) -> float {
        if (std::is_constant_evaluated()) {
            return gcem::pow(from_unorm(bit_n, data), 2.2f);
        } else {
            return std::pow(from_unorm(bit_n, data), 2.2f);
        }
    }
    constexpr auto to_srgb(uint32_t bit_n, float data) -> uint32_t {
        if (std::is_constant_evaluated()) {
            return to_unorm(bit_n, gcem::pow(data, 2.2f));
        } else {
            return to_unorm(bit_n, std::pow(data, 2.2f));
        }
    }
} // namespace float_conv

namespace {
    auto convert_formats(void const *src_data, FormatDescriptor src_format, void *dst_data, FormatDescriptor dst_format) -> GvoxResult {
        if (src_format.component_count != dst_format.component_count) {
            return GVOX_ERROR_INVALID_ARGUMENT;
        }

        if (std::bit_cast<uint32_t>(src_format) == std::bit_cast<uint32_t>(dst_format)) {
            auto size = static_cast<size_t>(src_format.c0_bit_count + src_format.c1_bit_count + src_format.c2_bit_count + src_format.c3_bit_count + 7) / 8;
            std::memcpy(dst_data, src_data, size);
            return GVOX_SUCCESS;
        }

        return GVOX_ERROR_UNKNOWN;

        // TODO: Finish impl
        // auto component_count = src_format.component_count + 1;
        // auto component_src_bit_counts = std::array<uint32_t, 4>{
        //     src_format.c0_bit_count,
        //     src_format.c1_bit_count,
        //     src_format.c2_bit_count,
        //     src_format.c3_bit_count,
        // };
        // auto component_dst_bit_counts = std::array<uint32_t, 4>{
        //     src_format.c0_bit_count,
        //     src_format.c1_bit_count,
        //     src_format.c2_bit_count,
        //     src_format.c3_bit_count,
        // };
        // auto current_src_bit_offset = uint32_t{0};
        // auto current_dst_bit_offset = uint32_t{0};
        // for (uint32_t i = 0; i < component_count; ++i) {
        //     uint32_t component_data{};
        //     // read in component data
        //     uint32_t src_component_data{};
        //     switch (src_format.encoding) {
        //     case GVOX_FORMAT_ENCODING_UNORM: component_data = std::bit_cast<uint32_t>(float_conv::from_unorm(component_src_bit_counts.at(i), src_component_data)); break;
        //     case GVOX_FORMAT_ENCODING_SNORM: component_data = std::bit_cast<uint32_t>(float_conv::from_snorm(component_src_bit_counts.at(i), src_component_data)); break;
        //     case GVOX_FORMAT_ENCODING_SRGB: component_data = std::bit_cast<uint32_t>(float_conv::from_srgb(component_src_bit_counts.at(i), src_component_data)); break;
        //     default: return GVOX_ERROR_INVALID_ARGUMENT;
        //     }
        //     uint32_t dst_component_data{};
        //     switch (dst_format.encoding) {
        //     case GVOX_FORMAT_ENCODING_UNORM: dst_component_data = float_conv::to_unorm(component_dst_bit_counts.at(i), std::bit_cast<float>(component_data)); break;
        //     case GVOX_FORMAT_ENCODING_SNORM: dst_component_data = float_conv::to_snorm(component_dst_bit_counts.at(i), std::bit_cast<float>(component_data)); break;
        //     case GVOX_FORMAT_ENCODING_SRGB: dst_component_data = float_conv::to_srgb(component_dst_bit_counts.at(i), std::bit_cast<float>(component_data)); break;
        //     default: return GVOX_ERROR_INVALID_ARGUMENT;
        //     }
        //     // write out component data
        //     current_src_bit_offset += component_src_bit_counts.at(i);
        //     current_dst_bit_offset += component_dst_bit_counts.at(i);
        // }
        // return GVOX_SUCCESS;
    }
} // namespace

auto gvox_translate_format(void const *src_data, GvoxFormat src_format, void *dst_data, GvoxFormat dst_format) GVOX_FUNC_ATTRIB->GvoxResult {
    return convert_formats(src_data, std::bit_cast<FormatDescriptor>(src_format), dst_data, std::bit_cast<FormatDescriptor>(dst_format));
}

auto gvox_translate_voxel(void const *src_data, GvoxVoxelDesc src_desc, void *dst_data, GvoxVoxelDesc dst_desc) GVOX_FUNC_ATTRIB->GvoxResult {
    // TODO: ...
    auto src_attrib_i = size_t{0};
    auto dst_attrib_i = size_t{0};

    auto src_attrib_offset = size_t{0};

    // if (info->src_channel_index >= src_voxel_desc->attributes.size()) {
    //     return GVOX_ERROR_INVALID_ARGUMENT;
    // }

    auto temp_out_data0 = std::array<uint8_t, 16>{};
    auto const *begin = static_cast<uint8_t const *>(src_data) + src_attrib_offset;
    auto const *end = begin + 4;

    std::copy(begin, end, temp_out_data0.data());

    if (dst_desc->attributes.size() == 1) {
        return convert_formats(
            temp_out_data0.data(), src_desc->attributes.at(src_attrib_i).format_desc,
            dst_data, dst_desc->attributes.at(dst_attrib_i).format_desc);
    }

    // auto temp_out_data1 = std::array<uint8_t, 16>{};

    return GVOX_ERROR_UNKNOWN;
}

auto gvox_sample(GvoxSampleInfo const *info) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;

    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->struct_type != GVOX_STRUCT_TYPE_SAMPLE_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    auto result = info->src->desc.sample(info->src->self, info->dst, info->dst_voxel_desc, info->offset);
    if (result != GVOX_SUCCESS) {
        return result;
    }

    return GVOX_SUCCESS;
}

// Stream API
