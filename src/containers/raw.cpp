#include <gvox/gvox.h>
#include <gvox/containers/raw.h>

#include "../utils/tracy.hpp"
#include "../utils/common.hpp"

#include <cstdlib>
#include <cstdint>

#include <array>
#include <vector>
#include <span>
#include <new>
#include <bit>
#include <unordered_map>

#include <iostream>
#include <iomanip>

#include "raw_helper.hpp"

static constexpr auto LOG2_CHUNK_SIZE = size_t{6};
static constexpr auto CHUNK_SIZE = size_t{1} << LOG2_CHUNK_SIZE;

struct Chunk {
    std::vector<int64_t> key_data{};
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

struct ChunkKey {
    GvoxOffset offset{};

    ChunkKey() = default;
    ChunkKey(ChunkKey const &) = delete;
    ChunkKey(ChunkKey &&) = default;
    auto operator=(ChunkKey const &) -> ChunkKey & = delete;
    auto operator=(ChunkKey &&) -> ChunkKey & = default;
    ~ChunkKey() = default;

    explicit ChunkKey(GvoxOffsetType auto a_offset) {
        offset.axis_n = a_offset.axis_n;
        offset.axis = a_offset.axis;
    }

    [[nodiscard]] constexpr auto size() const {
        return offset.axis_n;
    }
    constexpr auto operator[](size_t i) const -> auto const & {
        return offset.axis[i];
    }
};

template <>
struct std::hash<ChunkKey> {
    auto operator()(ChunkKey const &k) const -> std::size_t {
        using std::hash;
        size_t result = 0;
        for (uint32_t i = 0; i < k.size(); ++i) {
            result = result ^ (hash<int64_t>()(k[i]) << i);
        }
        return result;
    }
};
auto operator==(ChunkKey const &a, ChunkKey const &b) -> bool {
    auto max_dim = std::max(a.size(), b.size());
    for (uint32_t i = 0; i < max_dim; ++i) {
        auto a_val = i < a.size() ? a[i] : 0;
        auto b_val = i < b.size() ? b[i] : 0;
        if (a_val != b_val) {
            return false;
        }
    }
    return true;
}

struct GvoxRawContainer {
    GvoxVoxelDesc voxel_desc{};
    std::unordered_map<ChunkKey, Chunk> chunks{};

    // TODO: create key allocator
    std::vector<int64_t> chunk_key_buffer{};
    std::vector<uint64_t> chunk_key_free_list{};
};

auto gvox_container_raw_create(void **out_self, GvoxContainerCreateCbArgs const *args) -> GvoxResult {
    GvoxRawContainerConfig config;
    if (args->config != nullptr) {
        config = *static_cast<GvoxRawContainerConfig const *>(args->config);
    } else {
        config = {};
    }
    (*out_self) = new GvoxRawContainer({
        .voxel_desc = config.voxel_desc,
    });
    return GVOX_SUCCESS;
}
void gvox_container_raw_destroy(void *self_ptr) {
    delete static_cast<GvoxRawContainer *>(self_ptr);
}

template <std::integral T>
auto int_pow(T x, T n) {
    T result = T{1};
    for (T i = 0; i < n; ++i) {
        result *= x;
    }
    return result;
}

auto operator<<(std::ostream &out, GvoxOffsetOrExtentType auto offset) -> std::ostream & {
    out << "[";
    for (uint32_t i = 0; i < offset.axis_n - 1; ++i) {
        out << offset.axis[i] << ", ";
    }
    return out << offset.axis[offset.axis_n - 1] << "]";
}

auto voxel_in_chunk_index(GvoxOffsetType auto o) -> size_t {
    size_t result = 0;
    size_t stride = 1;
    for (uint32_t axis_i = 0; axis_i < o.axis_n; ++axis_i) {
        result += static_cast<size_t>(o.axis[axis_i]) * stride;
        stride *= CHUNK_SIZE;
    }
    return result;
}

struct TestIter {
    explicit TestIter(GvoxExtentType auto a_extent, std::span<int64_t> data) : coord_data{data} {
        reset_range(a_extent);
    }

