#include <gvox/gvox.h>
#include <gvox/adapters/parse/gvox_palette.h>

#include "../shared/gvox_palette.hpp"

#include <cstring>
#include <vector>
#include <new>

struct NodeHeader {
    uint32_t node_full_size;
    uint32_t region_count_x;
    uint32_t region_count_y;
    uint32_t region_count_z;
};
struct RegionHeader {
    uint32_t variant_n;
    uint32_t blob_offset; // if variant_n == 1, this is just the voxel
};

struct GvoxPaletteParseUserState {
    size_t offset{};
    NodeHeader node_header{};
    std::vector<RegionHeader> region_headers = {};
    std::vector<uint8_t> buffer = {};
};

extern "C" void gvox_parse_adapter_gvox_palette_begin(GvoxAdapterContext *ctx, [[maybe_unused]] void *config) {
    auto *user_state_ptr = gvox_adapter_malloc(ctx, sizeof(GvoxPaletteParseUserState));
    auto &user_state = *(new (user_state_ptr) GvoxPaletteParseUserState());
    gvox_parse_adapter_set_user_pointer(ctx, user_state_ptr);

    uint32_t node_n = 0;
    gvox_input_read(ctx, user_state.offset, sizeof(uint32_t), &node_n);
    user_state.offset += sizeof(uint32_t);

    if (node_n != 1) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "gvox_palette does not support more than 1 node");
        return;
    }

    gvox_input_read(ctx, user_state.offset, sizeof(NodeHeader), &user_state.node_header);
    user_state.offset += sizeof(NodeHeader);
    auto const region_n = user_state.node_header.region_count_x * user_state.node_header.region_count_y * user_state.node_header.region_count_z;
    user_state.region_headers.resize(region_n);

    for (auto &region_header : user_state.region_headers) {
        gvox_input_read(ctx, user_state.offset, sizeof(RegionHeader), &region_header);
        user_state.offset += sizeof(RegionHeader);
    }

    user_state.buffer.resize(user_state.node_header.node_full_size);
    gvox_input_read(ctx, user_state.offset, user_state.node_header.node_full_size, user_state.buffer.data());
}

extern "C" void gvox_parse_adapter_gvox_palette_end([[maybe_unused]] GvoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<GvoxPaletteParseUserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    user_state.~GvoxPaletteParseUserState();
}

extern "C" auto gvox_parse_adapter_gvox_palette_query_region_flags([[maybe_unused]] GvoxAdapterContext *ctx, GvoxRegionRange const *range, [[maybe_unused]] uint32_t channel_id) -> uint32_t {
    auto &user_state = *reinterpret_cast<GvoxPaletteParseUserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    auto ax = range->offset.x / 8;
    auto ay = range->offset.y / 8;
    auto az = range->offset.z / 8;
    auto bx = (range->offset.x + static_cast<int32_t>(range->extent.x)) / 8;
    auto by = (range->offset.y + static_cast<int32_t>(range->extent.y)) / 8;
    auto bz = (range->offset.z + static_cast<int32_t>(range->extent.z)) / 8;

    auto r_nx = user_state.node_header.region_count_x;
    auto r_ny = user_state.node_header.region_count_y;
    auto &a_region_header = user_state.region_headers[ax + ay * r_nx + az * r_nx * r_ny];

    uint32_t flags = 0;
    if (a_region_header.variant_n == 1) {
        flags |= GVOX_REGION_FLAG_UNIFORM;
        for (int32_t zi = az; zi <= bz; ++zi) {
            for (int32_t yi = ay; yi <= by; ++yi) {
                for (int32_t xi = ax; xi <= bx; ++xi) {
                    auto &b_region_header = user_state.region_headers[xi + yi * r_nx + zi * r_nx * r_ny];
                    if (b_region_header.variant_n != 1 || b_region_header.blob_offset != a_region_header.blob_offset) {
                        flags = 0;
                        break;
                    }
                }
            }
        }
    }
    return flags;
}

extern "C" auto gvox_parse_adapter_gvox_palette_load_region([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxOffset3D const *offset, [[maybe_unused]] uint32_t channel_id) -> GvoxRegion {
    auto &user_state = *reinterpret_cast<GvoxPaletteParseUserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    GvoxRegion const region = {
        .range = {
            .offset = {0, 0, 0},
            .extent = {
                user_state.node_header.region_count_x * REGION_SIZE,
                user_state.node_header.region_count_y * REGION_SIZE,
                user_state.node_header.region_count_z * REGION_SIZE,
            },
        },
        .channels = GVOX_CHANNEL_BIT_COLOR,
        .flags = 0,
        .data = nullptr,
    };
    return region;
}

extern "C" void gvox_parse_adapter_gvox_palette_unload_region([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxRegion *region) {
}

extern "C" auto gvox_parse_adapter_gvox_palette_sample_region([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxRegion const *region, [[maybe_unused]] GvoxOffset3D const *offset, [[maybe_unused]] uint32_t channel_id) -> uint32_t {
    auto &user_state = *reinterpret_cast<GvoxPaletteParseUserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    auto const xi = offset->x / REGION_SIZE;
    auto const yi = offset->y / REGION_SIZE;
    auto const zi = offset->z / REGION_SIZE;
    auto const px = offset->x - xi * REGION_SIZE;
    auto const py = offset->y - yi * REGION_SIZE;
    auto const pz = offset->z - zi * REGION_SIZE;
    auto r_nx = user_state.node_header.region_count_x;
    auto r_ny = user_state.node_header.region_count_y;
    auto &region_header = user_state.region_headers[xi + yi * r_nx + zi * r_nx * r_ny];
    if (region_header.variant_n <= 1) {
        return region_header.blob_offset;
    } else {
        uint8_t *buffer_ptr = user_state.buffer.data() + region_header.blob_offset;
        if (region_header.variant_n > 367) {
            auto const index = px + py * REGION_SIZE + pz * REGION_SIZE * REGION_SIZE;
            // return 0;
            return *reinterpret_cast<uint32_t *>(buffer_ptr + index * sizeof(uint32_t));
        } else {
            auto *palette_begin = reinterpret_cast<uint32_t *>(buffer_ptr);
            auto const bits_per_variant = ceil_log2(region_header.variant_n);
            buffer_ptr += region_header.variant_n * sizeof(uint32_t);
            auto const index = px + py * REGION_SIZE + pz * REGION_SIZE * REGION_SIZE;
            auto const bit_index = static_cast<uint32_t>(index) * bits_per_variant;
            auto const byte_index = bit_index / 8;
            auto const bit_offset = static_cast<uint32_t>(bit_index - byte_index * 8);
            auto const mask = get_mask(bits_per_variant);
            auto &input = *reinterpret_cast<uint32_t *>(buffer_ptr + byte_index);
            auto const palette_id = (input >> bit_offset) & mask;
            // return palette_id;
            return palette_begin[palette_id];
        }
    }
}
