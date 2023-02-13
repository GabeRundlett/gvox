#include <gvox/gvox.h>
#include <gvox/adapters/parse/gvox_raw.h>

#include <cstdlib>
#include <cstring>

#include <bit>
#include <array>
#include <vector>
#include <new>

struct GvoxRawParseUserState {
    GvoxRegionRange range{};
    uint32_t channel_flags{};
    uint32_t channel_n{};
    size_t offset{};
};

extern "C" void gvox_parse_adapter_gvox_raw_create(GvoxAdapterContext *ctx, void * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(GvoxRawParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) GvoxRawParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_gvox_raw_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GvoxRawParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~GvoxRawParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_gvox_raw_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GvoxRawParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    uint32_t magic = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(uint32_t);

    if (magic != std::bit_cast<uint32_t>(std::array<char, 4>{'g', 'v', 'r', '\0'})) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "parsing a gvox raw format must begin with a valid magic number");
        return;
    }

    gvox_input_read(blit_ctx, user_state.offset, sizeof(GvoxRegionRange), &user_state.range);
    user_state.offset += sizeof(GvoxRegionRange);

    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &user_state.channel_flags);
    user_state.offset += sizeof(uint32_t);

    user_state.channel_n = static_cast<uint32_t>(std::popcount(user_state.channel_flags));
}

extern "C" void gvox_parse_adapter_gvox_raw_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

extern "C" auto gvox_parse_adapter_gvox_raw_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_gvox_raw_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    auto &user_state = *static_cast<GvoxRawParseUserState *>(gvox_adapter_get_user_pointer(ctx));
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

extern "C" void gvox_parse_adapter_gvox_raw_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

extern "C" auto gvox_parse_adapter_gvox_raw_sample_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> uint32_t {
    auto &user_state = *static_cast<GvoxRawParseUserState *>(gvox_adapter_get_user_pointer(ctx));
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
    return voxel_data;
}
