#include <gvox/gvox.h>
// #include <gvox/adapters/parse/gvox_run_length_encoding.h>

#include <cstdlib>
#include <cstdint>

#include <bit>
#include <array>
#include <vector>
#include <new>

struct RunLengthEncodingParseUserState {
    GvoxRegionRange range{};
    uint32_t channel_flags{};
    uint32_t channel_n{};
    size_t offset{};

    std::vector<uint32_t> column_pointers{};
    std::vector<std::vector<uint32_t>> columns{};
};

// Base
extern "C" void gvox_parse_adapter_gvox_run_length_encoding_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(RunLengthEncodingParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) RunLengthEncodingParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_gvox_run_length_encoding_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<RunLengthEncodingParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~RunLengthEncodingParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_gvox_run_length_encoding_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) {
    auto &user_state = *static_cast<RunLengthEncodingParseUserState *>(gvox_adapter_get_user_pointer(ctx));

    uint32_t magic = 0;
    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(uint32_t);

    if (magic != std::bit_cast<uint32_t>(std::array<char, 4>{'r', 'l', 'e', '\0'})) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "parsing a RLE format must begin with a valid magic number");
        return;
    }

    gvox_input_read(blit_ctx, user_state.offset, sizeof(GvoxRegionRange), &user_state.range);
    user_state.offset += sizeof(GvoxRegionRange);

    gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t), &user_state.channel_flags);
    user_state.offset += sizeof(uint32_t);

    user_state.channel_n = static_cast<uint32_t>(std::popcount(user_state.channel_flags));

    user_state.column_pointers.resize(user_state.range.extent.x * user_state.range.extent.y);
    user_state.columns.resize(user_state.range.extent.x * user_state.range.extent.y);
    gvox_input_read(blit_ctx, user_state.offset, user_state.column_pointers.size() * sizeof(user_state.column_pointers[0]), user_state.column_pointers.data());
    // user_state.offset += user_state.column_pointers.size() * sizeof(user_state.column_pointers[0]);
}

extern "C" void gvox_parse_adapter_gvox_run_length_encoding_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

// General
extern "C" auto gvox_parse_adapter_gvox_run_length_encoding_query_details() -> GvoxParseAdapterDetails {
    return {
        .preferred_blit_mode = GVOX_BLIT_MODE_DONT_CARE,
    };
}

extern "C" auto gvox_parse_adapter_gvox_run_length_encoding_query_parsable_range(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) -> GvoxRegionRange {
    auto &user_state = *static_cast<RunLengthEncodingParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    return user_state.range;
}

extern "C" auto gvox_parse_adapter_gvox_run_length_encoding_sample_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxSample {
    auto &user_state = *static_cast<RunLengthEncodingParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto x_pos = static_cast<size_t>(offset->x - user_state.range.offset.x);
    auto y_pos = static_cast<size_t>(offset->y - user_state.range.offset.y);
    auto z_pos = static_cast<size_t>(offset->z - user_state.range.offset.z);
    auto &column = user_state.columns[x_pos + y_pos * user_state.range.extent.x];
    if (column.size() == 0) {
        // load column
        auto column_ptr = user_state.column_pointers[x_pos + y_pos * user_state.range.extent.x];
        column.resize(user_state.channel_n * user_state.range.extent.z);
        uint32_t zi = 0;
        uint32_t read_offset = column_ptr * sizeof(uint32_t) + static_cast<uint32_t>(user_state.offset);
        while (zi < user_state.range.extent.z) {
            for (uint32_t ci = 0; ci < user_state.channel_n; ++ci) {
                auto &voxel_data = column[ci + zi * user_state.channel_n];
                gvox_input_read(blit_ctx, read_offset + ci, sizeof(voxel_data), &voxel_data);
            }
            read_offset += user_state.channel_n * sizeof(uint32_t);
            uint32_t run_length = 0;
            gvox_input_read(blit_ctx, read_offset, sizeof(run_length), &run_length);
            read_offset += 1 * sizeof(uint32_t);
            for (uint32_t ri = 1; ri < run_length; ++ri) {
                for (uint32_t ci = 0; ci < user_state.channel_n; ++ci) {
                    auto &voxel_data = column[ci + zi * user_state.channel_n];
                    column[ci + (zi + ri) * user_state.channel_n] = voxel_data;
                }
            }
            zi += run_length;
        }
    }
    uint32_t voxel_data = 0;
    uint32_t voxel_channel_index = 0;
    for (uint32_t channel_index = 0; channel_index < channel_id; ++channel_index) {
        if (((user_state.channel_flags >> channel_index) & 0x1) != 0) {
            ++voxel_channel_index;
        }
    }
    voxel_data = column[z_pos * user_state.channel_n + voxel_channel_index];
    return {voxel_data, 1u};
}

// Serialize Driven
extern "C" auto gvox_parse_adapter_gvox_run_length_encoding_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_gvox_run_length_encoding_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    auto &user_state = *static_cast<RunLengthEncodingParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if ((channel_flags & ~user_state.channel_flags) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = *range,
        .channels = channel_flags & user_state.channel_flags,
        .flags = 0u,
        .data = nullptr,
    };
    return region;
}

extern "C" void gvox_parse_adapter_gvox_run_length_encoding_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

// Parse Driven
extern "C" void gvox_parse_adapter_gvox_run_length_encoding_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<RunLengthEncodingParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if ((channel_flags & ~user_state.channel_flags) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = *range,
        .channels = channel_flags & user_state.channel_flags,
        .flags = 0u,
        .data = nullptr,
    };
    gvox_emit_region(blit_ctx, &region);
}
