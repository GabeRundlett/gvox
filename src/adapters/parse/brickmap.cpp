#include <gvox/gvox.h>
// #include <gvox/adapters/parse/brickmap.h>

#include <cstdlib>
#include <cstdint>

#include <bit>
#include <array>
#include <vector>
#include <new>

#include "../shared/brickmap.hpp"

struct BrickmapParseUserState {
    GvoxRegionRange range{};
    uint32_t channel_flags{};
    uint32_t channel_n{};
    size_t offset{};

    GvoxExtent3D bricks_extent{};
    std::vector<Brick> bricks_heap{};
    std::vector<BrickmapHeader> brick_headers{};
};

// Base
extern "C" void gvox_parse_adapter_brickmap_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(BrickmapParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) BrickmapParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_brickmap_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<BrickmapParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~BrickmapParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_brickmap_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) {
    auto &user_state = *static_cast<BrickmapParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    uint32_t magic = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(uint32_t);

    if (magic != std::bit_cast<uint32_t>(std::array<char, 4>{'b', 'r', 'k', '\0'})) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "parsing a gvox raw format must begin with a valid magic number");
        return;
    }

    gvox_input_read(blit_ctx, user_state.offset, sizeof(GvoxRegionRange), &user_state.range);
    user_state.offset += sizeof(GvoxRegionRange);

    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &user_state.channel_flags);
    user_state.offset += sizeof(uint32_t);

    uint32_t heap_size = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(heap_size), &heap_size);
    user_state.offset += sizeof(heap_size);

    user_state.channel_n = static_cast<uint32_t>(std::popcount(user_state.channel_flags));

    user_state.bricks_extent.x = (user_state.range.extent.x + 7) / 8;
    user_state.bricks_extent.y = (user_state.range.extent.y + 7) / 8;
    user_state.bricks_extent.z = (user_state.range.extent.z + 7) / 8;

    user_state.brick_headers.resize(user_state.channel_n * user_state.bricks_extent.x * user_state.bricks_extent.y * user_state.bricks_extent.z);
    user_state.bricks_heap.resize(heap_size);

    gvox_input_read(blit_ctx, user_state.offset, user_state.brick_headers.size() * sizeof(user_state.brick_headers[0]), user_state.brick_headers.data());
    user_state.offset += user_state.brick_headers.size() * sizeof(user_state.brick_headers[0]);

    gvox_input_read(blit_ctx, user_state.offset, user_state.bricks_heap.size() * sizeof(user_state.bricks_heap[0]), user_state.bricks_heap.data());
    user_state.offset += user_state.bricks_heap.size() * sizeof(user_state.bricks_heap[0]);
}

extern "C" void gvox_parse_adapter_brickmap_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

// General
extern "C" auto gvox_parse_adapter_brickmap_query_details() -> GvoxParseAdapterDetails {
    return {
        .preferred_blit_mode = GVOX_BLIT_MODE_DONT_CARE,
    };
}

extern "C" auto gvox_parse_adapter_brickmap_query_parsable_range(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) -> GvoxRegionRange {
    auto &user_state = *static_cast<BrickmapParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    return user_state.range;
}

extern "C" auto gvox_parse_adapter_brickmap_sample_region(GvoxBlitContext * /*blit_ctx*/, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxSample {
    auto &user_state = *static_cast<BrickmapParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    uint32_t voxel_data = 0;
    uint32_t voxel_channel_index = 0;
    for (uint32_t channel_index = 0; channel_index < channel_id; ++channel_index) {
        if (((user_state.channel_flags >> channel_index) & 0x1) != 0) {
            ++voxel_channel_index;
        }
    }

    auto xi = static_cast<uint32_t>(offset->x - user_state.range.offset.x);
    auto yi = static_cast<uint32_t>(offset->y - user_state.range.offset.y);
    auto zi = static_cast<uint32_t>(offset->z - user_state.range.offset.z);
    auto bxi = xi / 8;
    auto byi = yi / 8;
    auto bzi = zi / 8;

    auto brick_index = bxi + byi * user_state.bricks_extent.x + bzi * user_state.bricks_extent.x * user_state.bricks_extent.y;
    auto const &brick_header = user_state.brick_headers[brick_index * user_state.channel_n + voxel_channel_index];

    if (brick_header.loaded.is_loaded) {
        auto sub_bxi = xi - bxi * 8;
        auto sub_byi = yi - byi * 8;
        auto sub_bzi = zi - bzi * 8;
        voxel_data = user_state.bricks_heap[brick_header.loaded.heap_index].voxels[sub_bxi + sub_byi * 8 + sub_bzi * 64];
    } else {
        voxel_data = brick_header.unloaded.lod_color;
    }

    return {voxel_data, 1u};
}

// Serialize Driven
extern "C" auto gvox_parse_adapter_brickmap_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_brickmap_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    auto &user_state = *static_cast<BrickmapParseUserState *>(gvox_adapter_get_user_pointer(ctx));
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

extern "C" void gvox_parse_adapter_brickmap_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

// Parse Driven
extern "C" void gvox_parse_adapter_brickmap_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<BrickmapParseUserState *>(gvox_adapter_get_user_pointer(ctx));
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
