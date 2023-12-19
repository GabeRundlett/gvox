#include <gvox/format.h>
#include <gvox/stream.h>
#include <gvox/gvox.h>
#include <gvox/containers/raw3d.h>

#include <cstdlib>
#include <cstdint>
#include <cstring>

#include <new>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>

#include "raw_helper.hpp"

static constexpr auto LOG2_CHUNK_SIZE = size_t{6};
static constexpr auto CHUNK_SIZE = size_t{1} << LOG2_CHUNK_SIZE;
static constexpr auto VOXELS_PER_CHUNK = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

struct Chunk {
    GvoxRange range{};
    uint32_t dim{};
    std::vector<uint8_t> data{};

    Chunk() = default;
    Chunk(Chunk const &) = delete;
    Chunk(Chunk &&) = default;
    auto operator=(Chunk const &) -> Chunk & = delete;
    auto operator=(Chunk &&) -> Chunk & = default;
    ~Chunk() = default;
};

template <>
struct std::hash<GvoxOffset3D> {
    auto operator()(GvoxOffset3D const &k) const -> std::size_t {
        using std::hash;
        size_t result = 0;
        result = result ^ (hash<int64_t>()(k.data[0]) << 0);
        result = result ^ (hash<int64_t>()(k.data[1]) << 1);
        result = result ^ (hash<int64_t>()(k.data[2]) << 2);
        return result;
    }
};
auto operator==(GvoxOffset3D const &a, GvoxOffset3D const &b) -> bool {
    return a.data[0] == b.data[0] && a.data[1] == b.data[1] && a.data[2] == b.data[2];
}

struct GvoxRaw3dContainer {
    GvoxVoxelDesc voxel_desc{};
    std::unordered_map<GvoxOffset3D, Chunk> chunks{};
};

