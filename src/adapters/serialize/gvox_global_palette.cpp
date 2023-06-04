#include <gvox/gvox.h>
// #include <gvox/adapters/serialize/gvox_global_palette.h>

#include <cstdlib>

#include <bit>
#include <array>
#include <vector>
#include <set>
#include <algorithm>

#include "../shared/math_helpers.hpp"
#include "../shared/thread_pool.hpp"

struct GlobalPaletteUserState {
    GvoxRegionRange range{};
    std::vector<uint32_t> voxels;
    std::vector<uint8_t> channels;
    size_t offset{};
    std::vector<std::set<uint32_t>> unique_values;
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
    std::mutex palette_mutex;
#endif
};

// Base
extern "C" void gvox_serialize_adapter_gvox_global_palette_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(GlobalPaletteUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) GlobalPaletteUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_serialize_adapter_gvox_global_palette_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GlobalPaletteUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~GlobalPaletteUserState();
    free(&user_state);
}

extern "C" void gvox_serialize_adapter_gvox_global_palette_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<GlobalPaletteUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.offset = 0;
    user_state.range = *range;
    auto magic = std::bit_cast<uint64_t>(std::array<char, 8>{'g', 'v', 'g', 'l', 'b', 'p', 'a', 'l'});
    gvox_output_write(blit_ctx, user_state.offset, sizeof(magic), &magic);
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
    user_state.unique_values.resize(user_state.channels.size());
    for (auto &unique_values : user_state.unique_values) {
        unique_values.insert(0u);
    }
    user_state.voxels.resize(user_state.channels.size() * range->extent.x * range->extent.y * range->extent.z);
}

extern "C" void gvox_serialize_adapter_gvox_global_palette_blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GlobalPaletteUserState *>(gvox_adapter_get_user_pointer(ctx));

    auto sorted_unique_values_lists = std::vector<std::vector<uint32_t>>{};
    sorted_unique_values_lists.resize(user_state.channels.size());

    for (size_t i = 0; i < user_state.channels.size(); ++i) {
        auto set_size = static_cast<uint32_t>(user_state.unique_values[i].size());
        sorted_unique_values_lists[i].reserve(set_size);
        gvox_output_write(blit_ctx, user_state.offset, sizeof(set_size), &set_size);
        user_state.offset += sizeof(set_size);
        for (auto const &val : user_state.unique_values[i]) {
            sorted_unique_values_lists[i].push_back(val);
        }
    }

    auto voxel_n = user_state.range.extent.x * user_state.range.extent.y * user_state.range.extent.z;

    for (size_t ci = 0; ci < user_state.channels.size(); ++ci) {
        auto &sorted_unique_values = sorted_unique_values_lists[ci];
        std::sort(sorted_unique_values.begin(), sorted_unique_values.end());

        std::vector<uint32_t> bits{};
        auto bits_per_voxel = ceil_log2(static_cast<uint32_t>(sorted_unique_values.size()));
        auto voxels_per_element = 8 * sizeof(bits[0]) / bits_per_voxel;
        bits.resize((voxel_n + voxels_per_element - 1) / voxels_per_element);
        auto voxel_mask = (1u << bits_per_voxel) - 1;

        for (uint32_t zi = 0; zi < user_state.range.extent.z; ++zi) {
            for (uint32_t yi = 0; yi < user_state.range.extent.y; ++yi) {
                for (uint32_t xi = 0; xi < user_state.range.extent.x; ++xi) {
                    auto voxel_i = xi + yi * user_state.range.extent.x + zi * user_state.range.extent.x * user_state.range.extent.y;
                    auto element_i = voxel_i / voxels_per_element;
                    auto element_offset = (voxel_i - element_i * voxels_per_element) * bits_per_voxel;
                    auto my_voxel = user_state.voxels[voxel_i * user_state.channels.size() + ci];
                    auto palette_index = static_cast<uint32_t>(std::lower_bound(sorted_unique_values.begin(), sorted_unique_values.end(), my_voxel) - sorted_unique_values.begin());
                    bits[element_i] |= (palette_index & voxel_mask) << element_offset;
                }
            }
        }

        gvox_output_write(blit_ctx, user_state.offset, sorted_unique_values.size() * sizeof(sorted_unique_values[0]), sorted_unique_values.data());
        user_state.offset += sorted_unique_values.size() * sizeof(sorted_unique_values[0]);

        gvox_output_write(blit_ctx, user_state.offset, bits.size() * sizeof(bits[0]), bits.data());
        user_state.offset += bits.size() * sizeof(bits[0]);
    }
}

static void handle_region(GlobalPaletteUserState &user_state, GvoxRegionRange const *range, auto user_func) {
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
extern "C" void gvox_serialize_adapter_gvox_global_palette_serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t /* channel_flags */) {
    auto &user_state = *static_cast<GlobalPaletteUserState *>(gvox_adapter_get_user_pointer(ctx));
    handle_region(
        user_state, range,
        [blit_ctx, &user_state](uint32_t channel_i, size_t output_index, GvoxOffset3D const &pos) {
            auto const sample_range = GvoxRegionRange{
                .offset = pos,
                .extent = GvoxExtent3D{1, 1, 1},
            };
            auto region = gvox_load_region_range(blit_ctx, &sample_range, 1u << user_state.channels[channel_i]);
            auto sample = gvox_sample_region(blit_ctx, &region, &pos, user_state.channels[channel_i]);
            if (sample.is_present == 0u) {
                sample.data = 0u;
            }
            user_state.voxels[output_index] = sample.data;
            user_state.unique_values[channel_i].insert(sample.data);
            gvox_unload_region_range(blit_ctx, &region, &sample_range);
        });
}

// Parse Driven
extern "C" void gvox_serialize_adapter_gvox_global_palette_receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    auto &user_state = *static_cast<GlobalPaletteUserState *>(gvox_adapter_get_user_pointer(ctx));
    handle_region(
        user_state, &region->range,
        [blit_ctx, region, &user_state](uint32_t channel_i, size_t output_index, GvoxOffset3D const &pos) {
            auto sample = gvox_sample_region(blit_ctx, region, &pos, user_state.channels[channel_i]);
            if (sample.is_present != 0u) {
                user_state.voxels[output_index] = sample.data;
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
                auto lock = std::lock_guard{user_state.palette_mutex};
#endif
                user_state.unique_values[channel_i].insert(sample.data);
            }
        });
}
