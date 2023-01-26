#include <gvox/adapter.h>
#include <gvox/adapters/parse/gvox_palette.h>

#include <cstring>
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

extern "C" void gvox_parse_adapter_gvox_palette_begin(GVoxAdapterContext *ctx, [[maybe_unused]] void *config) {
    auto &parse_state = *reinterpret_cast<ParseState *>(gvox_parse_adapter_malloc(ctx, sizeof(ParseState)));
    gvox_parse_adapter_set_user_pointer(ctx, &parse_state);
    memset(&parse_state, 0, sizeof(ParseState));

    uint32_t node_n = 0;
    gvox_input_read(ctx, parse_state.offset, sizeof(uint32_t), &node_n);
    parse_state.offset += sizeof(uint32_t);

    if (node_n != 1) {
        gvox_parse_adapter_push_error(ctx, GVOX_ERROR_UNKNOWN, "gvox_palette does not support more than 1 node");
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

extern "C" auto gvox_parse_adapter_gvox_palette_query_region_flags([[maybe_unused]] GVoxAdapterContext *ctx, [[maybe_unused]] GVoxOffset3D const *offset) -> uint32_t {
    return 0;
}

extern "C" void gvox_parse_adapter_gvox_palette_load_region(GVoxAdapterContext *ctx, GVoxOffset3D const *offset) {
    auto &parse_state = *reinterpret_cast<ParseState *>(gvox_parse_adapter_get_user_pointer(ctx));
    auto base_offset = parse_state.offset;
    uint32_t voxel_data = 0;
    // auto read_offset = base_offset + sizeof(uint32_t) * (offset->y * parse_state.size_x + offset->z * parse_state.size_x * parse_state.size_y);
    // gvox_input_read(ctx, read_offset, sizeof(voxel_data), &voxel_data);

    GVoxRegion region = {
        .range = {
            .offset = {
                (offset->x / 8) * 8,
                (offset->y / 8) * 8,
                (offset->z / 8) * 8,
            },
            .extent = {8, 8, 8},
        },
        .flags = GVOX_REGION_FLAG_UNIFORM,
        .data = reinterpret_cast<void *>(static_cast<size_t>(voxel_data)),
    };
    gvox_make_region_available(ctx, &region);
}

extern "C" auto gvox_parse_adapter_gvox_palette_sample_data([[maybe_unused]] GVoxAdapterContext *ctx, GVoxRegion const *region, [[maybe_unused]] GVoxOffset3D const *offset) -> uint32_t {
    return static_cast<uint32_t>(reinterpret_cast<size_t>(region->data));
}
