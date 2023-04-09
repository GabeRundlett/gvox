#include <gvox/gvox.h>
#include <gvox/adapters/parse/goxel.h>

#include <cstdlib>
#include <cstring>

#include <bit>
#include <array>
#include <vector>
#include <new>

struct GoxelParseUserState {
    GvoxRegionRange range{};
    uint32_t channel_flags{};
    uint32_t channel_n{};
    size_t offset{};
};

// Base
extern "C" void gvox_parse_adapter_goxel_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(GoxelParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) GoxelParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_goxel_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~GoxelParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_goxel_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) {
    auto &user_state = *static_cast<GoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    uint32_t magic = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(uint32_t);

    if (magic != std::bit_cast<uint32_t>(std::array<char, 4>{'G', 'O', 'X', ' '})) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "parsing a goxel file with an invalid magic number");
        return;
    }

    uint32_t version = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &version);
    user_state.offset += sizeof(uint32_t);

    if (version != 2) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unknown goxel file version");
        return;
    }

    for (uint32_t i = 0; i < 100; ++i) {
        std::array<char, 4> type = {};
        uint32_t data_length = 0;
        uint32_t crc = 0;

        gvox_input_read(blit_ctx, user_state.offset, sizeof(type), &type);
        user_state.offset += sizeof(uint32_t);
        gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &data_length);
        user_state.offset += sizeof(uint32_t);

        user_state.offset += data_length;

        gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &crc);
        user_state.offset += sizeof(uint32_t);
    }
}

extern "C" void gvox_parse_adapter_goxel_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

// General
extern "C" auto gvox_parse_adapter_goxel_query_details() -> GvoxParseAdapterDetails {
    return {
        .preferred_blit_mode = GVOX_BLIT_MODE_DONT_CARE,
    };
}

extern "C" auto gvox_parse_adapter_goxel_query_parsable_range(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) -> GvoxRegionRange {
    auto &user_state = *static_cast<GoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    return user_state.range;
}

extern "C" auto gvox_parse_adapter_goxel_sample_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxSample {
    auto &user_state = *static_cast<GoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto base_offset = user_state.offset;
    uint32_t voxel_data = 0;
    uint32_t voxel_channel_index = 0;
    for (uint32_t channel_index = 0; channel_index < channel_id; ++channel_index) {
        if (((user_state.channel_flags >> channel_index) & 0x1) != 0) {
            ++voxel_channel_index;
        }
    }
    auto read_offset = base_offset + sizeof(uint32_t) * (voxel_channel_index + user_state.channel_n * (static_cast<size_t>(offset->x - user_state.range.offset.x) + static_cast<size_t>(offset->y - user_state.range.offset.y) * user_state.range.extent.x + static_cast<size_t>(offset->z - user_state.range.offset.z) * user_state.range.extent.x * user_state.range.extent.y));
    gvox_input_read(blit_ctx, read_offset, sizeof(voxel_data), &voxel_data);
    return {voxel_data, 1u};
}

// Serialize Driven
extern "C" auto gvox_parse_adapter_goxel_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_goxel_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    auto &user_state = *static_cast<GoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
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

extern "C" void gvox_parse_adapter_goxel_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

// Parse Driven
extern "C" void gvox_parse_adapter_goxel_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<GoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
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
