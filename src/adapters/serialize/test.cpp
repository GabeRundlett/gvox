#include <gvox/gvox.h>
#include <gvox/adapters/serialize/test.h>

extern "C" void gvox_serialize_adapter_test_begin([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] void *config) {
}

extern "C" void gvox_serialize_adapter_test_end([[maybe_unused]] GvoxAdapterContext *ctx) {
}

extern "C" void gvox_serialize_adapter_test_serialize_region(GvoxAdapterContext *ctx, GvoxRegionRange const *range, [[maybe_unused]] uint32_t channels) {
    auto flags = gvox_query_region_flags(ctx, range, 0u);
    if ((flags & GVOX_REGION_FLAG_UNIFORM) == 0u) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_SERIALIZE_ADAPTER_UNREPRESENTABLE_DATA, "test format must be provided with a uniform range ");
    }
    auto region = gvox_load_region(ctx, &range->offset, GVOX_CHANNEL_ID_COLOR);
    auto data = gvox_sample_region(ctx, &region, &range->offset, GVOX_CHANNEL_ID_COLOR);
    gvox_unload_region(ctx, &region);
    gvox_output_write(ctx, 0, sizeof(data), &data);
}
