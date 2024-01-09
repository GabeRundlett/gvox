#include <gvox/gvox.h>
#include <gvox/containers/raw.h>

#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <new>
#include <vector>
#include <algorithm>

#include "raw_helper.hpp"

namespace {
    struct Container {
        GvoxVoxelDesc voxel_desc{};
        GvoxExtent3D extent{};
        void *pre_allocated_buffer{};
    };

    struct Iterator {
        size_t voxel_index{std::numeric_limits<size_t>::max()};
        GvoxOffset3D offset{};
        GvoxExtent3D extent{};
    };

    auto create(void **out_self, GvoxContainerCreateCbArgs const *args) -> GvoxResult {
        auto config = GvoxBoundedRaw3dContainerConfig{};
        if (args->config != nullptr) {
            config = *static_cast<GvoxBoundedRaw3dContainerConfig const *>(args->config);
        } else {
            config = {};
        }
        (*out_self) = new (std::nothrow) Container({
            .voxel_desc = config.voxel_desc,
            .extent = config.extent,
            .pre_allocated_buffer = config.pre_allocated_buffer,
        });
        return GVOX_SUCCESS;
    }

    void destroy(void *self_ptr) { delete static_cast<Container *>(self_ptr); }

    auto fill(void *self_ptr, void const *single_voxel_data, GvoxVoxelDesc src_voxel_desc, GvoxRange range) -> GvoxResult {
        auto &self = *static_cast<Container *>(self_ptr);

        // convert src data to be compatible with the dst_voxel_desc
        const auto *converted_data = static_cast<void const *>(nullptr);
        // test to see if the input data is already compatible (basically if it's the same exact voxel desc)
        auto converted_temp_storage = std::vector<uint8_t>{};
        auto self_voxel_size = (gvox_voxel_desc_size_in_bits(self.voxel_desc) + 7) >> 3;
        if (gvox_voxel_desc_compare(src_voxel_desc, self.voxel_desc) != 0) {
            converted_data = single_voxel_data;
        } else {
            converted_temp_storage.resize(self_voxel_size);
            // TODO: create standard mapping?
            auto mapping = GvoxAttributeMapping{.dst_index = 0, .src_index = 0};
            auto res = gvox_translate_voxel(single_voxel_data, src_voxel_desc, converted_temp_storage.data(), self.voxel_desc, &mapping, 1);
            if (res != GVOX_SUCCESS) {
                return res;
            }
            converted_data = converted_temp_storage.data();
        }

        if (range.offset.axis_n < range.extent.axis_n) {
            return GVOX_ERROR_INVALID_ARGUMENT;
        }
        auto const dim = std::min(range.extent.axis_n, 3u);

        if (dim < 1) {
            return GVOX_SUCCESS;
        }

        bool is_single_voxel = true;
        for (uint32_t i = 0; i < dim; ++i) {
            if (range.extent.axis[i] == 0) {
                return GVOX_SUCCESS;
            }
            if (range.extent.axis[i] != 1) {
                is_single_voxel = false;
            }
        }

        Voxel const in_voxel = {
            .ptr = static_cast<uint8_t const *>(converted_data),
            .size = static_cast<uint32_t>(self_voxel_size),
        };

        auto voxel_range_offset = GvoxOffset3D{0, 0, 0};
        auto voxel_range_extent = GvoxExtent3D{1, 1, 1};
        auto voxel_next = GvoxOffset3D{0, 0, 0};

        auto *voxel_ptr = static_cast<uint8_t *>(self.pre_allocated_buffer);

        {
            auto stride = size_t{in_voxel.size};

            bool completely_clipped = false;
            for (uint32_t i = 0; i < dim; ++i) {
                auto p0 = range.offset.axis[i];
                auto p1 = p0 + static_cast<int64_t>(range.extent.axis[i]) - 1;
                if (p0 >= static_cast<int64_t>(self.extent.data[i]) || p1 < 0) {
                    // NOTE: Should be impossible
                    completely_clipped = true;
                    break;
                }
                if (p0 < 0) {
                    p0 = 0;
                }
                if (p1 >= static_cast<int64_t>(self.extent.data[i])) {
                    p1 = static_cast<int64_t>(self.extent.data[i]) - 1;
                }
                voxel_range_offset.data[i] = p0;
                voxel_range_extent.data[i] = static_cast<uint64_t>(p1 - p0 + 1);

                voxel_ptr += static_cast<size_t>(voxel_range_offset.data[i]) * stride;
                voxel_next.data[i] = static_cast<int64_t>(static_cast<uint64_t>(stride) * (self.extent.data[i] - voxel_range_extent.data[i]));
                stride *= self.extent.data[i];
            }

            if (completely_clipped) {
                return GVOX_SUCCESS;
            }
        }

        if (is_single_voxel) {
            set(voxel_ptr, in_voxel);
        } else {
            fill_3d(voxel_ptr, in_voxel, GvoxExtentMut{.axis_n = 3, .axis = voxel_range_extent.data}, GvoxOffsetMut{.axis_n = 3, .axis = voxel_next.data});
        }

        return GVOX_SUCCESS;
    }

    auto move(void * /*self_ptr*/, GvoxContainer * /*src_containers*/, GvoxRange * /*src_ranges*/, GvoxOffset * /*offsets*/, uint32_t /*src_container_n*/) -> GvoxResult {
        return GVOX_ERROR_UNKNOWN;
    }

