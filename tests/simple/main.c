#include <gvox/gvox.h>

#include <gvox/adapters/input/file.h>

int main(void) {
    GVoxContext *gvox_ctx = gvox_create_context();

    GVoxInputAdapter *i_adapter = gvox_get_input_adapter(gvox_ctx, "file");
    GVoxOutputAdapter *o_adapter = gvox_get_output_adapter(gvox_ctx, "stdout");
    GVoxParseAdapter *parse_adapter = gvox_get_parse_adapter(gvox_ctx, "gvox_raw");
    GVoxSerializeAdapter *serialize_adapter = gvox_get_serialize_adapter(gvox_ctx, "colored_text");

    GVoxFileInputAdapterConfig i_config = {.filepath = "test_raw.gvox"};

    GVoxAdapterContext *adapter_ctx = gvox_create_adapter_context(
        i_adapter, &i_config,
        o_adapter, NULL,
        parse_adapter, NULL,
        serialize_adapter, NULL);

    GVoxRegionRange region_range = {
        .offset = {0, 0, 0},
        .extent = {8, 8, 8},
    };
    gvox_translate_region(adapter_ctx, &region_range);

    gvox_destroy_adapter_context(adapter_ctx);
    gvox_destroy_context(gvox_ctx);
}
