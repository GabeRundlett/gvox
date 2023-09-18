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

    auto *dst_voxel_desc = info->dst->desc.get_voxel_desc(info->dst->self);
    // convert src data to be compatible with the dst_voxel_desc

    auto *converted_data = static_cast<void *>(nullptr);
    // test to see if the input data is already compatible (basically if it's the same exact voxel desc)
    auto is_compatible_voxel_desc = [](GvoxVoxelDesc desc_a, GvoxVoxelDesc desc_b) -> bool {
        return desc_a == desc_b;
    };
    if (is_compatible_voxel_desc(info->src_desc, dst_voxel_desc)) {
        converted_data = info->src_data;
    } else {
        // converted_data = convert_data(converted_data_stack);
    }

    // issue the fill call
    return info->dst->desc.fill(info->dst->self, converted_data, info->range);
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
    auto convert_formats(FormatDescriptor src_format, void *src_data, FormatDescriptor dst_format, void *dst_data) -> GvoxResult {
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

auto gvox_sample(GvoxSampleInfo const *info) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;

    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->struct_type != GVOX_STRUCT_TYPE_SAMPLE_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    auto *src_voxel_desc = info->src->desc.get_voxel_desc(info->src->self);

    auto temp_out_data = std::array<uint32_t, 4>{};

    if (info->src_channel_index >= src_voxel_desc->attributes.size()) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }

    auto result = info->src->desc.sample(info->src->self, info->src_channel_index, temp_out_data.data(), info->offset);
    if (result != GVOX_SUCCESS) {
        return result;
    }

    return convert_formats(
        src_voxel_desc->attributes.at(info->src_channel_index).format_desc, temp_out_data.data(),
        std::bit_cast<FormatDescriptor>(info->dst_format), info->dst);
}

// Adapter API
