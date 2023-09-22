#ifndef GVOX_ADAPTER_H
#define GVOX_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gvox/core.h>

GVOX_ENUM(GvoxSeekOrigin){
    GVOX_SEEK_ORIGIN_BEG,
    GVOX_SEEK_ORIGIN_CUR,
    GVOX_SEEK_ORIGIN_END,
};

GVOX_STRUCT(GvoxInputAdapterCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxOutputAdapterCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxParserCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    GvoxInputAdapter input_adapter;
    void const *config;
};

GVOX_STRUCT(GvoxSerializerCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    GvoxOutputAdapter output_adapter;
    void const *config;
};

GVOX_STRUCT(GvoxContainerCreateCbArgs) {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxInputAdapterDescription) {
    GvoxResult (*create)(void **, GvoxInputAdapterCreateCbArgs const *);
    GvoxResult (*read)(void *, GvoxInputAdapter, uint8_t *, size_t);
    GvoxResult (*seek)(void *, GvoxInputAdapter, long, GvoxSeekOrigin);
    long (*tell)(void *, GvoxInputAdapter);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxOutputAdapterDescription) {
    GvoxResult (*create)(void **, GvoxOutputAdapterCreateCbArgs const *);
    GvoxResult (*write)(void *, GvoxOutputAdapter, uint8_t *, size_t);
    GvoxResult (*seek)(void *, GvoxOutputAdapter, long, GvoxSeekOrigin);
    long (*tell)(void *, GvoxOutputAdapter);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxParserDescription) {
    GvoxResult (*create)(void **, GvoxParserCreateCbArgs const *);
    void (*destroy)(void *);
    GvoxResult (*create_from_input)(GvoxInputAdapter, GvoxParser *);
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

GVOX_STRUCT(GvoxInputAdapterCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    struct GvoxInputAdapterCreateInfo const *adapter_chain;
    GvoxInputAdapterDescription description;
    GvoxInputAdapterCreateCbArgs cb_args;
};

GVOX_STRUCT(GvoxOutputAdapterCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    void const *adapter_chain;
    GvoxOutputAdapterDescription description;
    GvoxOutputAdapterCreateCbArgs cb_args;
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

GvoxResult GVOX_EXPORT gvox_create_input_adapter(GvoxInputAdapterCreateInfo const *info, GvoxInputAdapter *handle) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_create_output_adapter(GvoxOutputAdapterCreateInfo const *info, GvoxOutputAdapter *handle) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_create_parser(GvoxParserCreateInfo const *info, GvoxParser *handle) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_create_serializer(GvoxSerializerCreateInfo const *info, GvoxSerializer *handle) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_create_container(GvoxContainerCreateInfo const *info, GvoxContainer *handle) GVOX_FUNC_ATTRIB;

void GVOX_EXPORT gvox_destroy_input_adapter(GvoxInputAdapter handle) GVOX_FUNC_ATTRIB;
void GVOX_EXPORT gvox_destroy_output_adapter(GvoxOutputAdapter handle) GVOX_FUNC_ATTRIB;
void GVOX_EXPORT gvox_destroy_parser(GvoxParser handle) GVOX_FUNC_ATTRIB;
void GVOX_EXPORT gvox_destroy_serializer(GvoxSerializer handle) GVOX_FUNC_ATTRIB;
void GVOX_EXPORT gvox_destroy_container(GvoxContainer handle) GVOX_FUNC_ATTRIB;

GvoxResult GVOX_EXPORT gvox_get_standard_input_adapter_description(char const *name, GvoxInputAdapterDescription *desc) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_get_standard_output_adapter_description(char const *name, GvoxOutputAdapterDescription *desc) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_get_standard_parser_description(char const *name, GvoxParserDescription *desc) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_get_standard_serializer_description(char const *name, GvoxSerializerDescription *desc) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_get_standard_container_description(char const *name, GvoxContainerDescription *desc) GVOX_FUNC_ATTRIB;

GvoxResult GVOX_EXPORT gvox_input_read(GvoxInputAdapter handle, uint8_t *data, size_t size) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_input_seek(GvoxInputAdapter handle, long offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB;
long GVOX_EXPORT gvox_input_tell(GvoxInputAdapter handle) GVOX_FUNC_ATTRIB;

GvoxResult GVOX_EXPORT gvox_output_write(GvoxOutputAdapter handle, uint8_t *data, size_t size) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_output_seek(GvoxOutputAdapter handle, long offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB;
long GVOX_EXPORT gvox_output_tell(GvoxOutputAdapter handle) GVOX_FUNC_ATTRIB;

GvoxResult GVOX_EXPORT gvox_create_parser_from_input(GvoxInputAdapter input_adapter, GvoxParser *user_parser) GVOX_FUNC_ATTRIB;

#ifdef __cplusplus
}
#endif

#endif
