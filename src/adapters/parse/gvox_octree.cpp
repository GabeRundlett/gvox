#include <gvox/gvox.h>
// #include <gvox/adapters/parse/gvox_octree.h>
#include "../shared/gvox_octree.hpp"

#include <cstdlib>
#include <cstdint>

#include <bit>
#include <array>
#include <vector>
#include <new>

struct OctreeParseUserState {
    size_t offset{};
    Octree gvox_octree{};
};

// Base
extern "C" void gvox_parse_adapter_gvox_octree_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(OctreeParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) OctreeParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_gvox_octree_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<OctreeParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~OctreeParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_gvox_octree_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) {
    auto &user_state = *static_cast<OctreeParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    uint32_t magic = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(uint32_t);

    if (magic != std::bit_cast<uint32_t>(std::array<char, 4>{'o', 'c', 't', '\0'})) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "parsing a RLE format must begin with a valid magic number");
        return;
    }

    gvox_input_read(blit_ctx, user_state.offset, sizeof(GvoxRegionRange), &user_state.gvox_octree.range);
    user_state.offset += sizeof(GvoxRegionRange);

    uint32_t node_n = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(node_n), &node_n);
    user_state.offset += sizeof(node_n);

    user_state.gvox_octree.nodes.resize(node_n);
    gvox_input_read(blit_ctx, user_state.offset, sizeof(user_state.gvox_octree.nodes[0]) * node_n, user_state.gvox_octree.nodes.data());
}

extern "C" void gvox_parse_adapter_gvox_octree_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

// General
extern "C" auto gvox_parse_adapter_gvox_octree_query_details() -> GvoxParseAdapterDetails {
    return {
        .preferred_blit_mode = GVOX_BLIT_MODE_DONT_CARE,
    };
}

extern "C" auto gvox_parse_adapter_gvox_octree_query_parsable_range(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) -> GvoxRegionRange {
    auto &user_state = *static_cast<OctreeParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    return user_state.gvox_octree.range;
}

extern "C" auto gvox_parse_adapter_gvox_octree_sample_region(GvoxBlitContext * /*blit_ctx*/, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t /*channel_id*/) -> GvoxSample {
    auto &user_state = *static_cast<OctreeParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if (user_state.gvox_octree.nodes.size() == 1) {
        return {user_state.gvox_octree.nodes[0].leaf.color, 1u};
    }
    auto x_pos = static_cast<uint32_t>(offset->x - user_state.gvox_octree.range.offset.x);
    auto y_pos = static_cast<uint32_t>(offset->y - user_state.gvox_octree.range.offset.y);
    auto z_pos = static_cast<uint32_t>(offset->z - user_state.gvox_octree.range.offset.z);
    uint32_t voxel_data = user_state.gvox_octree.sample(user_state.gvox_octree.nodes[0].parent, x_pos, y_pos, z_pos);
    return {voxel_data, 1u};
}

// Serialize Driven
extern "C" auto gvox_parse_adapter_gvox_octree_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_gvox_octree_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    // auto &user_state = *static_cast<OctreeParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if ((channel_flags & ~static_cast<uint32_t>(GVOX_CHANNEL_BIT_COLOR)) != 0u) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = *range,
        .channels = channel_flags & GVOX_CHANNEL_BIT_COLOR,
        .flags = 0u,
        .data = nullptr,
    };
    return region;
}

extern "C" void gvox_parse_adapter_gvox_octree_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

// Parse Driven
extern "C" void gvox_parse_adapter_gvox_octree_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    // auto &user_state = *static_cast<OctreeParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if ((channel_flags & ~static_cast<uint32_t>(GVOX_CHANNEL_BIT_COLOR)) != 0u) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = *range,
        .channels = channel_flags & GVOX_CHANNEL_BIT_COLOR,
        .flags = 0u,
        .data = nullptr,
    };
    gvox_emit_region(blit_ctx, &region);
}
