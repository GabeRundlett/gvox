#include <gvox/gvox.h>
#include <gvox/containers/bounded_raw.h>

#include "../utils/tracy.hpp"
#include "gvox/core.h"

#include <cstdlib>
#include <cstdint>

#include <array>
#include <vector>
#include <span>
#include <new>
#include <bit>
#include <unordered_map>
#include <concepts>

#include <iostream>
#include <iomanip>

using Word = uint32_t;

struct Voxel {
    uint8_t *ptr{};
    uint32_t size{};
};

struct GvoxBoundedRawContainer {
    GvoxVoxelDesc voxel_desc{};
    GvoxExtent extent{};
    void *pre_allocated_buffer{};
};

auto gvox_container_bounded_raw_create(void **out_self, GvoxContainerCreateCbArgs const *args) -> GvoxResult {
    GvoxBoundedRawContainerConfig config;
    if (args->config != nullptr) {
        config = *static_cast<GvoxBoundedRawContainerConfig const *>(args->config);
    } else {
        config = {};
    }
    (*out_self) = new GvoxBoundedRawContainer({
        .voxel_desc = config.voxel_desc,
        .extent = config.extent,
        .pre_allocated_buffer = config.pre_allocated_buffer,
    });
    return GVOX_SUCCESS;
}
void gvox_container_bounded_raw_destroy(void *self_ptr) {
    delete static_cast<GvoxBoundedRawContainer *>(self_ptr);
}

auto gvox_container_bounded_raw_get_voxel_desc(void *self_ptr) -> GvoxVoxelDesc {
    auto &self = *static_cast<GvoxBoundedRawContainer *>(self_ptr);
    return self.voxel_desc;
}

auto gvox_container_bounded_raw_sample(void *self_ptr, uint32_t attrib_index, void *single_attrib_data, GvoxOffset offset) -> GvoxResult {
    auto &self = *static_cast<GvoxBoundedRawContainer *>(self_ptr);
    auto const dim = std::min(offset.axis_n, self.extent.axis_n);

    auto voxel_offset = uint64_t{0};
    auto voxel_size_bytes = (gvox_voxel_desc_size_in_bits(self.voxel_desc) + 7) >> 3;
    auto stride = uint64_t{voxel_size_bytes};

    for (size_t i = 0; i < dim; ++i) {
        if (offset.axis[i] < 0 || offset.axis[i] >= self.extent.axis[i]) {
            return GVOX_SUCCESS;
        }
        auto axis_p = static_cast<uint64_t>(offset.axis[i]);
        voxel_offset += axis_p * stride;
        stride *= self.extent.axis[i];
    }

    // TODO: ...
    uint32_t attrib_offset = attrib_index * 0;
    uint32_t attrib_size = 4;

    auto *data = static_cast<uint8_t *>(self.pre_allocated_buffer);

    std::copy(
        data + voxel_offset + attrib_offset,
        data + voxel_offset + attrib_offset + attrib_size,
        static_cast<uint8_t *>(single_attrib_data));

    return GVOX_SUCCESS;
}

