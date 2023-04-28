#include <gvox/gvox.h>
#include <gvox/streams/input/file.h>

#include <stdio.h>
#include <stdlib.h>

void check_result(GvoxResult result, char const *failure_message) {
    if (result != GVOX_SUCCESS) {
        printf("[GVOX ERROR CODE %d]: %s", (int)result, failure_message);
        exit(-1);
    }
}

void test_simple(void) {
    GvoxInputStream grass_file_input;
    {
        GvoxFileInputStreamConfig config = {0};
        config.filepath = "assets/grass.png";
        GvoxInputStreamCreateInfo input_ci = {0};
        input_ci.struct_type = GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_INFO;
        input_ci.next = NULL;
        input_ci.cb_args.config = &config;
        check_result(
            gvox_get_standard_input_stream_description("file", &input_ci.description),
            "Failed to find standard input stream 'file' description\n");
        check_result(
            gvox_create_input_stream(&input_ci, &grass_file_input),
            "Failed to create grass 'file' input stream\n");
    }

    GvoxParser grass_image_parser;
    {
        GvoxParserCreateInfo parser_ci = {0};
        parser_ci.struct_type = GVOX_STRUCT_TYPE_PARSER_CREATE_INFO;
        parser_ci.next = NULL;
        parser_ci.cb_args.config = NULL;
        parser_ci.cb_args.input_stream = grass_file_input;
        check_result(
            gvox_get_standard_parser_description("image", &parser_ci.description),
            "Failed to find standard parser 'image' description\n");
        check_result(
            gvox_create_parser(&parser_ci, &grass_image_parser),
            "Failed to create 'image' parser\n");
    }

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

    GvoxOutputStream grass_stdout_output;
    {
        GvoxOutputStreamCreateInfo output_ci = {0};
        output_ci.struct_type = GVOX_STRUCT_TYPE_OUTPUT_STREAM_CREATE_INFO;
        output_ci.next = NULL;
        output_ci.cb_args.config = NULL;
        check_result(
            gvox_get_standard_output_stream_description("stdout", &output_ci.description),
            "Failed to find standard output stream 'stdout' description\n");
        check_result(
            gvox_create_output_stream(&output_ci, &grass_stdout_output),
            "Failed to create grass 'stdout' output stream\n");
    }

    {
        GvoxBlitInfo blit_info = {0};
        blit_info.struct_type = GVOX_STRUCT_TYPE_BLIT_INFO;
        blit_info.next = NULL;
        blit_info.src = grass_image_parser;
        blit_info.dst = grass_colored_text_serializer;
        check_result(
            gvox_blit(&blit_info),
            "Failed to blit\n");
    }

    gvox_destroy_output_stream(grass_stdout_output);
    gvox_destroy_serializer(grass_colored_text_serializer);
    gvox_destroy_parser(grass_image_parser);
    gvox_destroy_input_stream(grass_file_input);
}

int main(void) {
    test_simple();
    return 0;
}
