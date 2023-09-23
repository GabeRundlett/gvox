#ifndef GVOX_STREAM_H
#define GVOX_STREAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gvox/core.h>

GVOX_ENUM(GvoxSeekOrigin){
    GVOX_SEEK_ORIGIN_BEG,
    GVOX_SEEK_ORIGIN_CUR,
    GVOX_SEEK_ORIGIN_END,
};

GVOX_STRUCT(GvoxInputStreamCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxOutputStreamCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxParserCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    GvoxInputStream input_stream;
    void const *config;
};

GVOX_STRUCT(GvoxSerializerCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    GvoxOutputStream output_stream;
    void const *config;
};

GVOX_STRUCT(GvoxContainerCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxInputStreamDescription) {
    GvoxResult (*create)(void **, GvoxInputStreamCreateCbArgs const *);
    GvoxResult (*read)(void *, GvoxInputStream, uint8_t *, size_t);
    GvoxResult (*seek)(void *, GvoxInputStream, long, GvoxSeekOrigin);
    long (*tell)(void *, GvoxInputStream);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxOutputStreamDescription) {
    GvoxResult (*create)(void **, GvoxOutputStreamCreateCbArgs const *);
    GvoxResult (*write)(void *, GvoxOutputStream, uint8_t *, size_t);
    GvoxResult (*seek)(void *, GvoxOutputStream, long, GvoxSeekOrigin);
    long (*tell)(void *, GvoxOutputStream);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxParserDescription) {
    GvoxResult (*create)(void **, GvoxParserCreateCbArgs const *);
    void (*destroy)(void *);
    GvoxResult (*create_from_input)(GvoxInputStream, GvoxParser *);
};

GVOX_STRUCT(GvoxSerializerDescription) {
    GvoxResult (*create)(void **, GvoxSerializerCreateCbArgs const *);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxContainerDescription) {
    GvoxResult (*create)(void **, GvoxContainerCreateCbArgs const *);
    void (*destroy)(void *);
    GvoxResult (*fill)(void *, void *, GvoxVoxelDesc, GvoxRange);
    GvoxResult (*sample)(void *, void *, GvoxVoxelDesc, GvoxOffset);
};

GVOX_STRUCT(GvoxInputStreamCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    struct GvoxInputStreamCreateInfo const *stream_chain;
    GvoxInputStreamDescription description;
    GvoxInputStreamCreateCbArgs cb_args;
};

GVOX_STRUCT(GvoxOutputStreamCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    void const *stream_chain;
    GvoxOutputStreamDescription description;
    GvoxOutputStreamCreateCbArgs cb_args;
};

GVOX_STRUCT(GvoxParserCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxParserDescription description;
    GvoxParserCreateCbArgs cb_args;
};

GVOX_STRUCT(GvoxSerializerCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxSerializerDescription description;
    GvoxSerializerCreateCbArgs cb_args;
};

GVOX_STRUCT(GvoxContainerCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainerDescription description;
    GvoxContainerCreateCbArgs cb_args;
};

GVOX_EXPORT GvoxResult gvox_create_input_stream(GvoxInputStreamCreateInfo const *info, GvoxInputStream *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_output_stream(GvoxOutputStreamCreateInfo const *info, GvoxOutputStream *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_parser(GvoxParserCreateInfo const *info, GvoxParser *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_serializer(GvoxSerializerCreateInfo const *info, GvoxSerializer *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_container(GvoxContainerCreateInfo const *info, GvoxContainer *handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT void gvox_destroy_input_stream(GvoxInputStream handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_output_stream(GvoxOutputStream handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_parser(GvoxParser handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_serializer(GvoxSerializer handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_container(GvoxContainer handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT GvoxResult gvox_input_read(GvoxInputStream handle, uint8_t *data, size_t size) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_input_seek(GvoxInputStream handle, long offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB;
GVOX_EXPORT long gvox_input_tell(GvoxInputStream handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT GvoxResult gvox_output_write(GvoxOutputStream handle, uint8_t *data, size_t size) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_output_seek(GvoxOutputStream handle, long offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB;
GVOX_EXPORT long gvox_output_tell(GvoxOutputStream handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT GvoxResult gvox_create_parser_from_input(GvoxParserDescription const *parsers_ptr, uint32_t parser_n, GvoxInputStream input_stream, GvoxParser *user_parser) GVOX_FUNC_ATTRIB;

#ifdef __cplusplus
}
#endif

#endif