    auto next() -> GvoxOffset {
        // ZoneScopedN("Iter Next");
        if (!(index < total)) {
            return {.axis_n = 0, .axis = nullptr};
        }
        auto coord = GvoxOffsetMut{.axis_n = extent.axis_n, .axis = coord_data.data()};
        for (auto d = size_t{0}; d < extent.axis_n; ++d) {
            coord.axis[d] = (index / dimension_cumulative_stride.axis[d]) % static_cast<int64_t>(extent.axis[d]);
        }
        ++index;
        return GvoxOffset(coord);
    }

    void reset_range(GvoxExtentType auto a_extent) {
        auto dim = a_extent.axis_n;
        // coord_data.resize(static_cast<size_t>(dim) * 3);
        auto *extent_ptr = reinterpret_cast<uint64_t *>(coord_data.data() + static_cast<ptrdiff_t>(dim) * 1);
        memcpy(extent_ptr, a_extent.axis, sizeof(a_extent.axis[0]) * dim);
        extent.axis_n = dim;
        extent.axis = extent_ptr;
        dimension_cumulative_stride.axis_n = dim;
        dimension_cumulative_stride.axis = coord_data.data() + static_cast<ptrdiff_t>(dim) * 2;
        total = 1;
        for (uint32_t d = 0; d < dim; ++d) {
            coord_data[d + dim * 2] = total;
            total *= static_cast<int64_t>(extent.axis[d]);
        }
        index = 0;
    }

  private:
    int64_t total{};
    int64_t index{};
    GvoxExtent extent{};
    GvoxOffset dimension_cumulative_stride{};
    std::span<int64_t> coord_data{};
    // std::vector<int64_t> coord_data{};
};

auto gvox_container_raw_sample(void *self_ptr, void *out_voxel_data, GvoxVoxelDesc out_voxel_desc, GvoxOffset offset) -> GvoxResult {
    auto &self = *static_cast<GvoxRawContainer *>(self_ptr);
    auto const dim = offset.axis_n;
    auto offset_buffer = std::vector<int64_t>(static_cast<size_t>(dim) * 2);
    auto chunk_offset = GvoxOffsetMut{.axis_n = dim, .axis = offset_buffer.data() + std::ptrdiff_t(0 * dim)};
    // auto voxel_offset = GvoxOffsetMut{.axis_n = dim, .axis = offset_buffer.data() + std::ptrdiff_t(1 * dim)};

    auto voxel_offset = uint64_t{0};
    auto voxel_size_bytes = (gvox_voxel_desc_size_in_bits(self.voxel_desc) + 7) >> 3;
    auto stride = uint64_t{voxel_size_bytes};

    for (size_t i = 0; i < dim; ++i) {
        chunk_offset.axis[i] = offset.axis[i] >> LOG2_CHUNK_SIZE;
        auto axis_p = static_cast<uint64_t>(offset.axis[i] - (chunk_offset.axis[i] << LOG2_CHUNK_SIZE));
        voxel_offset += axis_p * stride;
        stride *= CHUNK_SIZE;
    }

    auto chunk_iter = self.chunks.find(ChunkKey(chunk_offset));
    if (chunk_iter == self.chunks.end()) {
        return GVOX_SUCCESS;
    }

    auto &chunk = chunk_iter->second;
    auto *data = chunk.data.data();
    return gvox_translate_voxel(data + voxel_offset, self.voxel_desc, out_voxel_data, out_voxel_desc);
}

auto gvox_container_raw_fill(void *self_ptr, void *single_voxel_data, GvoxVoxelDesc src_voxel_desc, GvoxRange range) -> GvoxResult {
    auto &self = *static_cast<GvoxRawContainer *>(self_ptr);

    // convert src data to be compatible with the dst_voxel_desc
    auto *converted_data = static_cast<void *>(nullptr);
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
        .ptr = static_cast<uint8_t *>(converted_data),
        .size = static_cast<uint32_t>((gvox_voxel_desc_size_in_bits(self.voxel_desc) + 7) >> 3),
    };
    auto const voxels_in_chunk = int_pow(static_cast<uint64_t>(CHUNK_SIZE), static_cast<uint64_t>(dim));
    auto const chunk_size = in_voxel.size * voxels_in_chunk;

