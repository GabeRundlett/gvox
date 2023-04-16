#include <gvox/gvox.h>

#include <cstdlib>
#include <cstdint>

#include <bit>
#include <array>
#include <vector>
#include <new>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include <mutex>

static constexpr auto REGION_SIZE = size_t{8};
static constexpr auto CHUNK_SIZE = size_t{8};

static constexpr auto ceil_log2(uint32_t x) -> uint32_t {
    constexpr auto const t = std::array<uint32_t, 5>{
        0xFFFF0000u,
        0x0000FF00u,
        0x000000F0u,
        0x0000000Cu,
        0x00000002u};

    uint32_t y = (((x & (x - 1)) == 0) ? 0 : 1);
    int j = 16;

    for (uint32_t const i : t) {
        int const k = (((x & i) == 0) ? 0 : j);
        y += static_cast<uint32_t>(k);
        x >>= k;
        j >>= 1;
    }

    return y;
}

static constexpr auto get_mask(uint32_t bits_per_variant) -> uint32_t {
    return (~0u) >> (32 - bits_per_variant);
}

static constexpr auto calc_palette_region_size(size_t bits_per_variant) -> size_t {
    auto palette_region_size = (bits_per_variant * REGION_SIZE * REGION_SIZE * REGION_SIZE + 7) / 8;
    palette_region_size = (palette_region_size + 3) / 4;
    auto size = palette_region_size + 1;
    return size * 4;
}

static constexpr auto calc_block_size(size_t variant_n) -> size_t {
    return calc_palette_region_size(ceil_log2(static_cast<uint32_t>(variant_n))) + sizeof(uint32_t) * variant_n;
}

static constexpr auto VOXELS_PER_PALETTE_REGION = REGION_SIZE * REGION_SIZE * REGION_SIZE;
static constexpr auto PALETTE_REGIONS_PER_CHUNK = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

static constexpr auto MAX_REGION_ALLOCATION_SIZE = VOXELS_PER_PALETTE_REGION * sizeof(uint32_t);
static constexpr auto MAX_REGION_COMPRESSED_VARIANT_N =
    REGION_SIZE == 8 ? 367 : (REGION_SIZE == 16 ? 2559 : 0);

static_assert(calc_block_size(MAX_REGION_COMPRESSED_VARIANT_N) <= MAX_REGION_ALLOCATION_SIZE);
static_assert(calc_block_size(MAX_REGION_COMPRESSED_VARIANT_N + 1) > MAX_REGION_ALLOCATION_SIZE);

struct ChannelHeader {
    uint32_t variant_n;
    uint32_t blob_offset; // if variant_n == 1, this is just the data
};

struct PaletteRegion {
    std::unordered_set<uint32_t> palette{};
    std::unique_ptr<std::array<std::pair<uint32_t, bool>, VOXELS_PER_PALETTE_REGION>> data{};
    uint32_t accounted_for{};
};

using PaletteRegionChannels = std::vector<PaletteRegion>;

struct LoadedRegionHeader {
    std::vector<ChannelHeader> channels;
};

struct Chunk {
    std::array<PaletteRegionChannels, PALETTE_REGIONS_PER_CHUNK> palette_chunks{};
};

struct GvoxGvpContainer {
    GvoxRegionRange range{};
    std::vector<std::vector<std::vector<Chunk>>> chunks{};

    // parse
    // GvoxRegionRange range{};
    // uint32_t blob_size{};
    // uint32_t channel_flags{};
    // uint32_t channel_n{};
    // size_t offset{};
    // uint32_t r_nx{};
    // uint32_t r_ny{};
    // uint32_t r_nz{};
    // std::vector<LoadedRegionHeader> region_headers{};
    // std::vector<uint8_t> buffer{};
    // std::array<uint32_t, 32> channel_indices{};

    // serialize
    // GvoxRegionRange range{};
    // size_t offset{};
    // size_t blobs_begin{};
    // size_t blob_size_offset{};
    // std::vector<uint8_t> data{};
    // std::vector<uint8_t> channels{};
    // uint32_t region_nx{};
    // uint32_t region_ny{};
    // uint32_t region_nz{};
    // std::vector<PaletteRegionChannels> palette_region_channels{};
};

