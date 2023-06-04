#include <gvox/gvox.h>
// #include <gvox/adapters/serialize/gvox_brickmap.h>

#include <cstdlib>

#include <bit>
#include <vector>
#include <memory>

#include "../shared/gvox_brickmap.hpp"
#include "../shared/thread_pool.hpp"

struct TempBrickInfo {
    uint32_t first_voxel;
    uint32_t state = 0;
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS
    std::mutex access_mtx;
#endif
};

struct BrickmapUserState {
    GvoxRegionRange range{};
    std::vector<uint32_t> voxels;
    std::vector<uint8_t> channels;
    size_t offset{};
    GvoxExtent3D bricks_extent{};
    std::unique_ptr<std::vector<TempBrickInfo>> temp_brick_infos{};
    uint32_t brick_heap_size{};
};

// Base
extern "C" void gvox_serialize_adapter_gvox_brickmap_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(BrickmapUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) BrickmapUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_serialize_adapter_gvox_brickmap_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<BrickmapUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~BrickmapUserState();
    free(&user_state);
}

extern "C" void gvox_serialize_adapter_gvox_brickmap_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<BrickmapUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.offset = 0;
    user_state.range = *range;
    auto magic = std::bit_cast<uint32_t>(std::array<char, 4>{'b', 'r', 'k', '\0'});
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
    user_state.bricks_extent.x = (range->extent.x + 7) / 8;
    user_state.bricks_extent.y = (range->extent.y + 7) / 8;
    user_state.bricks_extent.z = (range->extent.z + 7) / 8;
    user_state.temp_brick_infos = std::make_unique<std::vector<TempBrickInfo>>(user_state.channels.size() * user_state.bricks_extent.x * user_state.bricks_extent.y * user_state.bricks_extent.z);
}

extern "C" void gvox_serialize_adapter_gvox_brickmap_blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<BrickmapUserState *>(gvox_adapter_get_user_pointer(ctx));
    std::vector<Brick> bricks_heap{};
    std::vector<BrickmapHeader> brick_headers{};
    brick_headers.resize(user_state.bricks_extent.x * user_state.bricks_extent.y * user_state.bricks_extent.z * user_state.channels.size());
    bricks_heap.reserve(user_state.brick_heap_size);
    for (uint32_t ci = 0; ci < user_state.channels.size(); ++ci) {
        for (uint32_t bzi = 0; bzi < user_state.bricks_extent.z; ++bzi) {
            for (uint32_t byi = 0; byi < user_state.bricks_extent.y; ++byi) {
                for (uint32_t bxi = 0; bxi < user_state.bricks_extent.x; ++bxi) {
                    auto brick_index = bxi + byi * user_state.bricks_extent.x + bzi * user_state.bricks_extent.x * user_state.bricks_extent.y;
                    auto &temp_brick = (*user_state.temp_brick_infos)[brick_index * user_state.channels.size() + ci];
                    auto &brick_header = brick_headers[brick_index + ci * user_state.bricks_extent.x * user_state.bricks_extent.y * user_state.bricks_extent.z];
                    auto brick_result = Brick{};
                    auto first_voxel = uint32_t{};
                    for (uint32_t zi = 0; zi < 8; ++zi) {
                        for (uint32_t yi = 0; yi < 8; ++yi) {
                            for (uint32_t xi = 0; xi < 8; ++xi) {
                                auto out_index = xi + yi * 8 + zi * 64;
                                auto &out_voxel = brick_result.voxels[out_index];
                                auto gxi = xi + bxi * 8;
                                auto gyi = yi + byi * 8;
                                auto gzi = zi + bzi * 8;
                                auto in_index = gxi + gyi * user_state.range.extent.x + gzi * user_state.range.extent.x * user_state.range.extent.y;
                                if (gxi < user_state.range.extent.x &&
                                    gyi < user_state.range.extent.y &&
                                    gzi < user_state.range.extent.z) {
                                    out_voxel = user_state.voxels[in_index * user_state.channels.size() + ci];
                                } else {
                                    out_voxel = 0u;
                                }
                                if (in_index == 0) {
                                    first_voxel = out_voxel;
                                }
                                if (!brick_header.loaded.is_loaded && out_voxel != first_voxel) {
                                    brick_header.loaded.is_loaded = true;
                                }
                            }
                        }
                    }
                    if (brick_header.loaded.is_loaded) {
                        brick_header.loaded.heap_index = static_cast<uint32_t>(bricks_heap.size());
                        bricks_heap.push_back(brick_result);
                    } else {
                        brick_header.unloaded.lod_color = temp_brick.first_voxel;
                    }
                }
            }
        }
    }

    user_state.brick_heap_size = static_cast<uint32_t>(bricks_heap.size());

    gvox_output_write(blit_ctx, user_state.offset, sizeof(user_state.brick_heap_size), &user_state.brick_heap_size);
    user_state.offset += sizeof(user_state.brick_heap_size);

    gvox_output_write(blit_ctx, user_state.offset, brick_headers.size() * sizeof(brick_headers[0]), brick_headers.data());
    user_state.offset += brick_headers.size() * sizeof(brick_headers[0]);
    gvox_output_write(blit_ctx, user_state.offset, bricks_heap.size() * sizeof(bricks_heap[0]), bricks_heap.data());
    user_state.offset += bricks_heap.size() * sizeof(bricks_heap[0]);
}

static void handle_region(BrickmapUserState &user_state, GvoxRegionRange const *range, auto user_func) {
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
extern "C" void gvox_serialize_adapter_gvox_brickmap_serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t /* channel_flags */) {
    auto &user_state = *static_cast<BrickmapUserState *>(gvox_adapter_get_user_pointer(ctx));
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
            gvox_unload_region_range(blit_ctx, &region, &sample_range);
        });
}

// Parse Driven
extern "C" void gvox_serialize_adapter_gvox_brickmap_receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    auto &user_state = *static_cast<BrickmapUserState *>(gvox_adapter_get_user_pointer(ctx));
    handle_region(
        user_state, &region->range,
        [blit_ctx, region, &user_state](uint32_t channel_i, size_t output_index, GvoxOffset3D const &pos) {
            auto sample = gvox_sample_region(blit_ctx, region, &pos, user_state.channels[channel_i]);
            if (sample.is_present != 0u) {
                user_state.voxels[output_index] = sample.data;
            }
        });
}
