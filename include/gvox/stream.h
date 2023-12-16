#ifndef GVOX_STREAM_H
#define GVOX_STREAM_H

#include <gvox/core.h>

GVOX_ENUM(GvoxSeekOrigin){
    GVOX_SEEK_ORIGIN_BEG,
    GVOX_SEEK_ORIGIN_CUR,
    GVOX_SEEK_ORIGIN_END,
};

GVOX_ENUM(GvoxIteratorAdvanceMode){
    GVOX_ITERATOR_ADVANCE_MODE_NEXT,
    GVOX_ITERATOR_ADVANCE_MODE_SKIP_BRANCH,
};

GVOX_STRUCT(GvoxIteratorAdvanceInfo) {
    // TODO: Figure out if this makes sense
    GvoxInputStream input_stream;
    GvoxIteratorAdvanceMode mode;
};

GVOX_STRUCT(GvoxSample) {
    GvoxOffset offset;
    void *dst_voxel_data;
    GvoxVoxelDesc dst_voxel_desc;
};

GVOX_STRUCT(GvoxInputStreamCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxOutputStreamCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxParserCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    GvoxInputStream input_stream;
    void const *config;
};

GVOX_STRUCT(GvoxSerializerCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    GvoxOutputStream output_stream;
    void const *config;
};

GVOX_STRUCT(GvoxContainerCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxInputStreamDescription) {
    GvoxResult (*create)(void **, GvoxInputStreamCreateCbArgs const *);
    GvoxResult (*read)(void *, GvoxInputStream, void *, size_t);
    GvoxResult (*seek)(void *, GvoxInputStream, int64_t, GvoxSeekOrigin);
    int64_t (*tell)(void *, GvoxInputStream);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxOutputStreamDescription) {
    GvoxResult (*create)(void **, GvoxOutputStreamCreateCbArgs const *);
    GvoxResult (*write)(void *, GvoxOutputStream, void *, size_t);
    GvoxResult (*seek)(void *, GvoxOutputStream, int64_t, GvoxSeekOrigin);
    int64_t (*tell)(void *, GvoxOutputStream);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxParserDescription) {
    GvoxResult (*create)(void **, GvoxParserCreateCbArgs const *);
    void (*destroy)(void *);
    GvoxResult (*create_from_input)(GvoxInputStream, GvoxParser *);

    void (*create_input_iterator)(void *, void **);
    void (*destroy_iterator)(void *, void *);
    void (*iterator_advance)(void *, void **, GvoxIteratorAdvanceInfo const *, GvoxIteratorValue *);
};

GVOX_STRUCT(GvoxSerializerDescription) {
    GvoxResult (*create)(void **, GvoxSerializerCreateCbArgs const *);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxContainerDescription) {
    GvoxResult (*create)(void **, GvoxContainerCreateCbArgs const *);
    void (*destroy)(void *);
    GvoxResult (*fill)(void *, void const *, GvoxVoxelDesc, GvoxRange);
    GvoxResult (*move)(void *, GvoxContainer *, GvoxRange *, GvoxOffset *, uint32_t);
    GvoxResult (*sample)(void *, GvoxSample const *, uint32_t);
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

GVOX_ENUM(GvoxIteratorType){
    GVOX_ITERATOR_TYPE_INPUT,
    GVOX_ITERATOR_TYPE_OUTPUT,
};

GVOX_STRUCT(GvoxIteratorCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxParser parser;
    GvoxIteratorType iterator_type;
};

GVOX_STRUCT(GvoxParserDescriptionCollection) {
    GvoxStructType struct_type;
    void const *next;
    GvoxParserDescription const *descriptions;
    uint32_t description_n;
};

GVOX_EXPORT GvoxResult gvox_create_input_stream(GvoxInputStreamCreateInfo const *info, GvoxInputStream *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_output_stream(GvoxOutputStreamCreateInfo const *info, GvoxOutputStream *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_parser(GvoxParserCreateInfo const *info, GvoxParser *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_serializer(GvoxSerializerCreateInfo const *info, GvoxSerializer *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_container(GvoxContainerCreateInfo const *info, GvoxContainer *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_iterator(GvoxIteratorCreateInfo const *info, GvoxIterator *handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT void gvox_destroy_input_stream(GvoxInputStream handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_output_stream(GvoxOutputStream handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_parser(GvoxParser handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_serializer(GvoxSerializer handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_container(GvoxContainer handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_iterator(GvoxIterator handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT GvoxResult gvox_input_read(GvoxInputStream handle, void *data, size_t size) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_input_seek(GvoxInputStream handle, int64_t offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB;
GVOX_EXPORT int64_t gvox_input_tell(GvoxInputStream handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT GvoxResult gvox_output_write(GvoxOutputStream handle, void *data, size_t size) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_output_seek(GvoxOutputStream handle, int64_t offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB;
GVOX_EXPORT int64_t gvox_output_tell(GvoxOutputStream handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT GvoxResult gvox_enumerate_standard_parser_descriptions(GvoxParserDescription const **out_descriptions, uint32_t *description_n) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_create_parser_from_input(GvoxParserDescriptionCollection const *parsers, GvoxInputStream input_stream, GvoxParser *user_parser) GVOX_FUNC_ATTRIB;

GVOX_EXPORT void gvox_iterator_advance(GvoxIterator handle, GvoxIteratorAdvanceInfo const *info, GvoxIteratorValue *value);

#endif
