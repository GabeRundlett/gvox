#include <gvox/gvox.h>

#include <gvox/adapters/input/file.h>
#include <gvox/adapters/serialize/colored_text.h>

int main(void) {
    GVoxContext *gvox_ctx = gvox_create_context();

    GVoxInputAdapter *i_adapter = gvox_get_input_adapter(gvox_ctx, "file");
    GVoxOutputAdapter *o_adapter = gvox_get_output_adapter(gvox_ctx, "stdout");
    GVoxParseAdapter *parse_adapter = gvox_get_parse_adapter(gvox_ctx, "gvox_palette");
    GVoxSerializeAdapter *serialize_adapter = gvox_get_serialize_adapter(gvox_ctx, "colored_text");

    // GVoxFileInputAdapterConfig i_config = {.filepath = "test_palette.gvox"};
    GVoxFileInputAdapterConfig i_config = {.filepath = "half-life-c2a5w.gvox"};
    GVoxColoredTextSerializeAdapterConfig s_config = {
        .downscale_factor = 512 / 8,
        .downscale_mode = GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_DOWNSCALE_MODE_NEAREST,
        .channel_index = GVOX_CHANNEL_INDEX_COLOR,
        .non_color_max_value = 1,
    };

    GVoxAdapterContext *adapter_ctx = gvox_create_adapter_context(
        gvox_ctx,
        i_adapter, &i_config,
        o_adapter, NULL,
        parse_adapter, NULL,
        serialize_adapter, &s_config);

    GVoxRegionRange region_range = {
        .offset = {0, 0, 0},
        .extent = {512, 512, 512},
    };
    gvox_translate_region(adapter_ctx, &region_range);

    gvox_destroy_adapter_context(adapter_ctx);
    gvox_destroy_context(gvox_ctx);
}
