#ifndef GVOX_GVOX_H
#define GVOX_GVOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef enum {
    GVOX_RESULT_SUCCESS = 0,
    GVOX_RESULT_ERROR_UNKNOWN = -1,
    GVOX_RESULT_ERROR_INVALID_PARAMETER = -2,

    GVOX_RESULT_ERROR_INPUT_ADAPTER = -3,
    GVOX_RESULT_ERROR_OUTPUT_ADAPTER = -4,
    GVOX_RESULT_ERROR_PARSE_ADAPTER = -5,
    GVOX_RESULT_ERROR_SERIALIZE_ADAPTER = -6,

    GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT = -7,
    GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT = -8,
    GVOX_RESULT_ERROR_SERIALIZE_ADAPTER_UNREPRESENTABLE_DATA = -9,
} GVoxResult;

#define GVOX_CHANNEL_INDEX_COLOR 0
#define GVOX_CHANNEL_INDEX_NORMAL 1
#define GVOX_CHANNEL_INDEX_MATERIAL_ID 2
#define GVOX_CHANNEL_INDEX_ROUGHNESS 3
#define GVOX_CHANNEL_INDEX_METALNESS 4
#define GVOX_CHANNEL_INDEX_OPACITY 5
#define GVOX_CHANNEL_INDEX_EMISSIVITY 6
#define GVOX_CHANNEL_INDEX_HARDNESS 7
#define GVOX_CHANNEL_INDEX_LAST_STANDARD 15

#define GVOX_CHANNEL_BIT_COLOR (1 << GVOX_CHANNEL_INDEX_COLOR)
#define GVOX_CHANNEL_BIT_NORMAL (1 << GVOX_CHANNEL_INDEX_NORMAL)
#define GVOX_CHANNEL_BIT_MATERIAL_ID (1 << GVOX_CHANNEL_INDEX_MATERIAL_ID)
#define GVOX_CHANNEL_BIT_ROUGHNESS (1 << GVOX_CHANNEL_INDEX_ROUGHNESS)
#define GVOX_CHANNEL_BIT_METALNESS (1 << GVOX_CHANNEL_INDEX_METALNESS)
#define GVOX_CHANNEL_BIT_OPACITY (1 << GVOX_CHANNEL_INDEX_OPACITY)
#define GVOX_CHANNEL_BIT_EMISSIVITY (1 << GVOX_CHANNEL_INDEX_EMISSIVITY)
#define GVOX_CHANNEL_BIT_HARDNESS (1 << GVOX_CHANNEL_INDEX_HARDNESS)
#define GVOX_CHANNEL_BIT_LAST_STANDARD (1 << GVOX_CHANNEL_INDEX_LAST_STANDARD)

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

GVoxResult gvox_get_result(GVoxContext *ctx);
void gvox_get_result_message(GVoxContext *ctx, char *const str_buffer, size_t *str_size);
void gvox_pop_result(GVoxContext *ctx);

GVoxInputAdapter *gvox_get_input_adapter(GVoxContext *ctx, char const *adapter_name);
GVoxOutputAdapter *gvox_get_output_adapter(GVoxContext *ctx, char const *adapter_name);
GVoxParseAdapter *gvox_get_parse_adapter(GVoxContext *ctx, char const *adapter_name);
GVoxSerializeAdapter *gvox_get_serialize_adapter(GVoxContext *ctx, char const *adapter_name);

GVoxAdapterContext *gvox_create_adapter_context(
    GVoxContext *gvox_ctx,
    GVoxInputAdapter *input_adapter, void *input_config,
    GVoxOutputAdapter *output_adapter, void *output_config,
    GVoxParseAdapter *parse_adapter, void *parse_config,
    GVoxSerializeAdapter *serialize_adapter, void *serialize_config);
void gvox_destroy_adapter_context(GVoxAdapterContext *ctx);
void gvox_translate_region(GVoxAdapterContext *ctx, GVoxRegionRange const *range);

uint32_t gvox_sample_data(GVoxAdapterContext *ctx, GVoxOffset3D const *offset, uint32_t channel_index);
uint32_t gvox_query_region_flags(GVoxAdapterContext *ctx, GVoxRegionRange const *range, uint32_t channel_index);

#ifdef __cplusplus
}
#endif

#endif
