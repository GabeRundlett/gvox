#include <gvox/gvox.h>
#include <gvox/adapters/parse/magicavoxel.h>

#define OGT_VOX_IMPLEMENTATION
#include "../shared/ogt_vox.hpp"

#include <cstdlib>
#include <cstring>

#include <bit>
#include <array>
#include <vector>
#include <new>

struct MagicavoxelParseUserState {
};

extern "C" void gvox_parse_adapter_magicavoxel_begin(GvoxAdapterContext *ctx, [[maybe_unused]] void *config) {
    auto *user_state_ptr = malloc(sizeof(MagicavoxelParseUserState));
    auto &user_state = *(new (user_state_ptr) MagicavoxelParseUserState());
    gvox_parse_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_magicavoxel_end([[maybe_unused]] GvoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<MagicavoxelParseUserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    user_state.~MagicavoxelParseUserState();
    free(&user_state);
}

extern "C" auto gvox_parse_adapter_magicavoxel_query_region_flags([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxRegionRange const *range, [[maybe_unused]] uint32_t channel_id) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_magicavoxel_load_region(GvoxAdapterContext *ctx, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxRegion {
    auto &user_state = *reinterpret_cast<MagicavoxelParseUserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    uint32_t voxel_data = 0;
    GvoxRegion const region = {
        .range = {
            .offset = *offset,
            .extent = {1, 1, 1},
        },
        .channels = channel_id,
        .flags = GVOX_REGION_FLAG_UNIFORM,
        .data = reinterpret_cast<void *>(static_cast<size_t>(voxel_data)),
    };
    return region;
}

extern "C" void gvox_parse_adapter_magicavoxel_unload_region([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxRegion *region) {
}

extern "C" auto gvox_parse_adapter_magicavoxel_sample_region([[maybe_unused]] GvoxAdapterContext *ctx, GvoxRegion const *region, [[maybe_unused]] GvoxOffset3D const *offset, uint32_t /*channel_id*/) -> uint32_t {
    return static_cast<uint32_t>(reinterpret_cast<size_t>(region->data));
}