auto gvox_container_image_create(void **self, void const *config_ptr) -> GvoxResult {
    GvoxImageContainerConfig config;
    if (config_ptr) {
        config = *static_cast<GvoxImageContainerConfig const *>(config_ptr);
    } else {
        config = {};
    }
    *self = new GvoxImageParseAdapter(config);
    return GVOX_SUCCESS;
}
auto gvox_container_image_parse(void *self, GvoxParseCbInfo const *info) -> GvoxResult {
    return static_cast<GvoxImageParseAdapter *>(self)->parse(*info);
}
auto gvox_container_image_serialize(void *self) -> GvoxResult {
    return static_cast<GvoxImageParseAdapter *>(self)->serialize();
}
void gvox_container_image_destroy(void *self) {
    delete static_cast<GvoxImageParseAdapter *>(self);
}

#if 0

// General
extern "C" auto gvox_parse_adapter_gvox_palette_query_details() -> GvoxParseAdapterDetails {
    return {
        .preferred_blit_mode = GVOX_BLIT_MODE_DONT_CARE,
    };
}

extern "C" auto gvox_parse_adapter_gvox_palette_query_parsable_range(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) -> GvoxRegionRange {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    return user_state.range;
}

extern "C" auto gvox_parse_adapter_gvox_palette_sample_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxSample {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if (offset->x < user_state.range.offset.x ||
        offset->y < user_state.range.offset.y ||
        offset->z < user_state.range.offset.z ||
        offset->x >= user_state.range.offset.x + static_cast<int32_t>(user_state.range.extent.x) ||
        offset->y >= user_state.range.offset.y + static_cast<int32_t>(user_state.range.extent.y) ||
        offset->z >= user_state.range.offset.z + static_cast<int32_t>(user_state.range.extent.z)) {
        return {0u, 0u};
    }
    auto const xi = static_cast<uint32_t>(offset->x - user_state.range.offset.x) / REGION_SIZE;
    auto const yi = static_cast<uint32_t>(offset->y - user_state.range.offset.y) / REGION_SIZE;
    auto const zi = static_cast<uint32_t>(offset->z - user_state.range.offset.z) / REGION_SIZE;
    auto const px = static_cast<uint32_t>(offset->x - user_state.range.offset.x) - xi * REGION_SIZE;
    auto const py = static_cast<uint32_t>(offset->y - user_state.range.offset.y) - yi * REGION_SIZE;
    auto const pz = static_cast<uint32_t>(offset->z - user_state.range.offset.z) - zi * REGION_SIZE;
    auto r_nx = user_state.r_nx;
    auto r_ny = user_state.r_ny;
    auto &channel_header = user_state.region_headers[xi + yi * r_nx + zi * r_nx * r_ny].channels[user_state.channel_indices[channel_id]];
    if (channel_header.variant_n <= 1) {
        return {channel_header.blob_offset, 1u};
    } else {
        uint8_t *buffer_ptr = user_state.buffer.data() + channel_header.blob_offset;
        if (channel_header.variant_n > MAX_REGION_COMPRESSED_VARIANT_N) {
            auto const index = px + py * REGION_SIZE + pz * REGION_SIZE * REGION_SIZE;
            // return 0;
            return {*reinterpret_cast<uint32_t *>(buffer_ptr + index * sizeof(uint32_t)), 1u};
        } else {
            auto *palette_begin = reinterpret_cast<uint32_t *>(buffer_ptr);
            auto const bits_per_variant = ceil_log2(channel_header.variant_n);
            buffer_ptr += channel_header.variant_n * sizeof(uint32_t);
            auto const index = px + py * REGION_SIZE + pz * REGION_SIZE * REGION_SIZE;
            auto const bit_index = static_cast<uint32_t>(index) * bits_per_variant;
            auto const byte_index = bit_index / 8;
            auto const bit_offset = static_cast<uint32_t>(bit_index - byte_index * 8);
            auto const mask = get_mask(bits_per_variant);
            auto input = std::bit_cast<uint32_t>(*reinterpret_cast<std::array<uint8_t, 4> *>(buffer_ptr + byte_index));
            auto const palette_id = (input >> bit_offset) & mask;
            // return palette_id;
            return {palette_begin[palette_id], 1u};
        }
    }
}

