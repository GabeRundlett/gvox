#include <gvox/gvox.h>

#include <gvox/adapters/input/file.h>
#include <gvox/adapters/output/file.h>
#include <gvox/adapters/output/stdout.h>
#include <adapters/procedural.h>
#include <gvox/adapters/serialize/gvox_raw.h>
#include <gvox/adapters/serialize/colored_text.h>

#include <stdio.h>

void test_raw_file_io(void) {
    GvoxContext *gvox_ctx = gvox_create_context();

    // Create gvox_raw file
    {
        GvoxParseAdapterInfo procedural_adapter_info = {
            .base_info = {
                .name_str = "procedural",
                .create = procedural_create,
                .destroy = procedural_destroy,
                .blit_begin = procedural_blit_begin,
                .blit_end = procedural_blit_end,
            },
            .query_region_flags = procedural_query_region_flags,
            .load_region = procedural_load_region,
            .unload_region = procedural_unload_region,
            .sample_region = procedural_sample_region,
        };
        GvoxFileOutputAdapterConfig o_config = {
            .filepath = "tests/simple/raw.gvox",
        };
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "file"), &o_config);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_register_parse_adapter(gvox_ctx, &procedural_adapter_info), NULL);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "gvox_raw"), NULL);

        GvoxRegionRange region_range = {
            .offset = {-4, -4, -4},
            .extent = {+8, +8, +8},
        };
        gvox_blit_region(
            NULL, o_ctx, p_ctx, s_ctx,
            &region_range, &region_range,
            GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);

        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }

    // Load gvox_raw file
    {
        GvoxFileInputAdapterConfig i_config = {
            .filepath = "tests/simple/raw.gvox",
            .byte_offset = 0,
        };
        GvoxColoredTextSerializeAdapterConfig s_config = {
            .channel_id = GVOX_CHANNEL_ID_COLOR,
            .non_color_max_value = 5,
        };
        GvoxAdapterContext *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "file"), &i_config);
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "stdout"), NULL);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "gvox_raw"), NULL);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "colored_text"), &s_config);

        GvoxRegionRange region_range = {
            .offset = {-4, -4, -4},
            .extent = {+8, +8, +8},
        };
        gvox_blit_region(
            i_ctx, o_ctx, p_ctx, s_ctx,
            &region_range, &region_range,
            GVOX_CHANNEL_BIT_COLOR);

        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }

    gvox_destroy_context(gvox_ctx);
}

void test_palette_file_io(void) {
    GvoxContext *gvox_ctx = gvox_create_context();

    // Create gvox_palette file
    {
        GvoxParseAdapterInfo procedural_adapter_info = {
            .base_info = {
                .name_str = "procedural",
                .create = procedural_create,
                .destroy = procedural_destroy,
                .blit_begin = procedural_blit_begin,
                .blit_end = procedural_blit_end,
            },
            .query_region_flags = procedural_query_region_flags,
            .load_region = procedural_load_region,
            .unload_region = procedural_unload_region,
            .sample_region = procedural_sample_region,
        };
        GvoxFileOutputAdapterConfig o_config = {
            .filepath = "tests/simple/raw.gvox",
        };
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "file"), &o_config);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_register_parse_adapter(gvox_ctx, &procedural_adapter_info), NULL);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "gvox_palette"), NULL);

        GvoxRegionRange region_range = {
            .offset = {-4, -4, -4},
            .extent = {+8, +8, +8},
        };
        gvox_blit_region(
            NULL, o_ctx, p_ctx, s_ctx,
            &region_range, &region_range,
            GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);

        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }

    // Load gvox_palette file
    {
        GvoxFileInputAdapterConfig i_config = {
            .filepath = "tests/simple/raw.gvox",
            .byte_offset = 0,
        };
        GvoxColoredTextSerializeAdapterConfig s_config = {
            .channel_id = GVOX_CHANNEL_ID_COLOR,
            .non_color_max_value = 5,
        };
        GvoxAdapterContext *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "file"), &i_config);
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "stdout"), NULL);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "gvox_palette"), NULL);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "colored_text"), &s_config);

        GvoxRegionRange region_range = {
            .offset = {-4, -4, -4},
            .extent = {+8, +8, +8},
        };
        gvox_blit_region(
            i_ctx, o_ctx, p_ctx, s_ctx,
            &region_range, &region_range,
            GVOX_CHANNEL_BIT_COLOR);

        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }

    gvox_destroy_context(gvox_ctx);
}

void test_magicavoxel(void) {
    GvoxContext *gvox_ctx = gvox_create_context();
    {
        GvoxFileInputAdapterConfig i_config = {
            .filepath = "assets/test.vox",
            .byte_offset = 0,
        };
        GvoxColoredTextSerializeAdapterConfig s_configs[] = {
            {
                .channel_id = GVOX_CHANNEL_ID_COLOR,
            },
            {
                .channel_id = GVOX_CHANNEL_ID_MATERIAL_ID,
                .non_color_max_value = 255,
            },
            {
                .channel_id = GVOX_CHANNEL_ID_ROUGHNESS,
            },
            {
                .channel_id = GVOX_CHANNEL_ID_TRANSPARENCY,
            },
            {
                .channel_id = GVOX_CHANNEL_ID_EMISSIVE_COLOR,
            },
        };
        for (uint32_t s_config_i = 0; s_config_i < sizeof(s_configs) / sizeof(s_configs[0]); ++s_config_i) {
            GvoxAdapterContext *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "file"), &i_config);
            GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "stdout"), NULL);
            GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "magicavoxel"), NULL);
            GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "colored_text"), &s_configs[s_config_i]);
            GvoxRegionRange region_range = {
                .offset = {-4, -4, +0},
                .extent = {+8, +8, +8},
            };
            gvox_blit_region(
                i_ctx, o_ctx, p_ctx, s_ctx,
                &region_range, &region_range,
                GVOX_CHANNEL_BIT_COLOR);
            gvox_destroy_adapter_context(i_ctx);
            gvox_destroy_adapter_context(o_ctx);
            gvox_destroy_adapter_context(p_ctx);
            gvox_destroy_adapter_context(s_ctx);
            printf("\n\n");
        }
    }
    gvox_destroy_context(gvox_ctx);
}

int main(void) {
    test_raw_file_io();
    test_palette_file_io();
    test_magicavoxel();
}
