#include <gvox/gvox.h>
// #include <gvox/adapters/parse/global_palette.h>

#include <cstdlib>
#include <cstdint>

#include <bit>
#include <array>
#include <vector>
#include <new>

#include "../shared/math_helpers.hpp"

struct Volume {
    std::vector<uint32_t> palette{};
    std::vector<uint32_t> voxels{};
};

struct GlobalPaletteParseUserState {
    GvoxRegionRange range{};
    uint32_t channel_flags{};
    uint32_t channel_n{};
    size_t offset{};
    std::vector<Volume> channel_volumes{};
};

// Base
extern "C" void gvox_parse_adapter_global_palette_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(GlobalPaletteParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) GlobalPaletteParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_global_palette_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GlobalPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~GlobalPaletteParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_global_palette_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) {
    auto &user_state = *static_cast<GlobalPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    uint32_t magic = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(uint32_t);

    if (magic != std::bit_cast<uint32_t>(std::array<char, 4>{'g', 'l', 'p', '\0'})) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "parsing a global palette must begin with a valid magic number");
        return;
    }

    gvox_input_read(blit_ctx, user_state.offset, sizeof(GvoxRegionRange), &user_state.range);
    user_state.offset += sizeof(GvoxRegionRange);

    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &user_state.channel_flags);
    user_state.offset += sizeof(uint32_t);

    user_state.channel_n = static_cast<uint32_t>(std::popcount(user_state.channel_flags));
    user_state.channel_volumes.resize(user_state.channel_n);

    auto voxel_n = user_state.range.extent.x * user_state.range.extent.y * user_state.range.extent.z;
    for (uint32_t ci = 0; ci < user_state.channel_n; ++ci) {
        uint32_t palette_size = 0;
        gvox_input_read(blit_ctx, user_state.offset, sizeof(palette_size), &palette_size);
        user_state.offset += sizeof(palette_size);
        auto &palette = user_state.channel_volumes[ci].palette;
        palette.resize(palette_size);
    }
    for (uint32_t ci = 0; ci < user_state.channel_n; ++ci) {
        auto &palette = user_state.channel_volumes[ci].palette;
        auto &voxels = user_state.channel_volumes[ci].voxels;
        auto palette_size = static_cast<uint32_t>(palette.size());
        auto bits_per_voxel = ceil_log2(palette_size);
        auto voxels_per_element = static_cast<uint32_t>(8 * sizeof(voxels[0]) / bits_per_voxel);
        voxels.resize((voxel_n + voxels_per_element - 1) / voxels_per_element);

        gvox_input_read(blit_ctx, user_state.offset, palette.size() * sizeof(palette[0]), palette.data());
        user_state.offset += palette.size() * sizeof(palette[0]);

        gvox_input_read(blit_ctx, user_state.offset, voxels.size() * sizeof(voxels[0]), voxels.data());
        user_state.offset += voxels.size() * sizeof(voxels[0]);
    }
}

extern "C" void gvox_parse_adapter_global_palette_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

// General
extern "C" auto gvox_parse_adapter_global_palette_query_details() -> GvoxParseAdapterDetails {
    return {
        .preferred_blit_mode = GVOX_BLIT_MODE_DONT_CARE,
    };
}

extern "C" auto gvox_parse_adapter_global_palette_query_parsable_range(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) -> GvoxRegionRange {
    auto &user_state = *static_cast<GlobalPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    return user_state.range;
}

extern "C" auto gvox_parse_adapter_global_palette_sample_region(GvoxBlitContext * /*blit_ctx*/, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxSample {
    auto &user_state = *static_cast<GlobalPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    uint32_t voxel_data = 0;
    uint32_t voxel_channel_index = 0;
    for (uint32_t channel_index = 0; channel_index < channel_id; ++channel_index) {
        if (((user_state.channel_flags >> channel_index) & 0x1) != 0) {
            ++voxel_channel_index;
        }
    }

    auto &palette = user_state.channel_volumes[voxel_channel_index].palette;
    auto &voxels = user_state.channel_volumes[voxel_channel_index].voxels;
    auto xi = static_cast<uint32_t>(offset->x - user_state.range.offset.x);
    auto yi = static_cast<uint32_t>(offset->y - user_state.range.offset.y);
    auto zi = static_cast<uint32_t>(offset->z - user_state.range.offset.z);
    auto bits_per_voxel = ceil_log2(static_cast<uint32_t>(palette.size()));
    auto voxels_per_element = static_cast<uint32_t>(8 * sizeof(voxels[0]) / bits_per_voxel);

    auto voxel_i = xi + yi * user_state.range.extent.x + zi * user_state.range.extent.x * user_state.range.extent.y;
    auto voxel_mask = (1u << bits_per_voxel) - 1;
    auto element_i = voxel_i / voxels_per_element;
    auto element_offset = (voxel_i - element_i * voxels_per_element) * bits_per_voxel;
    voxel_data = palette[(voxels[voxel_i / voxels_per_element] >> element_offset) & voxel_mask];

    return {voxel_data, 1u};
}

// Serialize Driven
extern "C" auto gvox_parse_adapter_global_palette_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_global_palette_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    auto &user_state = *static_cast<GlobalPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
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

extern "C" void gvox_parse_adapter_global_palette_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

// Parse Driven
extern "C" void gvox_parse_adapter_global_palette_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<GlobalPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
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
