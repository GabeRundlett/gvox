#include <gvox/adapter.h>
#include <gvox/adapters/parse/test.h>

#include <limits>

extern "C" void gvox_parse_adapter_test_begin([[maybe_unused]] GVoxAdapterContext *ctx, [[maybe_unused]] void *config) {
}

extern "C" void gvox_parse_adapter_test_end([[maybe_unused]] GVoxAdapterContext *ctx) {
}

extern "C" auto gvox_parse_adapter_test_query_region_flags([[maybe_unused]] GVoxAdapterContext *ctx, [[maybe_unused]] GVoxRegionRange const *range, [[maybe_unused]] uint32_t channel_index) -> uint32_t {
    return GVOX_REGION_FLAG_UNIFORM;
}

extern "C" void gvox_parse_adapter_test_load_region(GVoxAdapterContext *ctx, [[maybe_unused]] GVoxOffset3D const *offset, [[maybe_unused]] uint32_t channel_index) {
    uint32_t voxel_data = 0;
    gvox_input_read(ctx, 0, sizeof(voxel_data), &voxel_data);
    GVoxRegion const region = {
        .range = {
            .offset = {std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min()},
            .extent = {std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()},
        },
        .channels = 0xffffffff,
        .flags = GVOX_REGION_FLAG_UNIFORM,
        .data = reinterpret_cast<void *>(static_cast<size_t>(voxel_data)),
    };
    gvox_make_region_available(ctx, &region);
}

extern "C" auto gvox_parse_adapter_test_sample_data([[maybe_unused]] GVoxAdapterContext *ctx, GVoxRegion const *region, [[maybe_unused]] GVoxOffset3D const *offset, [[maybe_unused]] uint32_t channel_index) -> uint32_t {
    return static_cast<uint32_t>(reinterpret_cast<size_t>(region->data));
}
