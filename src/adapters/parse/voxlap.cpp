#include <gvox/gvox.h>
#include <gvox/adapters/parse/voxlap.h>

#include <cstdlib>
#include <cstdint>

#include <bit>
#include <vector>
#include <array>
#include <new>
#include <limits>

struct VoxlapParseUserState {
    GvoxVoxlapParseAdapterConfig config{};
    size_t offset{};

    std::vector<uint32_t> colors{};
    std::vector<bool> is_solid{};
};

// Base
extern "C" void gvox_parse_adapter_voxlap_create(GvoxAdapterContext *ctx, void const *config) {
    auto *user_state_ptr = malloc(sizeof(VoxlapParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) VoxlapParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
    if (config != nullptr) {
        user_state.config = *static_cast<GvoxVoxlapParseAdapterConfig const *>(config);
        if (user_state.config.size_x == std::numeric_limits<uint32_t>::max()) {
            user_state.config.size_x = 1024;
        }
        if (user_state.config.size_y == std::numeric_limits<uint32_t>::max()) {
            user_state.config.size_y = 1024;
        }
        if (user_state.config.make_solid == std::numeric_limits<uint8_t>::max()) {
            user_state.config.make_solid = 1;
        }
        if (user_state.config.is_ace_of_spades == std::numeric_limits<uint8_t>::max()) {
            user_state.config.is_ace_of_spades = 0;
        }
    } else {
        user_state.config = GvoxVoxlapParseAdapterConfig{
            .size_x = 1024,
            .size_y = 1024,
            .size_z = 256,
            .make_solid = 1,
        };
    }
}

extern "C" void gvox_parse_adapter_voxlap_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<VoxlapParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~VoxlapParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_voxlap_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) {
    auto &user_state = *static_cast<VoxlapParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto temp_i = uint32_t{};
    if (user_state.config.is_ace_of_spades == 0) {
        gvox_input_read(blit_ctx, user_state.offset, sizeof(temp_i), &temp_i);
        user_state.offset += sizeof(temp_i);
        if (temp_i != 0x09072000) {
            gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "A voxlap data stream must begin with a valid magic number");
            return;
        }
        gvox_input_read(blit_ctx, user_state.offset, sizeof(temp_i), &temp_i);
        user_state.offset += sizeof(temp_i);
        if (temp_i != 1024) {
            gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "Second u32 not 1024");
            return;
        }
        // gvox_input_read(blit_ctx, user_state.offset, sizeof(double) * 3, &ipos); // camera position
        user_state.offset += sizeof(double) * 3;
        // gvox_input_read(blit_ctx, user_state.offset, sizeof(double) * 3, &istr); // unit right vector
        user_state.offset += sizeof(double) * 3;
        // gvox_input_read(blit_ctx, user_state.offset, sizeof(double) * 3, &ihei); // unit down vector
        user_state.offset += sizeof(double) * 3;
        // gvox_input_read(blit_ctx, user_state.offset, sizeof(double) * 3, &ifor); // unit forward vector
        user_state.offset += sizeof(double) * 3;
    }
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t z = 0;
    uint32_t const voxel_n = user_state.config.size_x * user_state.config.size_y * user_state.config.size_z;
    user_state.colors.resize(voxel_n);
    user_state.is_solid.resize(voxel_n);
    auto setgeom = [&user_state](uint32_t x, uint32_t y, uint32_t z, bool solid) {
        if (z >= user_state.config.size_z) {
            return;
        }
        auto index = x + (user_state.config.size_y - 1 - y) * user_state.config.size_x + (user_state.config.size_z - 1 - z) * user_state.config.size_x * user_state.config.size_y;
        user_state.is_solid[index] = solid;
    };
    auto setcol = [&user_state](uint32_t x, uint32_t y, uint32_t z, uint32_t color) {
        if (z >= user_state.config.size_z) {
            return;
        }
        auto index = x + (user_state.config.size_y - 1 - y) * user_state.config.size_x + (user_state.config.size_z - 1 - z) * user_state.config.size_x * user_state.config.size_y;
        uint32_t c = 0;
        c |= ((color >> 0x10) & 0xff) << 0x00;
        c |= ((color >> 0x08) & 0xff) << 0x08;
        c |= ((color >> 0x00) & 0xff) << 0x10;
        user_state.colors[index] = c;
    };
    for (y = 0; y < user_state.config.size_y; ++y) {
        for (x = 0; x < user_state.config.size_x; ++x) {
            for (z = 0; z < user_state.config.size_z; ++z) {
                setgeom(x, y, z, user_state.config.make_solid != 0u);
            }
            z = 0;
            auto v = std::array<uint8_t, 4>{};
            for (;;) {
                gvox_input_read(blit_ctx, user_state.offset, sizeof(v), &v);
                uint32_t const number_4byte_chunks = v[0];
                uint32_t const top_color_start = v[1];
                uint32_t const top_color_end = v[2];
                uint32_t bottom_color_start = 0;
                uint32_t bottom_color_end = 0;
                uint32_t len_top = 0;
                uint32_t len_bottom = 0;
                if (user_state.config.make_solid != 0u) {
                    for (auto i = z; i < top_color_start; ++i) {
                        setgeom(x, y, i, false);
                    }
                }
                auto color_offset = user_state.offset + 4;
                for (z = top_color_start; z <= top_color_end; ++z) {
                    if (user_state.config.make_solid == 0u) {
                        setgeom(x, y, z, true);
                    }
                    auto col = uint32_t{};
                    gvox_input_read(blit_ctx, color_offset, sizeof(col), &col);
                    color_offset += 4;
                    setcol(x, y, z, col);
                }
                len_bottom = top_color_end - top_color_start + 1;
                if (number_4byte_chunks == 0) {
                    // infer ACTUAL number of 4-byte chunks from the length of the color data
                    user_state.offset += 4 * static_cast<size_t>(len_bottom + 1);
                    break;
                }
                len_top = (number_4byte_chunks - 1) - len_bottom;
                user_state.offset += static_cast<size_t>(v[0]) * 4;
                gvox_input_read(blit_ctx, user_state.offset, sizeof(v), &v);
                bottom_color_end = v[3]; // aka air start
                bottom_color_start = bottom_color_end - len_top;
                for (z = bottom_color_start; z < bottom_color_end; ++z) {
                    if (user_state.config.make_solid == 0u) {
                        setgeom(x, y, z, true);
                    }
                    auto col = uint32_t{};
                    gvox_input_read(blit_ctx, color_offset, sizeof(col), &col);
                    color_offset += 4;
                    setcol(x, y, z, col);
                }
            }
        }
    }
}

