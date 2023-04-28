#ifndef GVOX_GVOX_H
#define GVOX_GVOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#if !defined(GVOX_EXPORT)
#define GVOX_EXPORT
#endif

#define GVOX_DEFINE_HANDLE(NAME) typedef struct NAME##_ImplT *NAME

GVOX_DEFINE_HANDLE(GvoxInputStream);
GVOX_DEFINE_HANDLE(GvoxOutputStream);
GVOX_DEFINE_HANDLE(GvoxInputAdapter);
GVOX_DEFINE_HANDLE(GvoxOutputAdapter);
GVOX_DEFINE_HANDLE(GvoxParser);
GVOX_DEFINE_HANDLE(GvoxSerializer);
GVOX_DEFINE_HANDLE(GvoxContainer);

// Consumer API

typedef enum GvoxResult {
    GVOX_SUCCESS = 0,
    GVOX_ERROR_UNKNOWN = -1,
    GVOX_ERROR_UNKNOWN_STANDARD_INPUT_STREAM = -2,
    GVOX_ERROR_UNKNOWN_STANDARD_OUTPUT_STREAM = -3,
    GVOX_ERROR_UNKNOWN_STANDARD_INPUT_ADAPTER = -4,
    GVOX_ERROR_UNKNOWN_STANDARD_OUTPUT_ADAPTER = -5,
    GVOX_ERROR_UNKNOWN_STANDARD_PARSER = -6,
    GVOX_ERROR_UNKNOWN_STANDARD_SERIALIZER = -7,
    GVOX_ERROR_UNKNOWN_STANDARD_CONTAINER = -8,
    GVOX_ERROR_INVALID_ARGUMENT = -9,
    GVOX_ERROR_BAD_STRUCT_TYPE = -10,
    GVOX_ERROR_BAD_ADAPTER_CHAIN = -11,
} GvoxResult;

typedef enum GvoxStructType {
    GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_INFO,
    GVOX_STRUCT_TYPE_OUTPUT_STREAM_CREATE_INFO,
    GVOX_STRUCT_TYPE_PARSER_CREATE_INFO,
    GVOX_STRUCT_TYPE_SERIALIZER_CREATE_INFO,
    GVOX_STRUCT_TYPE_INPUT_ADAPTER_CREATE_INFO,
    GVOX_STRUCT_TYPE_OUTPUT_ADAPTER_CREATE_INFO,
    GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,

    GVOX_STRUCT_TYPE_PARSE_INFO,
    GVOX_STRUCT_TYPE_BLIT_INFO,
    GVOX_STRUCT_TYPE_SERIALIZE_INFO,
} GvoxStructType;

typedef enum GvoxSeekOrigin {
    GVOX_SEEK_ORIGIN_BEG,
    GVOX_SEEK_ORIGIN_CUR,
    GVOX_SEEK_ORIGIN_END,
} GvoxSeekOrigin;

typedef union GvoxTranslation {
    struct {
        float x, y, z, w;
    } axis;
    float data[4];
} GvoxTranslation;

typedef union GvoxRotation {
    struct {
        float x, y, z, w;
    } axis;
    float data[4];
} GvoxRotation;

typedef union GvoxScale {
    struct {
        float x, y, z, w;
    } axis;
    float data[4];
} GvoxScale;

typedef struct GvoxTransform {
    uint32_t dimension_n;
    float data[25];
} GvoxTransform;

typedef struct GvoxInputStreamCreateCbArgs {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
} GvoxInputStreamCreateCbArgs;

typedef struct GvoxOutputStreamCreateCbArgs {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
} GvoxOutputStreamCreateCbArgs;

typedef struct GvoxInputAdapterCreateCbArgs {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
} GvoxInputAdapterCreateCbArgs;

typedef struct GvoxOutputAdapterCreateCbArgs {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
} GvoxOutputAdapterCreateCbArgs;

typedef struct GvoxParserCreateCbArgs {
    GvoxStructType struct_type; // implied?
    void const *next;
    GvoxInputStream input_stream;
    void const *config;
} GvoxParserCreateCbArgs;

typedef struct GvoxSerializerCreateCbArgs {
    GvoxStructType struct_type; // implied?
    void const *next;
    GvoxOutputStream output_stream;
    void const *config;
} GvoxSerializerCreateCbArgs;

typedef struct GvoxContainerCreateCbArgs {
    GvoxStructType struct_type; // implied?
    void const *next;
    void const *config;
} GvoxContainerCreateCbArgs;

