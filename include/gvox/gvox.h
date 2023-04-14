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

GVOX_DEFINE_HANDLE(IoAdapter);
GVOX_DEFINE_HANDLE(Container);

// Consumer API

typedef enum GvoxResult {
    GVOX_SUCCESS = 0,
    GVOX_ERROR_UNKNOWN = -1,
    GVOX_ERROR_INVALID_ARGUMENT = -2,
    GVOX_ERROR_BAD_STRUCT_TYPE = -3,
    GVOX_ERROR_UNKNOWN_STANDARD_IO_ADAPTER = -4,
    GVOX_ERROR_UNKNOWN_STANDARD_CONTAINER = -5,
} GvoxResult;

typedef enum GvoxStructType {
    GVOX_STRUCT_TYPE_IO_ADAPTER_CREATE_INFO,
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
    GvoxIoAdapter input_adapter;
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

typedef struct GvoxIoAdapterDescription {
    GvoxResult (*create)(void **, void const *);
    GvoxResult (*input_read)(void *, uint8_t *, size_t);
    GvoxResult (*input_seek)(void *, long, GvoxSeekOrigin);
    void (*destroy)(void *);
} GvoxIoAdapterDescription;

typedef struct GvoxContainerDescription {
    GvoxResult (*create)(void **, void const *);
    GvoxResult (*parse)(void *, GvoxParseCbInfo const *);
    GvoxResult (*serialize)(void *);
    void (*destroy)(void *);
} GvoxContainerDescription;

typedef struct GvoxIoAdapterCreateInfo {
    GvoxStructType type;
    void const *next;
    GvoxIoAdapterDescription description;
    void const *config;
} GvoxIoAdapterCreateInfo;

typedef struct GvoxContainerCreateInfo {
    GvoxStructType type;
    void const *next;
    GvoxContainerDescription description;
    void const *config;
} GvoxContainerCreateInfo;

typedef struct GvoxParseInfo {
    GvoxStructType type;
    void const *next;
    GvoxIoAdapter src;
    GvoxContainer dst;
    GvoxTransform transform;
} GvoxParseInfo;

typedef struct GvoxBlitInfo {
    GvoxStructType type;
    void const *next;
    GvoxContainer src;
    GvoxContainer dst;
} GvoxBlitInfo;

typedef struct GvoxSerializeInfo {
    GvoxStructType type;
    void const *next;
    GvoxContainer src;
    GvoxIoAdapter dst;
} GvoxSerializeInfo;

// Consumer API

GvoxResult GVOX_EXPORT gvox_create_io_adapter(GvoxIoAdapterCreateInfo const *info_ptr, GvoxIoAdapter *io_adapter);
void GVOX_EXPORT gvox_destroy_io_adapter(GvoxIoAdapter io_adapter);

GvoxResult GVOX_EXPORT gvox_create_container(GvoxContainerCreateInfo const *info_ptr, GvoxContainer *container);
void GVOX_EXPORT gvox_destroy_container(GvoxContainer container);

GvoxResult GVOX_EXPORT gvox_get_standard_io_adapter_description(char const *name, GvoxIoAdapterDescription *desc);
GvoxResult GVOX_EXPORT gvox_get_standard_container_description(char const *name, GvoxContainerDescription *desc);

GvoxResult GVOX_EXPORT gvox_parse(GvoxParseInfo const *info);
GvoxResult GVOX_EXPORT gvox_blit(GvoxBlitInfo const *info);
GvoxResult GVOX_EXPORT gvox_serialize(GvoxSerializeInfo const *info);

// Adapter API

GvoxResult GVOX_EXPORT gvox_input_read(GvoxIoAdapter input_adapter, uint8_t *data, size_t size);
GvoxResult GVOX_EXPORT gvox_input_seek(GvoxIoAdapter input_adapter, long offset, GvoxSeekOrigin origin);

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
