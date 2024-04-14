#include <gvox/gvox.h>
#include <gvox/adapters/parse/kvx.h>

#include <cstdlib>
#include <cstdint>

#include <bit>
#include <vector>
#include <array>
#include <new>
#include <limits>

struct KvxParseUserState {
    GvoxKvxParseAdapterConfig config{};

    size_t voxdata_offset;
    uint32_t numbytes;
    uint32_t xsiz, ysiz, zsiz, xpivot, ypivot, zpivot;
    std::vector<uint32_t> xoffset{};
    std::vector<uint16_t> xyoffset{};
    std::vector<uint8_t> voxdata{};
    std::array<std::array<uint8_t, 3>, 256> palette;
};

// Base
extern "C" void gvox_parse_adapter_kvx_create(GvoxAdapterContext *ctx, void const *config) {
    auto *user_state_ptr = malloc(sizeof(KvxParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) KvxParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
    if (config != nullptr) {
        user_state.config = *static_cast<GvoxKvxParseAdapterConfig const *>(config);
        if (user_state.config.mipmaplevels == 0 || user_state.config.mipmaplevels == std::numeric_limits<uint8_t>::max()) {
            user_state.config.mipmaplevels = 5;
        }
    } else {
        user_state.config = GvoxKvxParseAdapterConfig{
            .mipmaplevels = 5,
        };
    }
}

extern "C" void gvox_parse_adapter_kvx_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<KvxParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~KvxParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_kvx_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) {
    auto &user_state = *static_cast<KvxParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    size_t offset = 0;
    auto read = [&](void *dst, size_t size) {
        gvox_input_read(blit_ctx, offset, size, dst);
        offset += size;
    };

    for (int32_t i = 0; i < user_state.config.mipmaplevels; i++) {
        uint32_t numbytes;
        read(&numbytes, 4);
        if (i == 0) {
            user_state.numbytes = numbytes;
            read(&user_state.xsiz, 4);
            read(&user_state.ysiz, 4);
            read(&user_state.zsiz, 4);
            read(&user_state.xpivot, 4);
            read(&user_state.ypivot, 4);
            read(&user_state.zpivot, 4);
            user_state.xoffset.resize(user_state.xsiz + 1);
            read(user_state.xoffset.data(), (user_state.xsiz + 1) * 4);
            user_state.xyoffset.resize(user_state.xsiz * (user_state.ysiz + 1));
            read(user_state.xyoffset.data(), (user_state.xsiz * (user_state.ysiz + 1)) * 2);
            user_state.voxdata_offset = (user_state.xsiz + 1) * 4 + user_state.xsiz * (user_state.ysiz + 1) * 2;
            user_state.voxdata.resize(user_state.numbytes - 24 - user_state.voxdata_offset);
            read(user_state.voxdata.data(), user_state.voxdata.size());
        } else {
            offset += numbytes;
        }
    }
    read(user_state.palette.data(), 768);
}

extern "C" void gvox_parse_adapter_kvx_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

// General
extern "C" auto gvox_parse_adapter_kvx_query_details() -> GvoxParseAdapterDetails {
    return {
        .preferred_blit_mode = GVOX_BLIT_MODE_DONT_CARE,
    };
}

extern "C" auto gvox_parse_adapter_kvx_query_parsable_range(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) -> GvoxRegionRange {
    auto &user_state = *static_cast<KvxParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    return {{0, 0, 0}, {user_state.xsiz, user_state.ysiz, user_state.zsiz}};
}

extern "C" auto gvox_parse_adapter_kvx_sample_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxSample {
    auto &user_state = *static_cast<KvxParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if (offset->x < 0 || offset->y < 0 || offset->z < 0 ||
        static_cast<uint32_t>(offset->x) >= user_state.xsiz ||
        static_cast<uint32_t>(offset->y) >= user_state.ysiz ||
        static_cast<uint32_t>(offset->z) >= user_state.zsiz) {
        return {0u, 0u};
    }

    auto x = offset->x;
    auto y = user_state.ysiz - offset->y - 1;
    auto z = user_state.zsiz - offset->z - 1;

    auto start_index = user_state.xoffset[x] - user_state.voxdata_offset + user_state.xyoffset[x * (user_state.ysiz + 1) + y];
    auto end_index = user_state.xoffset[x] - user_state.voxdata_offset + user_state.xyoffset[x * (user_state.ysiz + 1) + (y + 1)];

    if (start_index >= user_state.voxdata.size() || end_index > user_state.voxdata.size()) {
        return {0u, 0u};
    }
    auto *startptr = user_state.voxdata.data() + start_index;
    auto *endptr = user_state.voxdata.data() + end_index;

    bool is_solid = false;
    uint32_t color = 0x0;

    while (startptr < endptr) {
        auto slabztop = startptr[0];
        auto slabzleng = startptr[1];
        auto slabbackfacecullinfo = startptr[2];
        startptr += 3;

        if (z < slabztop) {
            // missed voxel
            if (is_solid) {
                auto palette_index = startptr[0];
                auto palette_entry = user_state.palette[palette_index];
                color = uint32_t(palette_entry[0] * 4) | uint32_t(palette_entry[1] * 4 << 8) | uint32_t(palette_entry[2] * 4 << 16) | (0x1u << 24);
            }
            break;
        }

        if (slabbackfacecullinfo & 0x20) {
            is_solid = false;
        } else {
            is_solid = true;
        }

        if (z < slabztop + slabzleng) {
            // hit voxel
            auto palette_index = startptr[z - slabztop];
            auto palette_entry = user_state.palette[palette_index];
            color = uint32_t(palette_entry[0] * 4) | uint32_t(palette_entry[1] * 4 << 8) | uint32_t(palette_entry[2] * 4 << 16) | (0x1u << 24);
            is_solid = true;
            break;
        }

        startptr += slabzleng;
    }

    switch (channel_id) {
    case GVOX_CHANNEL_ID_COLOR: return {color, static_cast<uint8_t>(is_solid)};
    case GVOX_CHANNEL_ID_MATERIAL_ID: return {static_cast<uint32_t>(is_solid), static_cast<uint8_t>(is_solid)};
    default: break;
    }
    return {0u, 0u};
}

// Serialize Driven
extern "C" auto gvox_parse_adapter_kvx_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_kvx_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    uint32_t const available_channels = GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_MATERIAL_ID;
    if ((channel_flags & ~available_channels) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = *range,
        .channels = channel_flags & available_channels,
        .flags = 0u,
        .data = nullptr,
    };
    return region;
}

extern "C" void gvox_parse_adapter_kvx_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

// Parse Driven
extern "C" void gvox_parse_adapter_kvx_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    uint32_t const available_channels = GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_MATERIAL_ID;
    if ((channel_flags & ~available_channels) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = *range,
        .channels = channel_flags & available_channels,
        .flags = 0u,
        .data = nullptr,
    };
    gvox_emit_region(blit_ctx, &region);
}
