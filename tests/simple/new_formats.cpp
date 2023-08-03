#include <gvox/gvox.h>

#include <gvox/adapters/input/file.h>
#include <gvox/adapters/input/byte_buffer.h>
#include <gvox/adapters/output/file.h>
#include <gvox/adapters/output/stdout.h>
#include <gvox/adapters/output/byte_buffer.h>
#include <gvox/adapters/parse/voxlap.h>
#include <gvox/adapters/serialize/gvox_raw.h>
#include <gvox/adapters/serialize/colored_text.h>
#include <gvox/adapters/serialize/random_sample.h>
#include <adapters/procedural.h>

#include <cstdio>
#include <cstdlib>
#include <cassert>

void handle_gvox_error(GvoxContext *gvox_ctx) {
    GvoxResult res = gvox_get_result(gvox_ctx);
    int error_count = 0;
    while (res != GVOX_RESULT_SUCCESS) {
        size_t size = 0;
        gvox_get_result_message(gvox_ctx, nullptr, &size);
        char *str = new char[size + 1];
        gvox_get_result_message(gvox_ctx, str, nullptr);
        str[size] = '\0';
        printf("ERROR: %s\n", str);
        gvox_pop_result(gvox_ctx);
        delete[] str;
        res = gvox_get_result(gvox_ctx);
        ++error_count;
    }
    if (error_count != 0) {
        exit(-error_count);
    }
}

auto const procedural_adapter_info = GvoxParseAdapterInfo{
    .base_info = {
        .name_str = "procedural",
        .create = procedural_create,
        .destroy = procedural_destroy,
        .blit_begin = procedural_blit_begin,
        .blit_end = procedural_blit_end,
    },
    .query_details = procedural_query_details,
    .sample_region = procedural_sample_region,
    .query_region_flags = procedural_query_region_flags,
    .load_region = procedural_load_region,
    .unload_region = procedural_unload_region,
    .parse_region = procedural_parse_region,
};

#define PARSE_MAGICAVOXEL 0
#define MAGICAVOXEL_FILE_NAME "mansion"
#define TEST_SERIALIZATION 1
#define TEST_RANDOM_ACCESS_SPEED 1

void test_adapter(char const *const output_filepath, char const *const adapter_name) {
    auto *gvox_ctx = gvox_create_context();
#if PARSE_MAGICAVOXEL
    GvoxRegionRange *region_range_ptr = nullptr;
#else
    auto region_range = GvoxRegionRange{.offset = {-4, -4, -4}, .extent = {8, 8, 8}};
    auto *region_range_ptr = &region_range;
#endif

#if TEST_SERIALIZATION
    // Create file
    {
        auto i_config = GvoxFileInputAdapterConfig{.filepath = "tests/simple/" MAGICAVOXEL_FILE_NAME ".vox", .byte_offset = 0};
        auto o_config = GvoxFileOutputAdapterConfig{.filepath = output_filepath};
        auto *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "file"), &i_config);
#if PARSE_MAGICAVOXEL
        auto *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "magicavoxel"), NULL);
#else
        auto *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_register_parse_adapter(gvox_ctx, &procedural_adapter_info), nullptr);
#endif
        auto *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "file"), &o_config);
        auto *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, adapter_name), nullptr);
        gvox_blit_region(i_ctx, o_ctx, p_ctx, s_ctx, region_range_ptr, GVOX_CHANNEL_BIT_COLOR);
        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }
    handle_gvox_error(gvox_ctx);
#endif

#if !PARSE_MAGICAVOXEL
    // Load file
    {
        auto i_config = GvoxFileInputAdapterConfig{.filepath = output_filepath, .byte_offset = 0};
        auto s_config = GvoxColoredTextSerializeAdapterConfig{.non_color_max_value = 5};
        auto *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "file"), &i_config);
        auto *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "stdout"), nullptr);
        auto *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, adapter_name), nullptr);
        auto *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "colored_text"), &s_config);
        gvox_blit_region(i_ctx, o_ctx, p_ctx, s_ctx, region_range_ptr, GVOX_CHANNEL_BIT_COLOR);
        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(o_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
    }
    handle_gvox_error(gvox_ctx);
#endif

#if TEST_RANDOM_ACCESS_SPEED
    // Load file
    {
        uint8_t *output_bytes = nullptr;
        size_t output_size = 0;
        auto i_config = GvoxFileInputAdapterConfig{.filepath = output_filepath, .byte_offset = 0};
        auto o_config = GvoxByteBufferOutputAdapterConfig{.out_size = &output_size, .out_byte_buffer_ptr = &output_bytes};
        auto s_config = GvoxRandomSampleSerializeAdapterConfig{.sample_count = 10000000};
        auto *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "file"), &i_config);
        auto *o_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_output_adapter(gvox_ctx, "byte_buffer"), &o_config);
        auto *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, adapter_name), nullptr);
        auto *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "random_sample"), &s_config);
        gvox_blit_region_serialize_driven(i_ctx, o_ctx, p_ctx, s_ctx, region_range_ptr, GVOX_CHANNEL_BIT_COLOR);
        gvox_destroy_adapter_context(i_ctx);
        gvox_destroy_adapter_context(p_ctx);
        gvox_destroy_adapter_context(s_ctx);
        auto duration = *reinterpret_cast<float *>(output_bytes);
        printf("%fs\n", duration);
        free(output_bytes);
    }
    handle_gvox_error(gvox_ctx);
#endif

    gvox_destroy_context(gvox_ctx);
}

auto main() -> int {
    printf("raw\n");
    test_adapter("tests/simple/outputs/" MAGICAVOXEL_FILE_NAME "_gvox_raw.gvr", "gvox_raw");
    printf("palette\n");
    test_adapter("tests/simple/outputs/" MAGICAVOXEL_FILE_NAME "_gvox_palette.gvox", "gvox_palette");
    printf("run length\n");
    test_adapter("tests/simple/outputs/" MAGICAVOXEL_FILE_NAME "_run_length_encoding.rle", "gvox_run_length_encoding");
    printf("octree\n");
    test_adapter("tests/simple/outputs/" MAGICAVOXEL_FILE_NAME "_octree.oct", "gvox_octree");
    printf("global palette\n");
    test_adapter("tests/simple/outputs/" MAGICAVOXEL_FILE_NAME "_global_palette.glp", "gvox_global_palette");
    printf("brickmap\n");
    test_adapter("tests/simple/outputs/" MAGICAVOXEL_FILE_NAME "_brickmap.brk", "gvox_brickmap");
}
