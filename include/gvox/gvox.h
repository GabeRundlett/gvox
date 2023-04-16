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

#define GVOX_DEFINE_HANDLE(NAME) typedef struct Gvox##NAME##_ImplT *Gvox##NAME

GVOX_DEFINE_HANDLE(InputStream);
GVOX_DEFINE_HANDLE(OutputStream);
GVOX_DEFINE_HANDLE(ParseAdapter);
GVOX_DEFINE_HANDLE(SerializeAdapter);
GVOX_DEFINE_HANDLE(Parser);
GVOX_DEFINE_HANDLE(Serializer);
GVOX_DEFINE_HANDLE(Container);

// Consumer API

typedef enum GvoxResult {
    GVOX_SUCCESS = 0,
    GVOX_ERROR_UNKNOWN = -1,
    GVOX_ERROR_UNKNOWN_STANDARD_INPUT_STREAM = -2,
    GVOX_ERROR_UNKNOWN_STANDARD_OUTPUT_STREAM = -3,
    GVOX_ERROR_UNKNOWN_STANDARD_PARSE_ADAPTER = -4,
    GVOX_ERROR_UNKNOWN_STANDARD_SERIALIZE_ADAPTER = -5,
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
    GVOX_STRUCT_TYPE_PARSE_ADAPTER_CREATE_INFO,
    GVOX_STRUCT_TYPE_SERIALIZE_ADAPTER_CREATE_INFO,
    GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,

    GVOX_STRUCT_TYPE_PARSE_INFO,
    GVOX_STRUCT_TYPE_BLIT_INFO,
    GVOX_STRUCT_TYPE_SERIALIZE_INFO,
    GVOX_STRUCT_TYPE_PARSE_CB_INFO,
} GvoxStructType;

typedef enum GvoxSeekOrigin {
    GVOX_SEEK_ORIGIN_BEG,
    GVOX_SEEK_ORIGIN_END,
    GVOX_SEEK_ORIGIN_CUR,
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

typedef struct GvoxParseCbInfo {
    GvoxStructType type;
    void const *next;
    GvoxInputStream input_stream;
    GvoxTransform transform;
} GvoxParseCbInfo;

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
    GvoxResult (*create)(void **, void const *);
    GvoxResult (*read)(void *, uint8_t *, size_t);
    GvoxResult (*seek)(void *, long, GvoxSeekOrigin);
    void (*destroy)(void *);
} GvoxInputStreamDescription;

typedef struct GvoxOutputStreamDescription {
    GvoxResult (*create)(void **, void const *);
    GvoxResult (*write)(void *, uint8_t *, size_t);
    GvoxResult (*seek)(void *, long, GvoxSeekOrigin);
    void (*destroy)(void *);
} GvoxOutputStreamDescription;

typedef struct GvoxParseAdapterDescription {
    GvoxResult (*create)(void **, void const *);
    void (*destroy)(void *);
} GvoxParseAdapterDescription;

typedef struct GvoxSerializeAdapterDescription {
    GvoxResult (*create)(void **, void const *);
    void (*destroy)(void *);
} GvoxSerializeAdapterDescription;

typedef struct GvoxParserDescription {
    GvoxResult (*create)(void **, void const *);
    void (*destroy)(void *);
} GvoxParserDescription;

typedef struct GvoxSerializerDescription {
    GvoxResult (*create)(void **, void const *);
    void (*destroy)(void *);
} GvoxSerializerDescription;

typedef struct GvoxContainerDescription {
    GvoxResult (*create)(void **, void const *);
    GvoxResult (*parse)(void *, GvoxParseCbInfo const *);
    GvoxResult (*serialize)(void *);
    void (*destroy)(void *);
} GvoxContainerDescription;

// Adapter Create Infos

typedef struct GvoxInputStreamCreateInfo {
    GvoxStructType type;
    void const *next;
    GvoxInputStreamDescription description;
    void const *config;
} GvoxInputStreamCreateInfo;

typedef struct GvoxOutputStreamCreateInfo {
    GvoxStructType type;
    void const *next;
    GvoxOutputStreamDescription description;
    void const *config;
} GvoxOutputStreamCreateInfo;

typedef struct GvoxParseAdapterCreateInfo {
    GvoxStructType type;
    void const *next;
    GvoxParseAdapterDescription description;
    void const *config;
} GvoxParseAdapterCreateInfo;

typedef struct GvoxSerializeAdapterCreateInfo {
    GvoxStructType type;
    void const *next;
    GvoxSerializeAdapterDescription description;
    void const *config;
} GvoxSerializeAdapterCreateInfo;

typedef struct GvoxParserCreateInfo {
    GvoxStructType type;
    void const *next;
    GvoxInputStream input_stream;
    void const *adapter_chain;
    GvoxParserDescription description;
    void const *config;
} GvoxParserCreateInfo;

