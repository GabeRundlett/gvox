#include <gvox/gvox.h>
#include <gvox/adapters/parse/gvox_palette.h>

#include "../shared/gvox_palette.hpp"

#include <cstdlib>
#include <cstring>

#include <bit>
#include <vector>
#include <new>

struct LoadedRegionHeader {
    std::vector<ChannelHeader> channels;
};

struct GvoxPaletteParseUserState {
    GvoxRegionRange range{};
    uint32_t blob_size{};
    uint32_t channel_flags{};
    uint32_t channel_n{};

    size_t offset{};
    uint32_t r_nx{};
    uint32_t r_ny{};
    uint32_t r_nz{};

    std::vector<LoadedRegionHeader> region_headers{};
    std::vector<uint8_t> buffer{};

    std::array<uint32_t, 32> channel_indices{};
};

extern "C" void gvox_parse_adapter_gvox_palette_create(GvoxAdapterContext *ctx, void * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(GvoxPaletteParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) GvoxPaletteParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_gvox_palette_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~GvoxPaletteParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_gvox_palette_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    uint32_t magic = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(uint32_t);

    if (magic != std::bit_cast<uint32_t>(std::array<char, 4>{'g', 'v', 'p', '\0'})) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "parsing a gvox palette format must begin with a valid magic number");
        return;
    }

    gvox_input_read(blit_ctx, user_state.offset, sizeof(GvoxRegionRange), &user_state.range);
    user_state.offset += sizeof(GvoxRegionRange);

    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &user_state.blob_size);
    user_state.offset += sizeof(uint32_t);

    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &user_state.channel_flags);
    user_state.offset += sizeof(uint32_t);

    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &user_state.channel_n);
    user_state.offset += sizeof(uint32_t);

    uint32_t next_channel = 0;
    for (uint8_t channel_i = 0; channel_i < 32; ++channel_i) {
        if ((user_state.channel_flags & (1u << channel_i)) != 0) {
            user_state.channel_indices[channel_i] = next_channel;
            ++next_channel;
        }
    }

    user_state.r_nx = (user_state.range.extent.x + REGION_SIZE - 1) / REGION_SIZE;
    user_state.r_ny = (user_state.range.extent.y + REGION_SIZE - 1) / REGION_SIZE;
    user_state.r_nz = (user_state.range.extent.z + REGION_SIZE - 1) / REGION_SIZE;

    user_state.region_headers.resize(user_state.r_nx * user_state.r_ny * user_state.r_nz);
    for (auto &region_header : user_state.region_headers) {
        region_header.channels.resize(user_state.channel_n);
        for (auto &channel_header : region_header.channels) {
            gvox_input_read(blit_ctx, user_state.offset, sizeof(ChannelHeader), &channel_header);
            user_state.offset += sizeof(ChannelHeader);
        }
    }

    user_state.buffer.resize(user_state.blob_size);
    gvox_input_read(blit_ctx, user_state.offset, user_state.blob_size, user_state.buffer.data());
}

extern "C" void gvox_parse_adapter_gvox_palette_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

