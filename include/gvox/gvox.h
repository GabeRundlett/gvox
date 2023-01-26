#ifndef GVOX_GVOX_H
#define GVOX_GVOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef enum {
    GVOX_SUCCESS = 0,
    GVOX_ERROR_UNKNOWN = -1,
} GVoxResult;

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

typedef struct {
    GVoxOffset3D offset;
    GVoxExtent3D extent;
} GVoxRegionRange;

typedef struct _GVoxContext GVoxContext;
typedef struct _GVoxInputAdapter GVoxInputAdapter;
typedef struct _GVoxOutputAdapter GVoxOutputAdapter;
typedef struct _GVoxParseAdapter GVoxParseAdapter;
typedef struct _GVoxSerializeAdapter GVoxSerializeAdapter;
typedef struct _GVoxAdapterContext GVoxAdapterContext;

GVoxContext *gvox_create_context(void);
void gvox_destroy_context(GVoxContext *ctx);

GVoxInputAdapter *gvox_get_input_adapter(GVoxContext *ctx, char const *adapter_name);
GVoxOutputAdapter *gvox_get_output_adapter(GVoxContext *ctx, char const *adapter_name);
GVoxParseAdapter *gvox_get_parse_adapter(GVoxContext *ctx, char const *adapter_name);
GVoxSerializeAdapter *gvox_get_serialize_adapter(GVoxContext *ctx, char const *adapter_name);

GVoxAdapterContext *gvox_create_adapter_context(
    GVoxInputAdapter *input_adapter, void *input_config,
    GVoxOutputAdapter *output_adapter, void *output_config,
    GVoxParseAdapter *parse_adapter, void *parse_config,
    GVoxSerializeAdapter *serialize_adapter, void *serialize_config);
void gvox_destroy_adapter_context(GVoxAdapterContext *ctx);
void gvox_translate_region(GVoxAdapterContext *ctx, GVoxRegionRange const *range);

uint32_t gvox_sample_data(GVoxAdapterContext *ctx, GVoxOffset3D const *offset);
uint32_t gvox_query_region_flags(GVoxAdapterContext *ctx, GVoxRegionRange const *range);

#ifdef __cplusplus
}
#endif

#endif