#if 0
typedef enum GvoxStorageFormat {
    GVOX_STORAGE_FORMAT_UNDEFINED,
    GVOX_STORAGE_FORMAT_UNORM,
    GVOX_STORAGE_FORMAT_SNORM,
    GVOX_STORAGE_FORMAT_USCALED,
    GVOX_STORAGE_FORMAT_SSCALED,
    GVOX_STORAGE_FORMAT_UINT,
    GVOX_STORAGE_FORMAT_SINT,
    GVOX_STORAGE_FORMAT_SRGB,
} GvoxStorageFormat;

typedef enum GvoxStorageType {
    GVOX_STORAGE_TYPE_UNDEFINED,
    GVOX_STORAGE_TYPE_I8,
    GVOX_STORAGE_TYPE_I16,
    GVOX_STORAGE_TYPE_I32,
    GVOX_STORAGE_TYPE_I64,
    GVOX_STORAGE_TYPE_U8,
    GVOX_STORAGE_TYPE_U16,
    GVOX_STORAGE_TYPE_U32,
    GVOX_STORAGE_TYPE_U64,
    GVOX_STORAGE_TYPE_F32,
    GVOX_STORAGE_TYPE_F64,
    GVOX_STORAGE_TYPE_SIZED_UINT = 1 << 24,
} GvoxStorageType;

typedef enum GvoxFilter {
    GVOX_FILTER_NEAREST,
    GVOX_FILTER_LINEAR,
} GvoxFilter;

typedef enum GvoxSamplerAddressMode {
    GVOX_SAMPLER_ADDRESS_MODE_REPEAT,
    GVOX_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    GVOX_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    GVOX_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
} GvoxSamplerAddressMode;

typedef struct GvoxExtent {
    uint32_t dimension_n;
    float *elements;
} GvoxExtent;
#endif

// Adapter Descriptions

typedef struct GvoxInputStreamDescription {
    GvoxResult (*create)(void **, GvoxInputStreamCreateCbArgs const *);
    GvoxResult (*read)(void *, uint8_t *, size_t);
    GvoxResult (*seek)(void *, long, GvoxSeekOrigin);
    long (*tell)(void *);
    void (*destroy)(void *);
} GvoxInputStreamDescription;

typedef struct GvoxOutputStreamDescription {
    GvoxResult (*create)(void **, GvoxOutputStreamCreateCbArgs const *);
    GvoxResult (*write)(void *, uint8_t *, size_t);
    GvoxResult (*seek)(void *, long, GvoxSeekOrigin);
    long (*tell)(void *);
    void (*destroy)(void *);
} GvoxOutputStreamDescription;

typedef struct GvoxInputAdapterDescription {
    GvoxResult (*create)(void **, GvoxInputAdapterCreateCbArgs const *);
    void (*destroy)(void *);
} GvoxInputAdapterDescription;

typedef struct GvoxOutputAdapterDescription {
    GvoxResult (*create)(void **, GvoxOutputAdapterCreateCbArgs const *);
    void (*destroy)(void *);
} GvoxOutputAdapterDescription;

typedef struct GvoxParserDescription {
    GvoxResult (*create)(void **, GvoxParserCreateCbArgs const *);
    void (*destroy)(void *);
} GvoxParserDescription;

typedef struct GvoxSerializerDescription {
    GvoxResult (*create)(void **, GvoxSerializerCreateCbArgs const *);
    void (*destroy)(void *);
} GvoxSerializerDescription;

typedef struct GvoxContainerDescription {
    GvoxResult (*create)(void **, GvoxContainerCreateCbArgs const *);
    void (*destroy)(void *);
} GvoxContainerDescription;

// Adapter Create Infos

typedef struct GvoxInputStreamCreateInfo {
    GvoxStructType struct_type;
    void const *next;
    void const *adapter_chain;
    GvoxInputStreamDescription description;
    GvoxInputStreamCreateCbArgs cb_args;
} GvoxInputStreamCreateInfo;

typedef struct GvoxOutputStreamCreateInfo {
    GvoxStructType struct_type;
    void const *next;
    void const *adapter_chain;
    GvoxOutputStreamDescription description;
    GvoxOutputStreamCreateCbArgs cb_args;
} GvoxOutputStreamCreateInfo;

typedef struct GvoxInputAdapterCreateInfo {
    GvoxStructType struct_type;
    void const *next;
    GvoxInputAdapterDescription description;
    GvoxInputAdapterCreateCbArgs cb_args;
} GvoxInputAdapterCreateInfo;

