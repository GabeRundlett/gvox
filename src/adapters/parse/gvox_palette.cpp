#include <gvox/adapter.h>
#include <gvox/adapters/parse/gvox_palette.h>

#include <cstring>
#include <array>
#include <vector>

using NodeHeader = struct {
    uint32_t node_full_size;
    uint32_t region_count_x;
    uint32_t region_count_y;
    uint32_t region_count_z;
};
using RegionHeader = struct {
    uint32_t variant_n;
    uint32_t blob_offset; // if variant_n == 1, this is just the voxel
};

using ParseState = struct {
    size_t offset;
    NodeHeader node_header;
    std::vector<RegionHeader> region_headers;
};

static constexpr auto REGION_SIZE = 8;

static constexpr auto ceil_log2(uint32_t x) -> uint32_t {
    constexpr auto const t = std::array<uint32_t, 6>{
        0x00000000u,
        0xFFFF0000u,
        0x0000FF00u,
        0x000000F0u,
        0x0000000Cu,
        0x00000002u};

    uint32_t y = (((x & (x - 1)) == 0) ? 0 : 1);
    int j = 32;

    for (uint32_t const i : t) {
        int const k = (((x & i) == 0) ? 0 : j);
        y += static_cast<uint32_t>(k);
        x >>= k;
        j >>= 1;
    }

    return y;
}

static constexpr auto get_mask(uint32_t bits_per_variant) -> uint32_t {
    return (~0u) >> (32 - bits_per_variant);
}

static constexpr auto calc_palette_region_size(size_t bits_per_variant) -> size_t {
    auto palette_region_size = (bits_per_variant * REGION_SIZE * REGION_SIZE * REGION_SIZE + 7) / 8;
    palette_region_size = (palette_region_size + 3) / 4;
    auto size = palette_region_size + 1;
    return size * 4;
}

static constexpr auto calc_block_size(size_t variant_n) -> size_t {
    return calc_palette_region_size(ceil_log2(static_cast<uint32_t>(variant_n))) + sizeof(uint32_t) * variant_n;
}

extern "C" void gvox_parse_adapter_gvox_palette_begin(GVoxAdapterContext *ctx, [[maybe_unused]] void *config) {
    auto &parse_state = *reinterpret_cast<ParseState *>(gvox_adapter_malloc(ctx, sizeof(ParseState)));
    gvox_parse_adapter_set_user_pointer(ctx, &parse_state);
    memset(&parse_state, 0, sizeof(ParseState));

    parse_state.offset += 16 + 16;

    uint32_t node_n = 0;
    gvox_input_read(ctx, parse_state.offset, sizeof(uint32_t), &node_n);
    parse_state.offset += sizeof(uint32_t);

    if (node_n != 1) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "gvox_palette does not support more than 1 node");
        return;
    }

    gvox_input_read(ctx, parse_state.offset, sizeof(NodeHeader), &parse_state.node_header);
    parse_state.offset += sizeof(NodeHeader);
    auto const region_n = parse_state.node_header.region_count_x * parse_state.node_header.region_count_y * parse_state.node_header.region_count_z;
    parse_state.region_headers.resize(region_n);

    for (auto &region_header : parse_state.region_headers) {
        gvox_input_read(ctx, parse_state.offset, sizeof(RegionHeader), &region_header);
        parse_state.offset += sizeof(RegionHeader);
    }
}

extern "C" void gvox_parse_adapter_gvox_palette_end([[maybe_unused]] GVoxAdapterContext *ctx) {
}

