#include <gvox/gvox.h>
#include <gvox/adapters/serialize/gvox_palette.h>

#include "../shared/gvox_palette.hpp"

#include <cstdint>

#include <bit>
#include <array>
#include <vector>
#include <new>
#include <unordered_set>
#include <algorithm>

struct ChannelHeader {
    uint32_t variant_n;
    uint32_t blob_offset; // if variant_n == 1, this is just the data
};

struct RegionHeader {
    uint32_t channel_n;
};

struct GvoxPaletteSerializeUserState {
    size_t offset{};
    size_t blobs_begin{};
    std::vector<uint8_t> data = {};
};

template <typename T>
static void write_data(uint8_t *&buffer_ptr, T const &data) {
    *reinterpret_cast<T *>(buffer_ptr) = data;
    buffer_ptr += sizeof(T);
}

extern "C" void gvox_serialize_adapter_gvox_palette_begin([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] void *config) {
    auto *user_state_ptr = gvox_adapter_malloc(ctx, sizeof(GvoxPaletteSerializeUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) GvoxPaletteSerializeUserState());
    gvox_serialize_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_serialize_adapter_gvox_palette_end([[maybe_unused]] GvoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<GvoxPaletteSerializeUserState *>(gvox_serialize_adapter_get_user_pointer(ctx));
    user_state.~GvoxPaletteSerializeUserState();
}

auto add_region(GvoxAdapterContext *ctx, GvoxPaletteSerializeUserState &user_state, GvoxRegionRange const &range, uint32_t rx, uint32_t ry, uint32_t rz, uint32_t ci, std::vector<uint8_t> const &channels) -> size_t {
    auto sample_u32_data = [ctx, ci, &channels](GvoxOffset3D const &pos) {
        auto region = gvox_load_region(ctx, &pos, channels[ci]);
        auto result = gvox_sample_region(ctx, &region, &pos, channels[ci]);
        gvox_unload_region(ctx, &region);
        if (channels[ci] == GVOX_CHANNEL_ID_NORMAL) {
            // TODO: potentially compress the normals?
        }
        return result;
    };
    auto tile_set = std::unordered_set<uint32_t>{};
    auto const ox = rx * REGION_SIZE;
    auto const oy = ry * REGION_SIZE;
    auto const oz = rz * REGION_SIZE;
    for (uint32_t zi = 0; zi < REGION_SIZE; ++zi) {
        for (uint32_t yi = 0; yi < REGION_SIZE; ++yi) {
            for (uint32_t xi = 0; xi < REGION_SIZE; ++xi) {
                auto const px = ox + xi;
                auto const py = oy + yi;
                auto const pz = oz + zi;
                auto u32_voxel = 0u;
                if (px < range.extent.x && py < range.extent.y && pz < range.extent.z) {
                    auto pos = GvoxOffset3D{
                        .x = static_cast<int32_t>(px) + range.offset.x,
                        .y = static_cast<int32_t>(py) + range.offset.y,
                        .z = static_cast<int32_t>(pz) + range.offset.z,
                    };
                    u32_voxel = sample_u32_data(pos);
                }
                tile_set.insert(u32_voxel);
            }
        }
    }
    auto const variant_n = static_cast<uint32_t>(tile_set.size());
    auto const bits_per_variant = ceil_log2(variant_n);
    size_t size = 0;
    ChannelHeader region_header{.variant_n = variant_n};
    if (variant_n > 367) {
        size = MAX_REGION_ALLOCATION_SIZE;
        auto const old_size = user_state.data.size();
        region_header.blob_offset = static_cast<uint32_t>(old_size - user_state.blobs_begin);
        user_state.data.resize(old_size + size);
        uint8_t *output_buffer = user_state.data.data() + old_size;
        for (uint32_t zi = 0; zi < REGION_SIZE; ++zi) {
            for (uint32_t yi = 0; yi < REGION_SIZE; ++yi) {
                for (uint32_t xi = 0; xi < REGION_SIZE; ++xi) {
                    auto const px = ox + xi;
                    auto const py = oy + yi;
                    auto const pz = oz + zi;
                    auto u32_voxel = 0u;
                    if (px < range.extent.x && py < range.extent.y && pz < range.extent.z) {
                        auto pos = GvoxOffset3D{
                            .x = static_cast<int32_t>(px) + range.offset.x,
                            .y = static_cast<int32_t>(py) + range.offset.y,
                            .z = static_cast<int32_t>(pz) + range.offset.z,
                        };
                        u32_voxel = sample_u32_data(pos);
                    }
                    write_data<uint32_t>(output_buffer, u32_voxel);
                }
            }
        }
    } else if (variant_n > 1) {
        // insert palette
        size += sizeof(uint32_t) * variant_n;
        // insert palette region
        size += calc_palette_region_size(bits_per_variant);
        auto const old_size = user_state.data.size();
        region_header.blob_offset = static_cast<uint32_t>(old_size - user_state.blobs_begin);
        user_state.data.resize(old_size + size);
        uint8_t *output_buffer = user_state.data.data() + old_size;
        auto *palette_begin = reinterpret_cast<uint32_t *>(output_buffer);
        auto *palette_end = palette_begin + variant_n;
        for (auto u32_voxel : tile_set) {
            write_data<uint32_t>(output_buffer, u32_voxel);
        }
        std::sort(palette_begin, palette_end);
        for (uint32_t zi = 0; zi < REGION_SIZE; ++zi) {
            for (uint32_t yi = 0; yi < REGION_SIZE; ++yi) {
                for (uint32_t xi = 0; xi < REGION_SIZE; ++xi) {
                    auto const px = ox + xi;
                    auto const py = oy + yi;
                    auto const pz = oz + zi;
                    auto const in_region_index = xi + yi * REGION_SIZE + zi * REGION_SIZE * REGION_SIZE;
                    auto u32_voxel = 0u;
                    if (px < range.extent.x && py < range.extent.y && pz < range.extent.z) {
                        auto pos = GvoxOffset3D{
                            .x = static_cast<int32_t>(px) + range.offset.x,
                            .y = static_cast<int32_t>(py) + range.offset.y,
                            .z = static_cast<int32_t>(pz) + range.offset.z,
                        };
                        u32_voxel = sample_u32_data(pos);
                    }
                    auto *palette_iter = std::find(palette_begin, palette_end, u32_voxel);
                    // assert(palette_iter != palette_end);
                    auto const palette_id = static_cast<size_t>(palette_iter - palette_begin);
                    auto const bit_index = static_cast<size_t>(in_region_index * bits_per_variant);
                    auto const byte_index = bit_index / 8;
                    auto const bit_offset = static_cast<uint32_t>(bit_index - byte_index * 8);
                    auto const mask = get_mask(bits_per_variant);
                    // assert(output_buffer + byte_index + 3 < data.data() + data.size());
                    auto &output = *reinterpret_cast<uint32_t *>(output_buffer + byte_index);
                    output = output & ~(mask << bit_offset);
                    output = output | static_cast<uint32_t>(palette_id << bit_offset);
                }
            }
        }
    } else {
        region_header.blob_offset = *tile_set.begin();
    }
    auto region_nx = (range.extent.x + REGION_SIZE - 1) / REGION_SIZE;
    auto region_ny = (range.extent.y + REGION_SIZE - 1) / REGION_SIZE;
    auto *channel_header_ptr =
        user_state.data.data() +
        (rx + ry * region_nx + rz * region_nx * region_ny) *
            (sizeof(RegionHeader) + sizeof(ChannelHeader) * channels.size()) +
        (sizeof(RegionHeader) + sizeof(ChannelHeader) * ci);
    write_data<ChannelHeader>(channel_header_ptr, region_header);
    return size;
}

extern "C" void gvox_serialize_adapter_gvox_palette_serialize_region(GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *reinterpret_cast<GvoxPaletteSerializeUserState *>(gvox_serialize_adapter_get_user_pointer(ctx));

    auto magic = std::bit_cast<uint32_t>(std::array<char, 4>{'g', 'v', 'p', '\0'});
    gvox_output_write(ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(magic);

    gvox_output_write(ctx, user_state.offset, sizeof(*range), range);
    user_state.offset += sizeof(*range);

    gvox_output_write(ctx, user_state.offset, sizeof(channel_flags), &channel_flags);
    user_state.offset += sizeof(channel_flags);

    std::vector<uint8_t> channels;
    channels.resize(static_cast<size_t>(std::popcount(channel_flags)));
    uint32_t next_channel = 0;
    for (uint8_t channel_i = 0; channel_i < 32; ++channel_i) {
        if ((channel_flags & (1u << channel_i)) != 0) {
            channels[next_channel] = channel_i;
            ++next_channel;
        }
    }

    auto region_nx = (range->extent.x + REGION_SIZE - 1) / REGION_SIZE;
    auto region_ny = (range->extent.y + REGION_SIZE - 1) / REGION_SIZE;
    auto region_nz = (range->extent.z + REGION_SIZE - 1) / REGION_SIZE;
    auto size = (sizeof(RegionHeader) + sizeof(ChannelHeader) * channels.size()) * region_nx * region_ny * region_nz;
    user_state.blobs_begin = size;
    auto const two_percent_raw_size = static_cast<size_t>(range->extent.x * range->extent.y * range->extent.z) * sizeof(uint32_t) * channels.size() / 50;
    user_state.data.resize(size + two_percent_raw_size);

    for (uint32_t zi = 0; zi < region_nz; ++zi) {
        for (uint32_t yi = 0; yi < region_ny; ++yi) {
            for (uint32_t xi = 0; xi < region_nx; ++xi) {
                for (uint32_t ci = 0; ci < channels.size(); ++ci) {
                    size += add_region(ctx, user_state, *range, xi, yi, zi, ci, channels);
                }
            }
        }
    }

    gvox_output_write(ctx, user_state.offset, user_state.data.size(), user_state.data.data());
    // for (uint32_t zi = 0; zi < range->extent.z; ++zi) {
    //     for (uint32_t yi = 0; yi < range->extent.y; ++yi) {
    //         for (uint32_t xi = 0; xi < range->extent.x; ++xi) {
    //             auto const pos = GvoxOffset3D{
    //                 static_cast<int32_t>(xi + range->offset.x),
    //                 static_cast<int32_t>(yi + range->offset.y),
    //                 static_cast<int32_t>(zi + range->offset.z),
    //             };
    //             for (uint32_t channel_i = 0; channel_i < channels.size(); ++channel_i) {
    //                 auto region = gvox_load_region(ctx, &pos, channels[channel_i]);
    //                 temp_voxel[channel_i] = gvox_sample_region(ctx, &region, &pos, channels[channel_i]);
    //                 gvox_unload_region(ctx, &region);
    //             }
    //             gvox_output_write(ctx, offset, voxel_size, temp_voxel.data());
    //             offset += voxel_size;
    //         }
    //     }
    // }
}
