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

using UserState = struct {
    size_t offset;
    NodeHeader node_header;
    std::vector<RegionHeader> region_headers;
    std::vector<uint8_t> buffer;
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
    auto &user_state = *reinterpret_cast<UserState *>(gvox_adapter_malloc(ctx, sizeof(UserState)));
    gvox_parse_adapter_set_user_pointer(ctx, &user_state);
    memset(&user_state, 0, sizeof(UserState));

    user_state.offset += 16 + 16;

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

    GVoxRegion const region = {
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

    gvox_make_region_available(ctx, &region);
}

extern "C" void gvox_parse_adapter_gvox_palette_end([[maybe_unused]] GVoxAdapterContext *ctx) {
}

extern "C" auto gvox_parse_adapter_gvox_palette_query_region_flags([[maybe_unused]] GVoxAdapterContext *ctx, GVoxRegionRange const *range, [[maybe_unused]] uint32_t channel_index) -> uint32_t {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    auto ax = range->offset.x / 8;
    auto ay = range->offset.y / 8;
    auto az = range->offset.z / 8;
    auto bx = (range->offset.x + range->extent.x) / 8;
    auto by = (range->offset.y + range->extent.y) / 8;
    auto bz = (range->offset.z + range->extent.z) / 8;

    auto r_nx = user_state.node_header.region_count_x;
    auto r_ny = user_state.node_header.region_count_y;
    auto &a_region_header = user_state.region_headers[ax + ay * r_nx + az * r_nx * r_ny];

    uint32_t flags = 0;
    if (a_region_header.variant_n == 1) {
        flags |= GVOX_REGION_FLAG_UNIFORM;
        for (uint32_t zi = az; zi <= bz; ++zi) {
            for (uint32_t yi = ay; yi <= by; ++yi) {
                for (uint32_t xi = ax; xi <= bx; ++xi) {
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

extern "C" void gvox_parse_adapter_gvox_palette_load_region([[maybe_unused]] GVoxAdapterContext *ctx, [[maybe_unused]] GVoxOffset3D const *offset, [[maybe_unused]] uint32_t channel_index) {
}

extern "C" auto gvox_parse_adapter_gvox_palette_sample_data([[maybe_unused]] GVoxAdapterContext *ctx, [[maybe_unused]] GVoxRegion const *region, [[maybe_unused]] GVoxOffset3D const *offset, uint32_t /*channel_index*/) -> uint32_t {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    auto const xi = offset->x / REGION_SIZE;
    auto const yi = offset->y / REGION_SIZE;
    auto const zi = offset->z / REGION_SIZE;
    auto const px = offset->x - xi * REGION_SIZE;
    auto const py = offset->y - yi * REGION_SIZE;
    auto const pz = offset->z - zi * REGION_SIZE;
    auto r_nx = user_state.node_header.region_count_x;
    auto r_ny = user_state.node_header.region_count_y;
    auto &region_header = user_state.region_headers[xi + yi * r_nx + zi * r_nx * r_ny];
    if (region_header.variant_n == 1) {
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