extern "C" auto gvox_parse_adapter_gvox_palette_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> uint32_t {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    if ((channel_flags & user_state.channel_flags) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
        return 0;
    }

    auto ax = static_cast<uint32_t>(range->offset.x - user_state.range.offset.x) / REGION_SIZE;
    auto ay = static_cast<uint32_t>(range->offset.y - user_state.range.offset.y) / REGION_SIZE;
    auto az = static_cast<uint32_t>(range->offset.z - user_state.range.offset.z) / REGION_SIZE;
    auto bx = (static_cast<uint32_t>(range->offset.x - user_state.range.offset.x) + range->extent.x + (REGION_SIZE - 1)) / REGION_SIZE;
    auto by = (static_cast<uint32_t>(range->offset.y - user_state.range.offset.y) + range->extent.y + (REGION_SIZE - 1)) / REGION_SIZE;
    auto bz = (static_cast<uint32_t>(range->offset.z - user_state.range.offset.z) + range->extent.z + (REGION_SIZE - 1)) / REGION_SIZE;

    uint32_t flags = 0;

    for (uint32_t channel_id = 0; channel_id < 32; ++channel_id) {
        if (((1u << channel_id) & channel_flags) != 0) {
            auto &a_channel_header = user_state.region_headers[ax + ay * user_state.r_nx + az * user_state.r_nx * user_state.r_ny].channels[user_state.channel_indices[channel_id]];
            if (a_channel_header.variant_n == 1) {
                flags |= GVOX_REGION_FLAG_UNIFORM;
                for (uint32_t zi = az; zi < bz; ++zi) {
                    for (uint32_t yi = ay; yi < by; ++yi) {
                        for (uint32_t xi = ax; xi < bx; ++xi) {
                            auto &b_channel_header = user_state.region_headers[xi + yi * user_state.r_nx + zi * user_state.r_nx * user_state.r_ny].channels[user_state.channel_indices[channel_id]];
                            if (b_channel_header.variant_n != 1 || b_channel_header.blob_offset != a_channel_header.blob_offset) {
                                flags = 0;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    return flags;
}

extern "C" auto gvox_parse_adapter_gvox_palette_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const * /*unused*/, uint32_t channel_flags) -> GvoxRegion {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if ((channel_flags & ~user_state.channel_flags) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = {
            .offset = {0, 0, 0},
            .extent = {
                user_state.r_nx * REGION_SIZE,
                user_state.r_ny * REGION_SIZE,
                user_state.r_nz * REGION_SIZE,
            },
        },
        .channels = channel_flags & user_state.channel_flags,
        .flags = 0,
        .data = nullptr,
    };
    return region;
}

extern "C" void gvox_parse_adapter_gvox_palette_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

extern "C" auto gvox_parse_adapter_gvox_palette_sample_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> uint32_t {
    auto &user_state = *static_cast<GvoxPaletteParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto const xi = static_cast<uint32_t>(offset->x - user_state.range.offset.x) / REGION_SIZE;
    auto const yi = static_cast<uint32_t>(offset->y - user_state.range.offset.y) / REGION_SIZE;
    auto const zi = static_cast<uint32_t>(offset->z - user_state.range.offset.z) / REGION_SIZE;
    auto const px = static_cast<uint32_t>(offset->x - user_state.range.offset.x) - xi * REGION_SIZE;
    auto const py = static_cast<uint32_t>(offset->y - user_state.range.offset.y) - yi * REGION_SIZE;
    auto const pz = static_cast<uint32_t>(offset->z - user_state.range.offset.z) - zi * REGION_SIZE;
    auto r_nx = user_state.r_nx;
    auto r_ny = user_state.r_ny;
    auto &channel_header = user_state.region_headers[xi + yi * r_nx + zi * r_nx * r_ny].channels[user_state.channel_indices[channel_id]];
    if (channel_header.variant_n <= 1) {
        return channel_header.blob_offset;
    } else {
        uint8_t *buffer_ptr = user_state.buffer.data() + channel_header.blob_offset;
        if (channel_header.variant_n > MAX_REGION_COMPRESSED_VARIANT_N) {
            auto const index = px + py * REGION_SIZE + pz * REGION_SIZE * REGION_SIZE;
            // return 0;
            return *reinterpret_cast<uint32_t *>(buffer_ptr + index * sizeof(uint32_t));
        } else {
            auto *palette_begin = reinterpret_cast<uint32_t *>(buffer_ptr);
            auto const bits_per_variant = ceil_log2(channel_header.variant_n);
            buffer_ptr += channel_header.variant_n * sizeof(uint32_t);
            auto const index = px + py * REGION_SIZE + pz * REGION_SIZE * REGION_SIZE;
            auto const bit_index = static_cast<uint32_t>(index) * bits_per_variant;
            auto const byte_index = bit_index / 8;
            auto const bit_offset = static_cast<uint32_t>(bit_index - byte_index * 8);
            auto const mask = get_mask(bits_per_variant);
#if 0
            // Note: This is technically UB, since I think it breaks the strict aliasing rules of C++.
            auto input = *reinterpret_cast<uint32_t *>(buffer_ptr + byte_index);
            // The "correct" solution is below.
#else
            auto input = std::bit_cast<uint32_t>(*reinterpret_cast<std::array<uint8_t, 4> *>(buffer_ptr + byte_index));
#endif
            auto const palette_id = (input >> bit_offset) & mask;
            // return palette_id;
            return palette_begin[palette_id];
        }
    }
}
