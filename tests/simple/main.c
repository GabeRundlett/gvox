#include <gvox/gvox.h>

#include <gvox/adapters/input/file.h>
#include <gvox/adapters/output/file.h>
#include <gvox/adapters/output/stdout.h>
#include <gvox/adapters/parse/procedural.h>
#include <gvox/adapters/serialize/gvox_raw.h>
#include <gvox/adapters/serialize/colored_text.h>

#include <stdio.h>

void test_raw_file_io(void) {
    GvoxContext *gvox_ctx = gvox_create_context();

    // Create gvox_raw file
    {
        GvoxOutputAdapter *o_adapter = gvox_get_output_adapter(gvox_ctx, "file");
        GvoxParseAdapter *parse_adapter = gvox_get_parse_adapter(gvox_ctx, "procedural");
        GvoxSerializeAdapter *serialize_adapter = gvox_get_serialize_adapter(gvox_ctx, "gvox_raw");
        GvoxFileOutputAdapterConfig o_config = {
            .filepath = "tests/simple/raw.gvox",
        };
        GvoxAdapterContext *adapter_ctx = gvox_create_adapter_context(
            gvox_ctx,
            NULL, NULL, // Note: No need for an input adapter! the 'procedural' adapter won't be using it
            o_adapter, &o_config,
            parse_adapter, NULL,
            serialize_adapter, NULL);
        GvoxRegionRange region_range = {
            .offset = {0, 0, 0},
            .extent = {16, 16, 16},
        };
        gvox_translate_region(adapter_ctx, &region_range, GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);
        gvox_destroy_adapter_context(adapter_ctx);
    }

    // Load gvox_raw file
    {
        GvoxInputAdapter *i_adapter = gvox_get_input_adapter(gvox_ctx, "file");
        GvoxOutputAdapter *o_adapter = gvox_get_output_adapter(gvox_ctx, "stdout");
        GvoxParseAdapter *parse_adapter = gvox_get_parse_adapter(gvox_ctx, "gvox_raw");
        GvoxSerializeAdapter *serialize_adapter = gvox_get_serialize_adapter(gvox_ctx, "colored_text");
        GvoxFileInputAdapterConfig i_config = {
            .filepath = "tests/simple/raw.gvox",
            .byte_offset = 0,
        };
        uint32_t channels[3] = {GVOX_CHANNEL_ID_COLOR, GVOX_CHANNEL_ID_NORMAL, GVOX_CHANNEL_ID_MATERIAL_ID};
        for (uint32_t channel_i = 0; channel_i < sizeof(channels) / sizeof(channels[0]); ++channel_i) {
            GvoxColoredTextSerializeAdapterConfig s_config = {
                .downscale_factor = 1,
                .downscale_mode = GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_DOWNSCALE_MODE_LINEAR,
                .channel_id = channels[channel_i],
                .non_color_max_value = 5,
            };
            GvoxAdapterContext *adapter_ctx = gvox_create_adapter_context(
                gvox_ctx,
                i_adapter, &i_config,
                o_adapter, NULL,
                parse_adapter, NULL,
                serialize_adapter, &s_config);
            GvoxRegionRange region_range = {
                .offset = {0, 0, 0},
                .extent = {16, 16, 16},
            };
            gvox_translate_region(adapter_ctx, &region_range, 1u << s_config.channel_id);
            gvox_destroy_adapter_context(adapter_ctx);
        }
    }

    gvox_destroy_context(gvox_ctx);
}

void test_palette_file_io(void) {
    GvoxContext *gvox_ctx = gvox_create_context();

    // Create gvox_palette file
    {
        GvoxOutputAdapter *o_adapter = gvox_get_output_adapter(gvox_ctx, "file");
        GvoxParseAdapter *parse_adapter = gvox_get_parse_adapter(gvox_ctx, "procedural");
        GvoxSerializeAdapter *serialize_adapter = gvox_get_serialize_adapter(gvox_ctx, "gvox_palette");
        GvoxFileOutputAdapterConfig o_config = {
            .filepath = "tests/simple/palette.gvox",
        };
        GvoxAdapterContext *adapter_ctx = gvox_create_adapter_context(
            gvox_ctx,
            NULL, NULL, // Note: No need for an input adapter! the 'procedural' adapter won't be using it
            o_adapter, &o_config,
            parse_adapter, NULL,
            serialize_adapter, NULL);
        GvoxRegionRange region_range = {
            .offset = {0, 0, 0},
            .extent = {16, 16, 16},
        };
        gvox_translate_region(adapter_ctx, &region_range, GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);
        gvox_destroy_adapter_context(adapter_ctx);
    }

    printf("\n");

    // Load gvox_palette file
    {
        GvoxInputAdapter *i_adapter = gvox_get_input_adapter(gvox_ctx, "file");
        GvoxOutputAdapter *o_adapter = gvox_get_output_adapter(gvox_ctx, "stdout");
        GvoxParseAdapter *parse_adapter = gvox_get_parse_adapter(gvox_ctx, "gvox_palette");
        GvoxSerializeAdapter *serialize_adapter = gvox_get_serialize_adapter(gvox_ctx, "colored_text");
        GvoxFileInputAdapterConfig i_config = {
            .filepath = "tests/simple/palette.gvox",
            .byte_offset = 0,
        };
        uint32_t channels[3] = {GVOX_CHANNEL_ID_COLOR, GVOX_CHANNEL_ID_NORMAL, GVOX_CHANNEL_ID_MATERIAL_ID};
        for (uint32_t channel_i = 0; channel_i < sizeof(channels) / sizeof(channels[0]); ++channel_i) {
            GvoxColoredTextSerializeAdapterConfig s_config = {
                .downscale_factor = 1,
                .downscale_mode = GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_DOWNSCALE_MODE_LINEAR,
                .channel_id = channels[channel_i],
                .non_color_max_value = 5,
            };
            GvoxAdapterContext *adapter_ctx = gvox_create_adapter_context(
                gvox_ctx,
                i_adapter, &i_config,
                o_adapter, NULL,
                parse_adapter, NULL,
                serialize_adapter, &s_config);
            GvoxRegionRange region_range = {
                .offset = {0, 0, 0},
                .extent = {16, 16, 16},
            };
            gvox_translate_region(adapter_ctx, &region_range, 1u << s_config.channel_id);
            gvox_destroy_adapter_context(adapter_ctx);
        }
    }

    gvox_destroy_context(gvox_ctx);
}

int main(void) {
    test_raw_file_io();
    test_palette_file_io();
}