extern "C" auto gvox_parse_adapter_gvox_palette_query_region_flags([[maybe_unused]] GVoxAdapterContext *ctx, GVoxRegionRange const *range, uint32_t channel_index) -> uint32_t {
    auto &parse_state = *reinterpret_cast<ParseState *>(gvox_parse_adapter_get_user_pointer(ctx));
    auto ax = range->offset.x / 8;
    auto ay = range->offset.y / 8;
    auto az = range->offset.z / 8;
    auto bx = (range->offset.x + range->extent.x) / 8;
    auto by = (range->offset.y + range->extent.y) / 8;
    auto bz = (range->offset.z + range->extent.z) / 8;

    auto r_nx = parse_state.node_header.region_count_x;
    auto r_ny = parse_state.node_header.region_count_y;
    auto &a_region_header = parse_state.region_headers[ax + ay * r_nx + az * r_nx * r_ny];

    uint32_t flags = 0;
    if (a_region_header.variant_n == 1) {
        flags |= GVOX_REGION_FLAG_UNIFORM;
        for (uint32_t zi = az; zi <= bz; ++zi) {
            for (uint32_t yi = ay; yi <= by; ++yi) {
                for (uint32_t xi = ax; xi <= bx; ++xi) {
                    auto &b_region_header = parse_state.region_headers[xi + yi * r_nx + zi * r_nx * r_ny];
                    if (b_region_header.variant_n > 1 || b_region_header.blob_offset != a_region_header.blob_offset) {
                        flags = 0;
                        break;
                    }
                }
            }
        }
    }
    return flags;
}

extern "C" void gvox_parse_adapter_gvox_palette_load_region(GVoxAdapterContext *ctx, GVoxOffset3D const *offset, uint32_t channel_index) {
    auto &parse_state = *reinterpret_cast<ParseState *>(gvox_parse_adapter_get_user_pointer(ctx));
    auto base_offset = parse_state.offset;

    auto rx = offset->x / 8;
    auto ry = offset->y / 8;
    auto rz = offset->z / 8;
    auto r_nx = parse_state.node_header.region_count_x;
    auto r_ny = parse_state.node_header.region_count_y;

    auto &region_header = parse_state.region_headers[rx + ry * r_nx + rz * r_nx * r_ny];

    GVoxRegion region = {
        .range = {
            .offset = {
                (offset->x / REGION_SIZE) * REGION_SIZE,
                (offset->y / REGION_SIZE) * REGION_SIZE,
                (offset->z / REGION_SIZE) * REGION_SIZE,
            },
            .extent = {REGION_SIZE, REGION_SIZE, REGION_SIZE},
        },
        .channels = 1u << channel_index,
        .flags = 0,
        .data = nullptr,
    };

    if (region_header.variant_n == 1) {
        region.flags |= GVOX_REGION_FLAG_UNIFORM;
        region.data = reinterpret_cast<void *>(static_cast<size_t>(region_header.blob_offset));
    } else {
        auto size = calc_block_size(region_header.variant_n);
        region.data = gvox_adapter_malloc(ctx, sizeof(uint32_t) + size);
        *reinterpret_cast<uint32_t *>(region.data) = region_header.variant_n;
        gvox_input_read(ctx, base_offset + region_header.blob_offset, size, reinterpret_cast<uint32_t *>(region.data) + 1);
    }

    gvox_make_region_available(ctx, &region);
}

extern "C" auto gvox_parse_adapter_gvox_palette_sample_data([[maybe_unused]] GVoxAdapterContext *ctx, GVoxRegion const *region, [[maybe_unused]] GVoxOffset3D const *offset, uint32_t channel_index) -> uint32_t {
    if (region->flags & GVOX_REGION_FLAG_UNIFORM) {
        return static_cast<uint32_t>(reinterpret_cast<size_t>(region->data));
    } else {
        uint8_t *buffer_ptr = reinterpret_cast<uint8_t *>(region->data);
        auto const &variant_n = *reinterpret_cast<uint32_t *>(buffer_ptr);
        buffer_ptr += sizeof(uint32_t);
        if (variant_n > 367) {
            auto const px = offset->x;
            auto const py = offset->y;
            auto const pz = offset->z;
            auto const index = px + py * REGION_SIZE + pz * REGION_SIZE * REGION_SIZE;
            // return 0;
            return *reinterpret_cast<uint32_t *>(buffer_ptr + index * sizeof(uint32_t));
        } else {
            auto *palette_begin = reinterpret_cast<uint32_t *>(buffer_ptr);
            auto const bits_per_variant = ceil_log2(variant_n);
            buffer_ptr += variant_n * sizeof(uint32_t);
            auto const px = offset->x;
            auto const py = offset->y;
            auto const pz = offset->z;
            auto const index = px + py * REGION_SIZE + pz * REGION_SIZE * REGION_SIZE;
            auto const bit_index = static_cast<uint32_t>(index * bits_per_variant);
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