auto gvox_container_bounded_raw_fill(void *self_ptr, void *single_voxel_data, GvoxRange range) -> GvoxResult {
    auto &self = *static_cast<GvoxBoundedRawContainer *>(self_ptr);
    if (range.offset.axis_n < range.extent.axis_n) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    auto const dim = range.extent.axis_n;

    if (dim < 1) {
        return GVOX_SUCCESS;
    }

    for (uint32_t i = 0; i < dim; ++i) {
        if (range.extent.axis[i] == 0) {
            return GVOX_SUCCESS;
        }
    }

    Voxel in_voxel = {
        .ptr = static_cast<uint8_t *>(single_voxel_data),
        .size = static_cast<uint32_t>((gvox_voxel_desc_size_in_bits(self.voxel_desc) + 7) >> 3),
    };

    auto offset_buffer = std::vector<int64_t>(static_cast<size_t>(dim) * 3);
    auto voxel_range_offset = GvoxOffsetMut{.axis_n = dim, .axis = offset_buffer.data() + std::ptrdiff_t(0 * dim)};
    auto voxel_range_extent = GvoxExtentMut{.axis_n = dim, .axis = reinterpret_cast<uint64_t *>(offset_buffer.data() + std::ptrdiff_t(1 * dim))};
    auto voxel_next = GvoxOffsetMut{.axis_n = dim, .axis = offset_buffer.data() + std::ptrdiff_t(2 * dim)};

    auto *voxel_ptr = static_cast<uint8_t *>(self.pre_allocated_buffer);
    {
        auto stride = size_t{in_voxel.size};

        bool completely_clipped = false;
        for (uint32_t i = 0; i < dim; ++i) {
            auto p0 = range.offset.axis[i];
            auto p1 = p0 + static_cast<int64_t>(range.extent.axis[i]) - 1;
            if (p0 >= static_cast<int64_t>(self.extent.axis[i]) || p1 < 0) {
                // NOTE: Should be impossible
                completely_clipped = true;
                break;
            }
            if (p0 < 0) {
                p0 = 0;
            }
            if (p1 >= static_cast<int64_t>(self.extent.axis[i])) {
                p1 = static_cast<int64_t>(self.extent.axis[i]) - 1;
            }
            voxel_range_offset.axis[i] = p0;
            voxel_range_extent.axis[i] = static_cast<uint64_t>(p1 - p0 + 1);

            voxel_ptr += static_cast<size_t>(voxel_range_offset.axis[i]) * stride;
            voxel_next.axis[i] = static_cast<int64_t>(static_cast<uint64_t>(stride) * (self.extent.axis[i] - voxel_range_extent.axis[i]));
            stride *= self.extent.axis[i];
        }

        if (completely_clipped) {
            return GVOX_SUCCESS;
        }
    }

    // TODO: Implement general case
    switch (dim) {
    case 2: {
        auto max_x = static_cast<uint32_t>(voxel_range_extent.axis[0]);
        auto max_y = static_cast<uint32_t>(voxel_range_extent.axis[1]);
        auto max_b = in_voxel.size;
        auto next_axis_0 = static_cast<size_t>(voxel_next.axis[0]);
        auto next_word_axis_0 = next_axis_0 / sizeof(Word);
        auto word_n = (max_b + sizeof(Word) - 1) / sizeof(Word);
        auto *in_word_ptr = reinterpret_cast<Word *>(in_voxel.ptr);
        auto *out_word_ptr = reinterpret_cast<Word *>(voxel_ptr);
        if (max_b == sizeof(Word)) {
            auto in_word = *in_word_ptr;
            for (uint32_t yi = 0; yi < max_y; yi++) {
                for (uint32_t xi = 0; xi < max_x; xi++) {
                    *out_word_ptr++ = in_word;
                }
                out_word_ptr += next_word_axis_0;
            }
        } else if ((max_b % sizeof(Word)) == 0) {
            for (uint32_t yi = 0; yi < max_y; yi++) {
                for (uint32_t vi = 0; vi < word_n; ++vi) {
                    auto *line_begin = out_word_ptr + vi;
                    auto in_word = in_word_ptr[vi];
                    for (uint32_t xi = 0; xi < max_x; xi++) {
                        *line_begin = in_word;
                        line_begin += word_n;
                    }
                }
                out_word_ptr += word_n * max_x;
                out_word_ptr += next_word_axis_0;
            }
        } else {
            for (uint32_t yi = 0; yi < max_y; yi++) {
                for (uint32_t xi = 0; xi < max_x; xi++) {
                    for (uint32_t i = 0; i < max_b; i++) {
                        *voxel_ptr++ = in_voxel.ptr[i];
                    }
                }
                voxel_ptr += next_axis_0;
            }
        }
    } break;
    default: return GVOX_ERROR_UNKNOWN;
    }

    return GVOX_SUCCESS;
}