extern "C" void gvox_parse_adapter_voxlap_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

// General
extern "C" auto gvox_parse_adapter_voxlap_query_details() -> GvoxParseAdapterDetails {
    return {
        .preferred_blit_mode = GVOX_BLIT_MODE_DONT_CARE,
    };
}

extern "C" auto gvox_parse_adapter_voxlap_query_parsable_range(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) -> GvoxRegionRange {
    auto &user_state = *static_cast<VoxlapParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    return {{0, 0, 0}, {user_state.config.size_x, user_state.config.size_y, user_state.config.size_z}};
}

extern "C" auto gvox_parse_adapter_voxlap_sample_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxSample {
    auto &user_state = *static_cast<VoxlapParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if (offset->x < 0 || offset->y < 0 || offset->z < 0 ||
        static_cast<uint32_t>(offset->x) >= user_state.config.size_x ||
        static_cast<uint32_t>(offset->y) >= user_state.config.size_y ||
        static_cast<uint32_t>(offset->z) >= user_state.config.size_z) {
        return {0u, 0u};
    }
    auto voxel_index =
        static_cast<uint32_t>(offset->x) +
        static_cast<uint32_t>(offset->y) * user_state.config.size_x +
        static_cast<uint32_t>(offset->z) * user_state.config.size_x * user_state.config.size_y;
    switch (channel_id) {
    case GVOX_CHANNEL_ID_COLOR: return {user_state.colors[voxel_index], static_cast<uint8_t>(user_state.is_solid[voxel_index])};
    case GVOX_CHANNEL_ID_MATERIAL_ID: return {static_cast<uint32_t>(user_state.is_solid[voxel_index]), static_cast<uint8_t>(user_state.is_solid[voxel_index])};
    default: break;
    }
    return {0u, 0u};
}

// Serialize Driven
extern "C" auto gvox_parse_adapter_voxlap_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_voxlap_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
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

extern "C" void gvox_parse_adapter_voxlap_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

// Parse Driven
extern "C" void gvox_parse_adapter_voxlap_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
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
