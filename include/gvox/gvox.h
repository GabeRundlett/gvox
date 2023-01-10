#ifndef GVOX_H
#define GVOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef enum {
    GVOX_SUCCESS = 0,
    GVOX_ERROR_FAILED_TO_LOAD_FILE = -1,
    GVOX_ERROR_FAILED_TO_LOAD_FORMAT = -2,
    GVOX_ERROR_INVALID_FORMAT = -3,
} GVoxResult;

typedef uint32_t GVoxVoxel;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} GVoxOffset3D;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} GVoxExtent3D;

#define GVOX_REGION_IS_UNIFORM_BIT 0x00000001

typedef struct {
    GVoxOffset3D offset;
    GVoxExtent3D extent;
    uint32_t flags;
} GVoxRegionMetadata;

typedef struct {
    GVoxRegionMetadata metadata;
    GVoxVoxel *data;
} GVoxRegion;

typedef struct {
    GVoxRegion *regions;
    size_t region_n;
} GVoxRegionList;

typedef struct _GVoxContext GVoxContext;
typedef struct _GVoxInputAdapter GVoxInputAdapter;
typedef struct _GVoxOutputAdapter GVoxOutputAdapter;
typedef struct _GVoxFormatAdapter GVoxFormatAdapter;

typedef struct {
    GVoxRegionList region_list;

    GVoxInputAdapter *input_adapter;
    void const *input_config;
    void *input_user_ptr;

    GVoxFormatAdapter *format_adapter;
    void const *format_config;
    void *format_user_ptr;
} GVoxParseState;

typedef struct {
    GVoxRegionMetadata const *full_region_metadata;

    GVoxOutputAdapter *output_adapter;
    void const *output_config;
    void *output_user_ptr;

    GVoxFormatAdapter *format_adapter;
    void const *format_config;
    void *format_user_ptr;
} GVoxSerializeState;

typedef struct {
    char const *name_str;

    void (*begin)(GVoxParseState *state);
    void (*end)(GVoxParseState *state);

    void (*read)(GVoxParseState *state, size_t position, size_t size, void **data);
} GVoxInputAdapterInfo;

typedef struct {
    char const *name_str;

    void (*begin)(GVoxSerializeState *state);
    void (*end)(GVoxSerializeState *state);

    void (*write)(GVoxSerializeState *state, size_t position, size_t size, void const *data);
    void (*reserve)(GVoxSerializeState *state, size_t size);
} GVoxOutputAdapterInfo;

typedef struct {
    char const *name_str;
    void *(*create_context)(void);
    void (*destroy_context)(void *);

    void (*serialize_begin)(GVoxSerializeState *state);
    void (*serialize_end)(GVoxSerializeState *state);
    void (*serialize_region)(GVoxSerializeState *state, GVoxRegion const *region);

    void (*parse)(GVoxParseState *state);
} GVoxFormatAdapterInfo;

GVoxContext *gvox_create_context(void);
void gvox_destroy_context(GVoxContext *ctx);

GVoxInputAdapter *gvox_register_input_adapter(GVoxContext *ctx, GVoxInputAdapterInfo const *adapter_info);
GVoxInputAdapter *gvox_get_input_adapter(GVoxContext *ctx, char const *adapter_name);

GVoxOutputAdapter *gvox_register_output_adapter(GVoxContext *ctx, GVoxOutputAdapterInfo const *adapter_info);
GVoxOutputAdapter *gvox_get_output_adapter(GVoxContext *ctx, char const *adapter_name);

GVoxFormatAdapter *gvox_register_format_adapter(GVoxContext *ctx, GVoxFormatAdapterInfo const *adapter_info);
GVoxFormatAdapter *gvox_get_format_adapter(GVoxContext *ctx, char const *adapter_name);

GVoxVoxel gvox_region_sample_voxel(GVoxRegion const *region, GVoxExtent3D const *position);
void gvox_serialize_write(GVoxSerializeState *state, size_t position, size_t size, void const *data);
void gvox_serialize_reserve(GVoxSerializeState *state, size_t size);

GVoxResult gvox_get_result(GVoxContext *ctx);
void gvox_get_result_message(GVoxContext *ctx, char *const str_buffer, size_t *str_size);
void gvox_pop_result(GVoxContext *ctx);

GVoxRegionList gvox_parse(
    GVoxInputAdapter *input_adapter,
    void const *input_config,
    GVoxFormatAdapter *format_adapter,
    void const *format_config);

void gvox_serialize(
    GVoxOutputAdapter *output_adapter,
    void const *output_config,
    GVoxFormatAdapter *format_adapter,
    void const *format_config,
    GVoxRegion const *region);

#ifdef __cplusplus
}
#endif

#endif