    auto sample(void *self_ptr, GvoxSample const *samples, uint32_t sample_n) -> GvoxResult {
        auto &self = *static_cast<Container *>(self_ptr);

        for (uint32_t sample_i = 0; sample_i < sample_n; ++sample_i) {
            auto const &out_voxel_data = samples[sample_i].dst_voxel_data;
            auto const &out_voxel_desc = samples[sample_i].dst_voxel_desc;
            auto const &offset = samples[sample_i].offset;

            auto const dim = std::min(offset.axis_n, 3u);
            auto self_voxel_size = (gvox_voxel_desc_size_in_bits(self.voxel_desc) + 7) >> 3;

            auto voxel_offset = size_t{};
            auto stride = size_t{self_voxel_size};
            for (size_t i = 0; i < dim; ++i) {
                if (offset.axis[i] < 0) {
                    return GVOX_SUCCESS;
                }
                voxel_offset += static_cast<size_t>(offset.axis[i]) * stride;
                stride *= self.extent.data[i];
            }

            auto *data = static_cast<uint8_t *>(self.pre_allocated_buffer);

            auto res = GVOX_SUCCESS;
            if (gvox_voxel_desc_compare(self.voxel_desc, out_voxel_desc) != 0) {
                memcpy(out_voxel_data, data + voxel_offset, self_voxel_size);
            } else {
                res = gvox_translate_voxel(data + voxel_offset, self.voxel_desc, out_voxel_data, out_voxel_desc, nullptr, 0);
            }
            if (res != GVOX_SUCCESS) {
                return res;
            }
        }
        return GVOX_SUCCESS;
    }

    void create_iterator(void * /*self_ptr*/, void **out_iterator_ptr) {
        // auto &self = *static_cast<Container *>(self_ptr);
        *out_iterator_ptr = new (std::nothrow) Iterator{
            // .chunk_iter = self.chunks.begin(),
        };
    }
    void destroy_iterator(void * /*self_ptr*/, void *iterator_ptr) {
        delete static_cast<Iterator *>(iterator_ptr);
    }
    void iterator_advance(void * /*self_ptr*/, void ** /*iterator_ptr*/, GvoxIteratorAdvanceInfo const * /*info*/, GvoxIteratorValue * /*out*/) {
        // auto &self = *static_cast<Container *>(self_ptr);
        // auto &iter = *static_cast<Iterator *>(*iterator_ptr);
        // out->range = GvoxRange{
        //     .offset = {.axis_n = 3, .axis = iter.offset.data},
        //     .extent = {.axis_n = 3, .axis = iter.extent.data},
        // };
        // while (true) {
        //     if (iter.chunk_iter == self.chunks.end()) {
        //         out->tag = GVOX_ITERATOR_VALUE_TYPE_NULL;
        //         return;
        //     }
        //     auto &[chunk_offset, chunk] = *iter.chunk_iter;
        //     if (iter.voxel_index == std::numeric_limits<size_t>::max()) {
        //         if (mode == GVOX_ITERATOR_ADVANCE_MODE_SKIP_BRANCH) {
        //             ++iter.chunk_iter;
        //             mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT;
        //             continue;
        //         } else {
        //             // Enter node
        //             iter.offset = chunk_offset;
        //             iter.extent = {CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE};
        //             out->tag = GVOX_ITERATOR_VALUE_TYPE_NODE_BEGIN;
        //             iter.voxel_index = 0;
        //             return;
        //         }
        //     }
        //     if (iter.voxel_index >= CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE || mode == GVOX_ITERATOR_ADVANCE_MODE_SKIP_BRANCH) {
        //         // Exit node
        //         iter.offset = chunk_offset;
        //         iter.extent = {CHUNK_SIZE, CHUNK_SIZE, CHUNK_SIZE};
        //         iter.voxel_index = std::numeric_limits<size_t>::max();
        //         out->tag = GVOX_ITERATOR_VALUE_TYPE_NODE_END;
        //         ++iter.chunk_iter;
        //         return;
        //     }
        //     {
        //         // Next voxel
        //         iter.offset = chunk_voxel_index_to_offset3d(iter.voxel_index);
        //         iter.offset.data[0] += chunk_offset.data[0] * static_cast<int64_t>(CHUNK_SIZE);
        //         iter.offset.data[1] += chunk_offset.data[1] * static_cast<int64_t>(CHUNK_SIZE);
        //         iter.offset.data[2] += chunk_offset.data[2] * static_cast<int64_t>(CHUNK_SIZE);
        //         iter.extent = GvoxExtent3D{1, 1, 1};
        //         out->tag = GVOX_ITERATOR_VALUE_TYPE_LEAF;
        //         out->voxel_data = static_cast<void *>(chunk.data.data() + iter.voxel_index * self.voxel_size_bytes());
        //         ++iter.voxel_index;
        //         out->voxel_desc = self.voxel_desc;
        //         return;
        //     }
        // }
    }
} // namespace

auto gvox_container_bounded_raw3d_description() GVOX_FUNC_ATTRIB->GvoxContainerDescription {
    return GvoxContainerDescription{
        .create = create,
        .destroy = destroy,
        .fill = fill,
        .move = move,
        .sample = sample,
        .create_iterator = create_iterator,
        .destroy_iterator = destroy_iterator,
        .iterator_advance = iterator_advance,
    };
}