auto gvox_container_raw3d_description() GVOX_FUNC_ATTRIB->GvoxContainerDescription {
    return GvoxContainerDescription{
        .create = [](void **out_self, GvoxContainerCreateCbArgs const *args) -> GvoxResult {
            GvoxRaw3dContainerConfig config;
            if (args->config != nullptr) {
                config = *static_cast<GvoxRaw3dContainerConfig const *>(args->config);
            } else {
                config = {};
            }
            (*out_self) = new (std::nothrow) GvoxRaw3dContainer({
                .voxel_desc = config.voxel_desc,
            });
            return GVOX_SUCCESS;
        },
        .destroy = [](void *self_ptr) { delete static_cast<GvoxRaw3dContainer *>(self_ptr); },
        .fill = [](void *self_ptr, void const *single_voxel_data, GvoxVoxelDesc src_voxel_desc, GvoxRange range) -> GvoxResult {
            auto &self = *static_cast<GvoxRaw3dContainer *>(self_ptr);

            // convert src data to be compatible with the dst_voxel_desc
            const auto *converted_data = static_cast<void const *>(nullptr);
            // test to see if the input data is already compatible (basically if it's the same exact voxel desc)
            auto is_compatible_voxel_desc = [](GvoxVoxelDesc desc_a, GvoxVoxelDesc desc_b) -> bool {
                return desc_a == desc_b;
            };
            if (is_compatible_voxel_desc(src_voxel_desc, self.voxel_desc)) {
                converted_data = single_voxel_data;
            } else {
                // converted_data = convert_data(converted_data_stack);
            }

            if (range.offset.axis_n < range.extent.axis_n) {
                return GVOX_ERROR_INVALID_ARGUMENT;
            }
            auto const dim = std::min(range.extent.axis_n, 3u);

            if (dim < 1) {
                return GVOX_SUCCESS;
            }

            for (uint32_t i = 0; i < dim; ++i) {
                if (range.extent.axis[i] == 0) {
                    return GVOX_SUCCESS;
                }
            }

            Voxel const in_voxel = {
                .ptr = static_cast<uint8_t const *>(converted_data),
                .size = static_cast<uint32_t>((gvox_voxel_desc_size_in_bits(self.voxel_desc) + 7) >> 3),
            };
            auto const voxels_in_chunk = VOXELS_PER_CHUNK;
            auto const chunk_size = in_voxel.size * voxels_in_chunk;

            bool is_single_voxel = true;
            for (uint32_t i = 0; i < dim; ++i) {
                if (range.extent.axis[i] == 0) {
                    return GVOX_SUCCESS;
                }
                if (range.extent.axis[i] != 1) {
                    is_single_voxel = false;
                }
            }

            auto voxel_range_offset = GvoxOffset3D{};
            auto voxel_range_extent = GvoxExtent3D{};
            auto voxel_next = GvoxOffset3D{};

            auto chunk_range_offset = GvoxOffset3D{};
            auto chunk_range_extent = GvoxExtent3D{};
            auto chunk_coord = GvoxOffset3D{};

            for (size_t i = 0; i < dim; ++i) {
                auto range_max_i = range.offset.axis[i] + static_cast<int64_t>(std::max<uint64_t>(range.extent.axis[i], 1)) - 1;
                chunk_range_offset.data[i] = range.offset.axis[i] >> LOG2_CHUNK_SIZE;
                auto max_chunk_origin_i = range_max_i >> LOG2_CHUNK_SIZE;

                chunk_range_extent.data[i] = static_cast<uint64_t>(max_chunk_origin_i - chunk_range_offset.data[i] + 1);
            }

            auto chunk_iter_coord = GvoxExtent3D{};
            for (chunk_iter_coord.data[2] = 0; chunk_iter_coord.data[2] < chunk_range_extent.data[2]; ++chunk_iter_coord.data[2]) {
                for (chunk_iter_coord.data[1] = 0; chunk_iter_coord.data[1] < chunk_range_extent.data[1]; ++chunk_iter_coord.data[1]) {
                    for (chunk_iter_coord.data[0] = 0; chunk_iter_coord.data[0] < chunk_range_extent.data[0]; ++chunk_iter_coord.data[0]) {
                        bool completely_clipped = false;
                        for (uint32_t i = 0; i < dim; ++i) {
                            chunk_coord.data[i] = chunk_range_offset.data[i] + static_cast<int64_t>(chunk_iter_coord.data[i]);
                            auto chunk_p0 = chunk_coord.data[i] * static_cast<int64_t>(CHUNK_SIZE);
                            auto p0 = range.offset.axis[i] - chunk_p0;
                            auto p1 = p0 + static_cast<int64_t>(range.extent.axis[i]) - 1;
                            if (p0 >= static_cast<int64_t>(CHUNK_SIZE) || p1 < 0) {
                                // NOTE: Should be impossible
                                completely_clipped = true;
                                break;
                            }
                            if (p0 < 0) {
                                p0 = 0;
                            }
                            if (p1 >= static_cast<int64_t>(CHUNK_SIZE)) {
                                p1 = static_cast<int64_t>(CHUNK_SIZE) - 1;
                            }
                            voxel_range_offset.data[i] = p0;
                            voxel_range_extent.data[i] = static_cast<uint64_t>(p1 - p0 + 1);
                        }

                        if (completely_clipped) {
                            continue;
                        }

                        auto &chunk = self.chunks[chunk_coord];
                        if (chunk.dim < dim) {
                            // Need to resize the current chunk
                            // TODO: copy old data over
                            chunk.data.resize(static_cast<size_t>(chunk_size));
                        }
                        chunk.dim = std::max(dim, chunk.dim);

                        auto *voxel_ptr = chunk.data.data();
                        {
                            auto stride = size_t{in_voxel.size};
                            for (uint32_t i = 0; i < dim; ++i) {
                                voxel_ptr += static_cast<size_t>(voxel_range_offset.data[i]) * stride;
                                voxel_next.data[i] = static_cast<int64_t>(static_cast<uint64_t>(stride) * (CHUNK_SIZE - voxel_range_extent.data[i]));
                                stride *= CHUNK_SIZE;
                            }
                        }

                        if (is_single_voxel) {
                            set(voxel_ptr, in_voxel);
                        } else {
                            fill_3d(voxel_ptr, in_voxel, GvoxExtentMut{.axis_n = 3, .axis = voxel_range_extent.data}, GvoxOffsetMut{.axis_n = 3, .axis = voxel_next.data});
                        }
                    }
                }
            }

            return GVOX_SUCCESS;
        },
        .move = [](void *self_ptr, GvoxContainer *src_containers, GvoxRange *src_ranges, GvoxOffset *offsets, uint32_t src_container_n) -> GvoxResult {
            auto &self = *static_cast<GvoxRaw3dContainer *>(self_ptr);
            for (uint32_t src_i = 0; src_i < src_container_n; ++src_i) {
                auto &src = *static_cast<GvoxRaw3dContainer *>(*(void **)(src_containers[src_i]));
                auto &src_range = src_ranges[src_i];

                if (src_range.offset.axis_n < src_range.extent.axis_n) {
                    return GVOX_ERROR_INVALID_ARGUMENT;
                }
                auto const dim = std::min(src_range.extent.axis_n, 3u);
                // check if range lies on chunk bounds
                for (size_t axis_i = 0; axis_i < dim; ++axis_i) {
                    auto src_offset_i = src_range.offset.axis[axis_i];
                    auto dst_offset_i = src_range.offset.axis[axis_i] + offsets[src_i].axis[axis_i];
                    auto extent_i = axis_i < src_range.extent.axis_n ? src_range.extent.axis[axis_i] : 1;
                    if ((src_offset_i & static_cast<int64_t>(CHUNK_SIZE - 1)) != 0 ||
                        (dst_offset_i & static_cast<int64_t>(CHUNK_SIZE - 1)) != 0 ||
                        (extent_i & static_cast<int64_t>(CHUNK_SIZE - 1)) != 0) {
                        return GVOX_ERROR_INVALID_ARGUMENT;
                    }
                }
                // full chunks, lets move them over.
                for (auto &[coord, chunk] : src.chunks) {
                    bool in_range = true;
                    for (size_t axis_i = 0; axis_i < 3; ++axis_i) {
                        auto min_chunk_i = src_range.offset.axis[axis_i] >> LOG2_CHUNK_SIZE;
                        auto extent_i = axis_i < src_range.extent.axis_n ? src_range.extent.axis[axis_i] : 1;
                        auto max_chunk_i = min_chunk_i + (static_cast<int64_t>(extent_i) >> LOG2_CHUNK_SIZE) - 1;
                        auto coord_i = coord.data[axis_i];
                        if (coord_i < min_chunk_i || coord_i > max_chunk_i) {
                            in_range = false;
                            break;
                        }
                    }
                    if (!in_range) {
                        continue;
                    }
                    auto dst_coord = coord;
                    for (size_t axis_i = 0; axis_i < 3; ++axis_i) {
                        dst_coord.data[axis_i] += offsets[src_i].axis[axis_i] >> LOG2_CHUNK_SIZE;
                    }
                    self.chunks[dst_coord] = std::move(chunk);
                }
            }
            return GVOX_SUCCESS;
        },
        .sample = [](void *self_ptr, GvoxSample const *samples, uint32_t sample_n) -> GvoxResult {
            for (uint32_t sample_i = 0; sample_i < sample_n; ++sample_i) {
                auto const &out_voxel_data = samples[sample_i].dst_voxel_data;
                auto const &out_voxel_desc = samples[sample_i].dst_voxel_desc;
                auto const &offset = samples[sample_i].offset;

                auto &self = *static_cast<GvoxRaw3dContainer *>(self_ptr);
                auto const dim = std::min(offset.axis_n, 3u);
                auto chunk_offset = GvoxOffset3D{};

                auto voxel_offset = uint64_t{0};
                auto voxel_size_bytes = (gvox_voxel_desc_size_in_bits(self.voxel_desc) + 7) >> 3;
                auto stride = uint64_t{voxel_size_bytes};

                for (size_t i = 0; i < dim; ++i) {
                    chunk_offset.data[i] = offset.axis[i] >> LOG2_CHUNK_SIZE;
                    auto axis_p = static_cast<uint64_t>(offset.axis[i] - (chunk_offset.data[i] << LOG2_CHUNK_SIZE));
                    voxel_offset += axis_p * stride;
                    stride *= CHUNK_SIZE;
                }

                auto chunk_iter = self.chunks.find(chunk_offset);
                if (chunk_iter == self.chunks.end()) {
                    continue;
                }

                auto &chunk = chunk_iter->second;
                auto *data = chunk.data.data();

                auto res = GVOX_SUCCESS;
                if (gvox_voxel_desc_compare(self.voxel_desc, out_voxel_desc) != 0) {
                    auto size = gvox_voxel_desc_size_in_bits(self.voxel_desc);
                    memcpy(out_voxel_data, data + voxel_offset, (size + 7) / 8);
                } else {
                    res = gvox_translate_voxel(data + voxel_offset, self.voxel_desc, out_voxel_data, out_voxel_desc, nullptr, 0);
                }
                if (res != GVOX_SUCCESS) {
                    return res;
                }
            }
            return GVOX_SUCCESS;
        },
    };
}
