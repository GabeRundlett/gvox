#pragma once

#include <array>

static constexpr auto REGION_SIZE = 8ull;

static constexpr auto ceil_log2(uint32_t x) -> uint32_t {
    constexpr auto const t = std::array<uint32_t, 5>{
        0xFFFF0000u,
        0x0000FF00u,
        0x000000F0u,
        0x0000000Cu,
        0x00000002u};

    uint32_t y = (((x & (x - 1)) == 0) ? 0 : 1);
    int j = 16;

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

static constexpr auto MAX_REGION_ALLOCATION_SIZE = REGION_SIZE * REGION_SIZE * REGION_SIZE * sizeof(uint32_t);
static constexpr auto MAX_REGION_COMPRESSED_VARIANT_N =
    REGION_SIZE == 8 ? 367 : (REGION_SIZE == 16 ? 2559 : 0);

static_assert(calc_block_size(MAX_REGION_COMPRESSED_VARIANT_N) <= MAX_REGION_ALLOCATION_SIZE);
static_assert(calc_block_size(MAX_REGION_COMPRESSED_VARIANT_N + 1) > MAX_REGION_ALLOCATION_SIZE);

struct ChannelHeader {
    uint32_t variant_n;
    uint32_t blob_offset; // if variant_n == 1, this is just the data
};
