#include <gvox/gvox.h>
#include <gvox/adapters/parse/test.h>

#include <limits>

extern "C" void gvox_parse_adapter_test_begin([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] void *config) {
}

extern "C" void gvox_parse_adapter_test_end([[maybe_unused]] GvoxAdapterContext *ctx) {
}

extern "C" auto gvox_parse_adapter_test_query_region_flags([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxRegionRange const *range, [[maybe_unused]] uint32_t channel_id) -> uint32_t {
    return GVOX_REGION_FLAG_UNIFORM;
}

extern "C" auto gvox_parse_adapter_test_load_region(GvoxAdapterContext *ctx, [[maybe_unused]] GvoxOffset3D const *offset, [[maybe_unused]] uint32_t channel_id) -> GvoxRegion {
    uint32_t voxel_data = 0;
    gvox_input_read(ctx, 0, sizeof(voxel_data), &voxel_data);
    GvoxRegion const region = {
        .range = {
            .offset = {std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()},
            .extent = {std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()},
        },
        .channels = 0xffffffff,
        .flags = GVOX_REGION_FLAG_UNIFORM,
        .data = reinterpret_cast<void *>(static_cast<size_t>(voxel_data)),
    };
    return region;
}

extern "C" void gvox_parse_adapter_test_unload_region([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxRegion *region) {
}

extern "C" auto gvox_parse_adapter_test_sample_region([[maybe_unused]] GvoxAdapterContext *ctx, GvoxRegion const *region, [[maybe_unused]] GvoxOffset3D const *offset, [[maybe_unused]] uint32_t channel_id) -> uint32_t {
    return static_cast<uint32_t>(reinterpret_cast<size_t>(region->data));
}