    auto offset_buffer = std::vector<int64_t>(static_cast<size_t>(dim) * 16);
    auto chunk_coord_buffer = std::vector<int64_t>(static_cast<size_t>(dim));

    for (uint32_t i = 0; i < dim; ++i) {
        if (range.extent.axis[i] == 0) {
            return GVOX_SUCCESS;
        }
    }

    auto voxel_range_offset = GvoxOffsetMut{.axis_n = dim, .axis = offset_buffer.data() + std::ptrdiff_t(0 * dim)};
    auto voxel_range_extent = GvoxExtentMut{.axis_n = dim, .axis = reinterpret_cast<uint64_t *>(offset_buffer.data() + std::ptrdiff_t(1 * dim))};
    auto voxel_next = GvoxOffsetMut{.axis_n = dim, .axis = offset_buffer.data() + std::ptrdiff_t(2 * dim)};

    auto chunk_range_offset = GvoxOffsetMut{.axis_n = dim, .axis = offset_buffer.data() + std::ptrdiff_t(3 * dim)};
    auto chunk_range_extent = GvoxExtentMut{.axis_n = dim, .axis = reinterpret_cast<uint64_t *>(offset_buffer.data() + std::ptrdiff_t(4 * dim))};
    auto chunk_coord = GvoxOffsetMut{.axis_n = dim, .axis = chunk_coord_buffer.data()};

    for (size_t i = 0; i < dim; ++i) {
        auto range_max_i = range.offset.axis[i] + static_cast<int64_t>(std::max<uint64_t>(range.extent.axis[i], 1)) - 1;
        chunk_range_offset.axis[i] = range.offset.axis[i] >> LOG2_CHUNK_SIZE;
        auto max_chunk_origin_i = range_max_i >> LOG2_CHUNK_SIZE;

        chunk_range_extent.axis[i] = static_cast<uint64_t>(max_chunk_origin_i - chunk_range_offset.axis[i] + 1);
    }

    auto chunk_range_iter = TestIter(chunk_range_extent, {offset_buffer.data() + std::ptrdiff_t(6 * dim), static_cast<size_t>(dim) * 3});

    while (true) {
        auto chunk_iter_coord = chunk_range_iter.next();
        if (chunk_iter_coord.axis == nullptr) {
            break;
        }

        bool completely_clipped = false;
        for (uint32_t i = 0; i < dim; ++i) {
            chunk_coord.axis[i] = chunk_range_offset.axis[i] + chunk_iter_coord.axis[i];
            auto chunk_p0 = chunk_coord.axis[i] * static_cast<int64_t>(CHUNK_SIZE);
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
            voxel_range_offset.axis[i] = p0;
            voxel_range_extent.axis[i] = static_cast<uint64_t>(p1 - p0 + 1);
        }

        if (completely_clipped) {
            continue;
        }

        auto &chunk = self.chunks[ChunkKey(chunk_coord)];
        if (chunk.dim == 0) {
            // First time created
            chunk.key_data = std::move(chunk_coord_buffer);
            // TODO: Make faster by necessitating a new global alloc for every chunk
            chunk_coord_buffer = std::vector<int64_t>(static_cast<size_t>(dim));
            chunk_coord = GvoxOffsetMut{.axis_n = dim, .axis = chunk_coord_buffer.data()};
        }
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
                voxel_ptr += static_cast<size_t>(voxel_range_offset.axis[i]) * stride;
                voxel_next.axis[i] = static_cast<int64_t>(static_cast<uint64_t>(stride) * (CHUNK_SIZE - voxel_range_extent.axis[i]));
                stride *= CHUNK_SIZE;
            }
        }

        // NOTE: Currently breaks with N > 4 because the chunk alloc would be too big
        fill_Nd(dim, voxel_ptr, in_voxel, voxel_range_extent, voxel_next);
    }

    return GVOX_SUCCESS;
}
