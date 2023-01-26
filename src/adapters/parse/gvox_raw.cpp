#include <gvox/adapter.h>
#include <gvox/adapters/parse/gvox_raw.h>

#include <cstring>

using ParseState = struct {
    uint64_t size_x;
    uint64_t size_y;
    uint64_t size_z;

    uint64_t offset;
};

extern "C" void gvox_parse_adapter_gvox_raw_begin(GVoxAdapterContext *ctx, [[maybe_unused]] void *config) {
    auto &parse_state = *reinterpret_cast<ParseState *>(gvox_parse_adapter_malloc(ctx, sizeof(ParseState)));
    gvox_parse_adapter_set_user_pointer(ctx, &parse_state);
    memset(&parse_state, 0, sizeof(ParseState));

    uint64_t node_n = 0;
    gvox_input_read(ctx, parse_state.offset, sizeof(uint64_t), &node_n);
    parse_state.offset += sizeof(uint64_t);

    if (node_n != 1) {
        gvox_parse_adapter_push_error(ctx, GVOX_ERROR_UNKNOWN, "gvox_raw does not support more than 1 node");
        return;
    }

    gvox_input_read(ctx, parse_state.offset, sizeof(uint64_t), &parse_state.size_x);
    parse_state.offset += sizeof(uint64_t);
    gvox_input_read(ctx, parse_state.offset, sizeof(uint64_t), &parse_state.size_y);
    parse_state.offset += sizeof(uint64_t);
    gvox_input_read(ctx, parse_state.offset, sizeof(uint64_t), &parse_state.size_z);
    parse_state.offset += sizeof(uint64_t);
}

extern "C" void gvox_parse_adapter_gvox_raw_end([[maybe_unused]] GVoxAdapterContext *ctx) {
}

extern "C" auto gvox_parse_adapter_gvox_raw_query_region_flags([[maybe_unused]] GVoxAdapterContext *ctx, [[maybe_unused]] GVoxOffset3D const *offset) -> uint32_t {
    return 0;
}

extern "C" void gvox_parse_adapter_gvox_raw_load_region(GVoxAdapterContext *ctx, GVoxOffset3D const *offset) {
    auto &parse_state = *reinterpret_cast<ParseState *>(gvox_parse_adapter_get_user_pointer(ctx));
    auto base_offset = parse_state.offset + static_cast<size_t>(offset->x) * sizeof(uint32_t);
    uint32_t voxel_data = 0;
    auto read_offset = base_offset + sizeof(uint32_t) * (offset->y * parse_state.size_x + offset->z * parse_state.size_x * parse_state.size_y);
    gvox_input_read(ctx, read_offset, sizeof(voxel_data), &voxel_data);
    GVoxRegion const region = {
        .range = {
            .offset = *offset,
            .extent = {1, 1, 1},
        },
        .flags = GVOX_REGION_FLAG_UNIFORM,
        .data = reinterpret_cast<void *>(static_cast<size_t>(voxel_data)),
    };
    gvox_make_region_available(ctx, &region);
}

extern "C" auto gvox_parse_adapter_gvox_raw_sample_data([[maybe_unused]] GVoxAdapterContext *ctx, GVoxRegion const *region, [[maybe_unused]] GVoxOffset3D const *offset) -> uint32_t {
    return static_cast<uint32_t>(reinterpret_cast<size_t>(region->data));
}