typedef struct GvoxOutputAdapterCreateInfo {
    GvoxStructType struct_type;
    void const *next;
    GvoxOutputAdapterDescription description;
    GvoxOutputAdapterCreateCbArgs cb_args;
} GvoxOutputAdapterCreateInfo;

typedef struct GvoxParserCreateInfo {
    GvoxStructType struct_type;
    void const *next;
    GvoxParserDescription description;
    GvoxParserCreateCbArgs cb_args;
} GvoxParserCreateInfo;

typedef struct GvoxSerializerCreateInfo {
    GvoxStructType struct_type;
    void const *next;
    GvoxSerializerDescription description;
    GvoxSerializerCreateCbArgs cb_args;
} GvoxSerializerCreateInfo;

typedef struct GvoxContainerCreateInfo {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainerDescription description;
    GvoxContainerCreateCbArgs cb_args;
} GvoxContainerCreateInfo;

typedef struct GvoxBlitInfo {
    GvoxStructType struct_type;
    void const *next;
    GvoxParser src;
    GvoxSerializer dst;
} GvoxBlitInfo;

typedef struct GvoxSerializeInfo {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainer src;
    GvoxInputStream dst;
} GvoxSerializeInfo;

// Consumer API

GvoxResult GVOX_EXPORT gvox_create_input_stream(GvoxInputStreamCreateInfo const *info, GvoxInputStream *handle);
GvoxResult GVOX_EXPORT gvox_create_output_stream(GvoxOutputStreamCreateInfo const *info, GvoxOutputStream *handle);
GvoxResult GVOX_EXPORT gvox_create_input_adapter(GvoxInputAdapterCreateInfo const *info, GvoxInputAdapter *handle);
GvoxResult GVOX_EXPORT gvox_create_output_adapter(GvoxOutputAdapterCreateInfo const *info, GvoxOutputAdapter *handle);
GvoxResult GVOX_EXPORT gvox_create_parser(GvoxParserCreateInfo const *info, GvoxParser *handle);
GvoxResult GVOX_EXPORT gvox_create_serializer(GvoxSerializerCreateInfo const *info, GvoxSerializer *handle);
GvoxResult GVOX_EXPORT gvox_create_container(GvoxContainerCreateInfo const *info, GvoxContainer *handle);

void GVOX_EXPORT gvox_destroy_input_stream(GvoxInputStream handle);
void GVOX_EXPORT gvox_destroy_output_stream(GvoxOutputStream handle);
void GVOX_EXPORT gvox_destroy_input_adapter(GvoxInputAdapter handle);
void GVOX_EXPORT gvox_destroy_output_adapter(GvoxOutputAdapter handle);
void GVOX_EXPORT gvox_destroy_parser(GvoxParser handle);
void GVOX_EXPORT gvox_destroy_serializer(GvoxSerializer handle);
void GVOX_EXPORT gvox_destroy_container(GvoxContainer handle);

GvoxResult GVOX_EXPORT gvox_get_standard_input_stream_description(char const *name, GvoxInputStreamDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_output_stream_description(char const *name, GvoxOutputStreamDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_input_adapter_description(char const *name, GvoxInputAdapterDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_output_adapter_description(char const *name, GvoxOutputAdapterDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_parser_description(char const *name, GvoxParserDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_serializer_description(char const *name, GvoxSerializerDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_container_description(char const *name, GvoxContainerDescription *desc);

GvoxResult GVOX_EXPORT gvox_blit(GvoxBlitInfo const *info);

// Adapter API

GvoxResult GVOX_EXPORT gvox_input_read(GvoxInputStream handle, uint8_t *data, size_t size);
GvoxResult GVOX_EXPORT gvox_input_seek(GvoxInputStream handle, long offset, GvoxSeekOrigin origin);
long GVOX_EXPORT gvox_input_tell(GvoxInputStream handle);

GvoxResult GVOX_EXPORT gvox_output_write(GvoxOutputStream handle, uint8_t *data, size_t size);
GvoxResult GVOX_EXPORT gvox_output_seek(GvoxOutputStream handle, long offset, GvoxSeekOrigin origin);
long GVOX_EXPORT gvox_output_tell(GvoxOutputStream handle);

#undef GVOX_DEFINE_HANDLE

#ifdef __cplusplus
}
#endif

#endif
