#include <gvox/gvox.h>
#include <gvox/adapters/serialize/gvox_raw.h>

#include <cstdlib>

#include <bit>
#include <array>
#include <vector>

struct GvoxRawUserState {
    GvoxRegionRange range{};
    std::vector<uint32_t> voxels;
    std::vector<uint8_t> channels;
    size_t offset{};
};

// Base
extern "C" void gvox_serialize_adapter_gvox_raw_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(GvoxRawUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) GvoxRawUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_serialize_adapter_gvox_raw_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GvoxRawUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~GvoxRawUserState();
    free(&user_state);
}

extern "C" void gvox_serialize_adapter_gvox_raw_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<GvoxRawUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.offset = 0;
    user_state.range = *range;
    auto magic = std::bit_cast<uint32_t>(std::array<char, 4>{'g', 'v', 'r', '\0'});
    gvox_output_write(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(magic);
    gvox_output_write(blit_ctx, user_state.offset, sizeof(*range), range);
    user_state.offset += sizeof(*range);
    gvox_output_write(blit_ctx, user_state.offset, sizeof(channel_flags), &channel_flags);
    user_state.offset += sizeof(channel_flags);
    user_state.channels.resize(static_cast<size_t>(std::popcount(channel_flags)));
    uint32_t next_channel = 0;
    for (uint8_t channel_i = 0; channel_i < 32; ++channel_i) {
        if ((channel_flags & (1u << channel_i)) != 0) {
            user_state.channels[next_channel] = channel_i;
            ++next_channel;
        }
    }
    user_state.voxels.resize(user_state.channels.size() * range->extent.x * range->extent.y * range->extent.z);
}

extern "C" void gvox_serialize_adapter_gvox_raw_blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GvoxRawUserState *>(gvox_adapter_get_user_pointer(ctx));
    gvox_output_write(blit_ctx, user_state.offset, user_state.voxels.size() * sizeof(user_state.voxels[0]), user_state.voxels.data());
}

static void handle_region(GvoxRawUserState &user_state, GvoxRegionRange const *range, auto user_func) {
    for (uint32_t zi = 0; zi < range->extent.z; ++zi) {
        for (uint32_t yi = 0; yi < range->extent.y; ++yi) {
            for (uint32_t xi = 0; xi < range->extent.x; ++xi) {
                auto const pos = GvoxOffset3D{
                    static_cast<int32_t>(xi) + range->offset.x,
                    static_cast<int32_t>(yi) + range->offset.y,
                    static_cast<int32_t>(zi) + range->offset.z,
                };
                if (pos.x < user_state.range.offset.x ||
                    pos.y < user_state.range.offset.y ||
                    pos.z < user_state.range.offset.z ||
                    pos.x >= user_state.range.offset.x + static_cast<int32_t>(user_state.range.extent.x) ||
                    pos.y >= user_state.range.offset.y + static_cast<int32_t>(user_state.range.extent.y) ||
                    pos.z >= user_state.range.offset.z + static_cast<int32_t>(user_state.range.extent.z)) {
                    continue;
                }
                auto output_rel_x = static_cast<size_t>(pos.x - user_state.range.offset.x);
                auto output_rel_y = static_cast<size_t>(pos.y - user_state.range.offset.y);
                auto output_rel_z = static_cast<size_t>(pos.z - user_state.range.offset.z);
                for (uint32_t channel_i = 0; channel_i < user_state.channels.size(); ++channel_i) {
                    auto output_index = static_cast<size_t>(output_rel_x + output_rel_y * user_state.range.extent.x + output_rel_z * user_state.range.extent.x * user_state.range.extent.y) * user_state.channels.size() + channel_i;
                    user_func(channel_i, output_index, pos);
                }
            }
        }
    }
}

// Serialize Driven
extern "C" void gvox_serialize_adapter_gvox_raw_serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t /* channel_flags */) {
    auto &user_state = *static_cast<GvoxRawUserState *>(gvox_adapter_get_user_pointer(ctx));
    handle_region(
        user_state, range,
        [blit_ctx, &user_state](uint32_t channel_i, size_t output_index, GvoxOffset3D const &pos) {
            auto const sample_range = GvoxRegionRange{
                .offset = pos,
                .extent = GvoxExtent3D{1, 1, 1},
            };
            auto region = gvox_load_region_range(blit_ctx, &sample_range, 1u << user_state.channels[channel_i]);
            auto sample = gvox_sample_region(blit_ctx, &region, &pos, user_state.channels[channel_i]);
            if (sample.present == 0u) {
                sample.data = 0u;
            }
            user_state.voxels[output_index] = sample.data;
            gvox_unload_region_range(blit_ctx, &region, &sample_range);
        });
}

// Parse Driven
extern "C" void gvox_serialize_adapter_gvox_raw_receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    auto &user_state = *static_cast<GvoxRawUserState *>(gvox_adapter_get_user_pointer(ctx));
    handle_region(
        user_state, &region->range,
        [blit_ctx, region, &user_state](uint32_t channel_i, size_t output_index, GvoxOffset3D const &pos) {
            auto sample = gvox_sample_region(blit_ctx, region, &pos, user_state.channels[channel_i]);
            if (sample.present != 0u) {
                user_state.voxels[output_index] = sample.data;
            }
        });
}
