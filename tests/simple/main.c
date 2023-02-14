#include <gvox/gvox.h>

#include <gvox/adapters/input/file.h>
#include <gvox/adapters/input/byte_buffer.h>
#include <gvox/adapters/output/file.h>
#include <gvox/adapters/output/stdout.h>
#include <gvox/adapters/output/byte_buffer.h>
#include <adapters/procedural.h>
#include <gvox/adapters/parse/voxlap.h>
#include <gvox/adapters/serialize/gvox_raw.h>
#include <gvox/adapters/serialize/colored_text.h>

#include <stdio.h>
#include <stdlib.h>

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
            &region_range,
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
            &region_range,
            GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);

        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }

    gvox_destroy_context(gvox_ctx);
}

void test_palette_buffer_io(void) {
    GvoxContext *gvox_ctx = gvox_create_context();

    uint8_t *data = NULL;
    size_t size = 0;

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
        GvoxByteBufferOutputAdapterConfig o_config = {
            .out_byte_buffer_ptr = &data,
            .out_size = &size,
            .allocate = NULL,
        };
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "byte_buffer"), &o_config);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_register_parse_adapter(gvox_ctx, &procedural_adapter_info), NULL);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "gvox_palette"), NULL);

        GvoxRegionRange region_range = {
            .offset = {-4, -4, -4},
            .extent = {+8, +8, +8},
        };
        gvox_blit_region(
            NULL, o_ctx, p_ctx, s_ctx,
            &region_range,
            GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);

        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }

    // Load gvox_palette file
    {
        GvoxByteBufferInputAdapterConfig i_config = {
            .data = data,
            .size = size,
        };
        GvoxColoredTextSerializeAdapterConfig s_config = {
            .non_color_max_value = 5,
        };
        GvoxAdapterContext *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "byte_buffer"), &i_config);
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "stdout"), NULL);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "gvox_palette"), NULL);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "colored_text"), &s_config);

        GvoxRegionRange region_range = {
            .offset = {-4, -4, -4},
            .extent = {+8, +8, +8},
        };
        gvox_blit_region(
            i_ctx, o_ctx, p_ctx, s_ctx,
            &region_range,
            GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);

        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }

    if (data) {
        free(data);
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
            .filepath = "tests/simple/palette.gvox",
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
            &region_range,
            GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);

        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }

    // Load gvox_palette file
    {
        GvoxFileInputAdapterConfig i_config = {
            .filepath = "tests/simple/palette.gvox",
            .byte_offset = 0,
        };
        GvoxColoredTextSerializeAdapterConfig s_config = {
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
            &region_range,
            GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);

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
        GvoxColoredTextSerializeAdapterConfig s_config = {
            .non_color_max_value = 255,
        };
        GvoxAdapterContext *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "file"), &i_config);
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "stdout"), NULL);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "magicavoxel"), NULL);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "colored_text"), &s_config);
        GvoxRegionRange region_range = {
            .offset = {-4, -4, +0},
            .extent = {+8, +8, +8},
        };
        gvox_blit_region(
            i_ctx, o_ctx, p_ctx, s_ctx,
            &region_range,
            GVOX_CHANNEL_BIT_COLOR |
                GVOX_CHANNEL_BIT_MATERIAL_ID |
                GVOX_CHANNEL_BIT_ROUGHNESS |
                GVOX_CHANNEL_BIT_TRANSPARENCY |
                GVOX_CHANNEL_BIT_EMISSIVITY);
        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }
    {
        GvoxFileInputAdapterConfig i_config = {
            .filepath = "assets/test.vox",
            .byte_offset = 0,
        };
        GvoxFileOutputAdapterConfig o_config = {
            .filepath = "assets/test.gvox",
        };
        GvoxAdapterContext *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "file"), &i_config);
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "file"), &o_config);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "magicavoxel"), NULL);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "gvox_palette"), NULL);
        GvoxRegionRange region_range = {
            .offset = {-4, -4, +0},
            .extent = {+8, +8, +8},
        };
        gvox_blit_region(
            i_ctx, o_ctx, p_ctx, s_ctx,
            &region_range,
            GVOX_CHANNEL_BIT_COLOR |
                GVOX_CHANNEL_BIT_MATERIAL_ID |
                GVOX_CHANNEL_BIT_ROUGHNESS |
                GVOX_CHANNEL_BIT_TRANSPARENCY |
                GVOX_CHANNEL_BIT_EMISSIVITY);
        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }
    gvox_destroy_context(gvox_ctx);
}

void test_voxlap(void) {
    GvoxContext *gvox_ctx = gvox_create_context();
    FILE *f;
    fopen_s(&f, "assets/arab.vxl", "rb");
    fseek(f, 0, SEEK_END);
    size_t size = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(size + 1);
    fread(data, size, 1, f);
    fclose(f);
    {
        GvoxByteBufferInputAdapterConfig i_config = {
            .data = data,
            .size = size,
        };
        GvoxVoxlapParseAdapterConfig p_config = {
            .size_x = 512,
            .size_y = 512,
            .size_z = 64,
            .make_solid = 1,
            .is_ace_of_spades = 1,
        };
        GvoxColoredTextSerializeAdapterConfig s_config = {
            .non_color_max_value = 2,
        };
        GvoxAdapterContext *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "byte_buffer"), &i_config);
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "stdout"), NULL);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "voxlap"), &p_config);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "colored_text"), &s_config);
        GvoxRegionRange region_range = {
            .offset = {+0, +0, +0},
            .extent = {+8, +8, +8},
        };
        gvox_blit_region(
            i_ctx, o_ctx, p_ctx, s_ctx,
            &region_range,
            GVOX_CHANNEL_BIT_COLOR |
                GVOX_CHANNEL_BIT_MATERIAL_ID);
        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }
    {
        GvoxByteBufferInputAdapterConfig i_config = {
            .data = data,
            .size = size,
        };
        GvoxFileOutputAdapterConfig o_config = {
            .filepath = "assets/arab.gvox",
        };
        GvoxVoxlapParseAdapterConfig p_config = {
            .size_x = 512,
            .size_y = 512,
            .size_z = 64,
            .make_solid = 0,
            .is_ace_of_spades = 1,
        };
        GvoxAdapterContext *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "byte_buffer"), &i_config);
        GvoxAdapterContext *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "file"), &o_config);
        GvoxAdapterContext *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "voxlap"), &p_config);
        GvoxAdapterContext *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "gvox_palette"), NULL);
        GvoxRegionRange region_range = {
            .offset = {+0, +0, +0},
            .extent = {+512, +512, +64},
        };
        gvox_blit_region(
            i_ctx, o_ctx, p_ctx, s_ctx,
            &region_range,
            GVOX_CHANNEL_BIT_COLOR |
                GVOX_CHANNEL_BIT_MATERIAL_ID);
        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }
    gvox_destroy_context(gvox_ctx);
}

int main(void) {
    test_raw_file_io();
    test_palette_buffer_io();
    test_palette_file_io();
    test_magicavoxel();
    test_voxlap();
}