// Serialize Driven
extern "C" auto gvox_parse_adapter_gvox_palette_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> uint32_t {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    if ((channel_flags & ~user_state.channel_flags) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
        return 0;
    }

    auto ax = static_cast<uint32_t>(range->offset.x - user_state.range.offset.x) / static_cast<uint32_t>(REGION_SIZE);
    auto ay = static_cast<uint32_t>(range->offset.y - user_state.range.offset.y) / static_cast<uint32_t>(REGION_SIZE);
    auto az = static_cast<uint32_t>(range->offset.z - user_state.range.offset.z) / static_cast<uint32_t>(REGION_SIZE);
    auto bx = (static_cast<uint32_t>(range->offset.x - user_state.range.offset.x) + range->extent.x + static_cast<uint32_t>(REGION_SIZE - 1)) / static_cast<uint32_t>(REGION_SIZE);
    auto by = (static_cast<uint32_t>(range->offset.y - user_state.range.offset.y) + range->extent.y + static_cast<uint32_t>(REGION_SIZE - 1)) / static_cast<uint32_t>(REGION_SIZE);
    auto bz = (static_cast<uint32_t>(range->offset.z - user_state.range.offset.z) + range->extent.z + static_cast<uint32_t>(REGION_SIZE - 1)) / static_cast<uint32_t>(REGION_SIZE);

    uint32_t flags = 0;

    for (uint32_t channel_id = 0; channel_id < 32; ++channel_id) {
        if (((1u << channel_id) & channel_flags) != 0) {
            auto &a_channel_header = user_state.region_headers[ax + ay * user_state.r_nx + az * user_state.r_nx * user_state.r_ny].channels[user_state.channel_indices[channel_id]];
            if (a_channel_header.variant_n == 1) {
                flags |= GVOX_REGION_FLAG_UNIFORM;
                for (uint32_t zi = az; zi < bz; ++zi) {
                    for (uint32_t yi = ay; yi < by; ++yi) {
                        for (uint32_t xi = ax; xi < bx; ++xi) {
                            auto &b_channel_header = user_state.region_headers[xi + yi * user_state.r_nx + zi * user_state.r_nx * user_state.r_ny].channels[user_state.channel_indices[channel_id]];
                            if (b_channel_header.variant_n != 1 || b_channel_header.blob_offset != a_channel_header.blob_offset) {
                                flags = 0;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return flags;
}

extern "C" auto gvox_parse_adapter_gvox_palette_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if ((channel_flags & ~user_state.channel_flags) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = *range,
        .channels = channel_flags & user_state.channel_flags,
        .flags = 0u,
        .data = nullptr,
    };
    return region;
}

extern "C" void gvox_parse_adapter_gvox_palette_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

// [output]
extern "C" void gvox_serialize_adapter_gvox_palette_serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t /*channel_flags*/) {
    auto &user_state = *static_cast<GvoxPaletteSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.thread_pool.start();
#define IMPL(axis)                                                                                                                                   \
    auto per_thread_size = range->extent.axis / 32;                                                                                                  \
    if (per_thread_size > 0) {                                                                                                                       \
        for (uint32_t i = 0; i < 32; ++i) {                                                                                                          \
            user_state.thread_pool.enqueue([blit_ctx, &user_state, range, per_thread_size, i]() {                                                    \
                auto temp_range = *range;                                                                                                            \
                temp_range.extent.axis = per_thread_size;                                                                                            \
                temp_range.offset.axis = range->offset.axis + static_cast<int32_t>(per_thread_size * i);                                             \
                handle_region(blit_ctx, user_state, &temp_range, nullptr);                                                                           \
            });                                                                                                                                      \
        }                                                                                                                                            \
    }                                                                                                                                                \
    auto temp_range = *range;                                                                                                                        \
    temp_range.extent.axis = range->extent.axis % 32;                                                                                                \
    temp_range.offset.axis = range->offset.axis + static_cast<int32_t>(user_state.range.extent.axis) - static_cast<int32_t>(temp_range.extent.axis); \
    if (range->extent.axis > 0) {                                                                                                                    \
        handle_region(blit_ctx, user_state, &temp_range, nullptr);                                                                                   \
    }
    auto max_axis = std::max({range->extent.x, range->extent.y, range->extent.z});
    if (max_axis == range->extent.x) {
        IMPL(x)
    } else if (max_axis == range->extent.y) {
        IMPL(y)
    } else {
        IMPL(z)
    }
    while (user_state.thread_pool.busy()) {
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
        std::this_thread::sleep_for(10ms);
#endif
    }
    user_state.thread_pool.stop();
}

// Parse Driven
extern "C" void gvox_parse_adapter_gvox_palette_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if ((channel_flags & ~user_state.channel_flags) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = *range,
        .channels = channel_flags & user_state.channel_flags,
        .flags = 0u,
        .data = nullptr,
    };
    gvox_emit_region(blit_ctx, &region);
}

// [output]
extern "C" void gvox_serialize_adapter_gvox_palette_receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    auto &user_state = *static_cast<GvoxPaletteSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto temp_region = *region;
    handle_region(blit_ctx, user_state, &region->range, &temp_region);
}

#endif