typedef struct GvoxSerializerCreateInfo {
    GvoxStructType type;
    void const *next;
    GvoxInputStream input_stream;
    void const *adapter_chain;
    GvoxSerializerDescription description;
    void const *config;
} GvoxSerializerCreateInfo;

typedef struct GvoxContainerCreateInfo {
    GvoxStructType type;
    void const *next;
    GvoxContainerDescription description;
    void const *config;
} GvoxContainerCreateInfo;

typedef struct GvoxParseInfo {
    GvoxStructType type;
    void const *next;
    GvoxInputStream src;
    GvoxContainer dst;
    GvoxTransform transform;
} GvoxParseInfo;

typedef struct GvoxBlitInfo {
    GvoxStructType type;
    void const *next;
    GvoxParser src;
    GvoxSerializer dst;
} GvoxBlitInfo;

typedef struct GvoxSerializeInfo {
    GvoxStructType type;
    void const *next;
    GvoxContainer src;
    GvoxInputStream dst;
} GvoxSerializeInfo;

// Consumer API

GvoxResult GVOX_EXPORT gvox_create_input_stream(GvoxInputStreamCreateInfo const *info, GvoxInputStream *handle);
GvoxResult GVOX_EXPORT gvox_create_output_stream(GvoxOutputStreamCreateInfo const *info, GvoxOutputStream *handle);
GvoxResult GVOX_EXPORT gvox_create_parse_adapter(GvoxParseAdapterCreateInfo const *info, GvoxParseAdapter *handle);
GvoxResult GVOX_EXPORT gvox_create_serialize_adapter(GvoxSerializeAdapterCreateInfo const *info, GvoxSerializeAdapter *handle);
GvoxResult GVOX_EXPORT gvox_create_parser(GvoxParserCreateInfo const *info, GvoxParser *handle);
GvoxResult GVOX_EXPORT gvox_create_serializer(GvoxSerializerCreateInfo const *info, GvoxSerializer *handle);
GvoxResult GVOX_EXPORT gvox_create_container(GvoxContainerCreateInfo const *info, GvoxContainer *handle);

void GVOX_EXPORT gvox_destroy_input_stream(GvoxInputStream handle);
void GVOX_EXPORT gvox_destroy_output_stream(GvoxOutputStream handle);
void GVOX_EXPORT gvox_destroy_parse_adapter(GvoxParseAdapter handle);
void GVOX_EXPORT gvox_destroy_serialize_adapter(GvoxSerializeAdapter handle);
void GVOX_EXPORT gvox_destroy_parser(GvoxParser handle);
void GVOX_EXPORT gvox_destroy_serializer(GvoxSerializer handle);
void GVOX_EXPORT gvox_destroy_container(GvoxContainer handle);

GvoxResult GVOX_EXPORT gvox_get_standard_input_stream_description(char const *name, GvoxInputStreamDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_output_stream_description(char const *name, GvoxOutputStreamDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_parse_adapter_description(char const *name, GvoxParseAdapterDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_serialize_adapter_description(char const *name, GvoxSerializeAdapterDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_parser_description(char const *name, GvoxParserDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_serializer_description(char const *name, GvoxSerializerDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_container_description(char const *name, GvoxContainerDescription *desc);

GvoxResult GVOX_EXPORT gvox_parse(GvoxParseInfo const *info);
GvoxResult GVOX_EXPORT gvox_blit(GvoxBlitInfo const *info);
GvoxResult GVOX_EXPORT gvox_serialize(GvoxSerializeInfo const *info);

// Adapter API

GvoxResult GVOX_EXPORT gvox_input_read(GvoxInputStream handle, uint8_t *data, size_t size);
GvoxResult GVOX_EXPORT gvox_input_seek(GvoxInputStream handle, long offset, GvoxSeekOrigin origin);

GvoxResult GVOX_EXPORT gvox_output_write(GvoxOutputStream handle, uint8_t *data, size_t size);
GvoxResult GVOX_EXPORT gvox_output_seek(GvoxOutputStream handle, long offset, GvoxSeekOrigin origin);

// Utilities

GvoxTransform GVOX_EXPORT gvox_identity_transform(uint32_t dimension);
GvoxTransform GVOX_EXPORT gvox_inverse_transform(GvoxTransform const *transform);
GvoxTranslation GVOX_EXPORT gvox_apply_transform(GvoxTransform const *transform, GvoxTranslation point);
void GVOX_EXPORT gvox_translate(GvoxTransform *transform, GvoxTranslation trn);
void GVOX_EXPORT gvox_rotate(GvoxTransform *transform, GvoxRotation rot);
void GVOX_EXPORT gvox_scale(GvoxTransform *transform, GvoxScale scl);

#undef GVOX_DEFINE_HANDLE

#ifdef __cplusplus
}
#endif

#endif
