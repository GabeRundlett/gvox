#include <gvox/gvox.h>
#include <gvox/adapter.h>
#include <gvox/adapters/input/file.h>

#if 1
#include <stdio.h>
#include <stdlib.h>

void check_result(GvoxResult result, char const *failure_message) {
    if (result != GVOX_SUCCESS) {
        printf("[GVOX ERROR CODE %d]: %s", (int)result, failure_message);
        exit(-1);
    }
}

GvoxInputAdapter grass_file_adapter(void) {
    GvoxInputAdapter result;

    GvoxFileInputAdapterConfig config = {0};
    config.filepath = "assets/grass.png";
    GvoxInputAdapterCreateInfo input_ci = {0};
    input_ci.struct_type = GVOX_STRUCT_TYPE_INPUT_ADAPTER_CREATE_INFO;
    input_ci.next = NULL;
    input_ci.cb_args.config = &config;
    check_result(
        gvox_get_standard_input_adapter_description("file", &input_ci.description),
        "Failed to find standard input adapter 'file' description\n");

    check_result(
        gvox_create_input_adapter(&input_ci, &result),
        "Failed to create grass 'file' input adapter\n");
    return result;
}

GvoxInputAdapter grass_zip_adapter(void) {
    GvoxInputAdapter result;

    GvoxFileInputAdapterConfig config = {0};
    config.filepath = "assets/grass.png.gz";
    GvoxInputAdapterCreateInfo file_input_ci = {0};
    file_input_ci.struct_type = GVOX_STRUCT_TYPE_INPUT_ADAPTER_CREATE_INFO;
    file_input_ci.next = NULL;
    file_input_ci.cb_args.config = &config;
    file_input_ci.adapter_chain = NULL;
    check_result(
        gvox_get_standard_input_adapter_description("file", &file_input_ci.description),
        "Failed to find standard input adapter 'file' description\n");

    GvoxInputAdapterCreateInfo zip_input_ci = {0};
    zip_input_ci.struct_type = GVOX_STRUCT_TYPE_INPUT_ADAPTER_CREATE_INFO;
    zip_input_ci.next = NULL;
    zip_input_ci.cb_args.config = NULL;
    zip_input_ci.adapter_chain = &file_input_ci;
    check_result(
        gvox_get_standard_input_adapter_description("gzip", &zip_input_ci.description),
        "Failed to find standard input adapter 'gzip' description\n");

    check_result(
        gvox_create_input_adapter(&zip_input_ci, &result),
        "Failed to create grass input adapter\n");
    return result;
}

void test_simple(void) {
    GvoxInputAdapter grass_file_input = grass_zip_adapter();

    GvoxParser grass_image_parser;
    check_result(
        gvox_create_parser_from_input(grass_file_input, &grass_image_parser),
        "Failed to find a suitable parser for the input stream\n");

    GvoxSerializer grass_colored_text_serializer;
    {
        GvoxSerializerCreateInfo serializer_ci = {0};
        serializer_ci.struct_type = GVOX_STRUCT_TYPE_SERIALIZER_CREATE_INFO;
        serializer_ci.next = NULL;
        serializer_ci.cb_args.config = NULL;
        check_result(
            gvox_get_standard_serializer_description("colored_text", &serializer_ci.description),
            "Failed to find standard serializer 'colored_text' description\n");
        check_result(
            gvox_create_serializer(&serializer_ci, &grass_colored_text_serializer),
            "Failed to create 'colored_text' parser\n");
    }

    GvoxOutputAdapter grass_stdout_output;
    {
        GvoxOutputAdapterCreateInfo output_ci = {0};
        output_ci.struct_type = GVOX_STRUCT_TYPE_OUTPUT_ADAPTER_CREATE_INFO;
        output_ci.next = NULL;
        output_ci.cb_args.config = NULL;
        check_result(
            gvox_get_standard_output_adapter_description("stdout", &output_ci.description),
            "Failed to find standard output adapter 'stdout' description\n");
        check_result(
            gvox_create_output_adapter(&output_ci, &grass_stdout_output),
            "Failed to create grass 'stdout' output adapter\n");
    }

    {
        // check_result(
        //     gvox_blit_prepare(grass_image_parser),
        //     "Failed to begin blit\n");

        GvoxBlitInfo blit_info = {0};
        blit_info.struct_type = GVOX_STRUCT_TYPE_BLIT_INFO;
        blit_info.next = NULL;
        blit_info.src = grass_image_parser;
        blit_info.dst = grass_colored_text_serializer;
        // blit_info.initial_metadata = NULL;

        check_result(
            gvox_blit(&blit_info),
            "Failed to blit\n");
    }

    gvox_destroy_output_adapter(grass_stdout_output);
    gvox_destroy_serializer(grass_colored_text_serializer);
    gvox_destroy_parser(grass_image_parser);
    gvox_destroy_input_adapter(grass_file_input);
}
#endif

int main(void) {
    gvox_init();
    test_simple();
    return 0;
}
