#ifndef GVOX_GVOX_H
#define GVOX_GVOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

// Consumer API

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
} GvoxResult;

#define GVOX_CHANNEL_ID_COLOR 0
#define GVOX_CHANNEL_ID_NORMAL 1
#define GVOX_CHANNEL_ID_MATERIAL_ID 2
#define GVOX_CHANNEL_ID_ROUGHNESS 3
#define GVOX_CHANNEL_ID_METALNESS 4
#define GVOX_CHANNEL_ID_TRANSPARENCY 5
#define GVOX_CHANNEL_ID_IOR 6
#define GVOX_CHANNEL_ID_EMISSIVITY 7
#define GVOX_CHANNEL_ID_HARDNESS 8
#define GVOX_CHANNEL_ID_LAST_STANDARD 23
#define GVOX_CHANNEL_ID_LAST 31

#define GVOX_CHANNEL_BIT_COLOR (1 << GVOX_CHANNEL_ID_COLOR)
#define GVOX_CHANNEL_BIT_NORMAL (1 << GVOX_CHANNEL_ID_NORMAL)
#define GVOX_CHANNEL_BIT_MATERIAL_ID (1 << GVOX_CHANNEL_ID_MATERIAL_ID)
#define GVOX_CHANNEL_BIT_ROUGHNESS (1 << GVOX_CHANNEL_ID_ROUGHNESS)
#define GVOX_CHANNEL_BIT_METALNESS (1 << GVOX_CHANNEL_ID_METALNESS)
#define GVOX_CHANNEL_BIT_TRANSPARENCY (1 << GVOX_CHANNEL_ID_TRANSPARENCY)
#define GVOX_CHANNEL_BIT_IOR (1 << GVOX_CHANNEL_ID_IOR)
#define GVOX_CHANNEL_BIT_EMISSIVITY (1 << GVOX_CHANNEL_ID_EMISSIVITY)
#define GVOX_CHANNEL_BIT_HARDNESS (1 << GVOX_CHANNEL_ID_HARDNESS)
#define GVOX_CHANNEL_BIT_LAST_STANDARD (1 << GVOX_CHANNEL_ID_LAST_STANDARD)
#define GVOX_CHANNEL_BIT_LAST (1 << GVOX_CHANNEL_ID_LAST)

#define GVOX_REGION_FLAG_UNIFORM 0x00000001

typedef struct _GvoxContext GvoxContext;
typedef struct _GvoxAdapter GvoxAdapter;
typedef struct _GvoxAdapterContext GvoxAdapterContext;
typedef struct _GvoxBlitContext GvoxBlitContext;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} GvoxOffset3D;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
} GvoxExtent3D;

typedef struct {
    GvoxOffset3D offset;
    GvoxExtent3D extent;
} GvoxRegionRange;

typedef struct {
    GvoxRegionRange range;
    uint32_t channels;
    uint32_t flags;
    void *data;
} GvoxRegion;

typedef struct {
    char const *name_str;
    void (*create)(GvoxAdapterContext *ctx, void *config);
    void (*destroy)(GvoxAdapterContext *ctx);
    void (*blit_begin)(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx);
    void (*blit_end)(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx);
} GvoxAdapterBaseInfo;

typedef struct {
    GvoxAdapterBaseInfo base_info;
    void (*read)(GvoxAdapterContext *ctx, size_t position, size_t size, void *data);
} GvoxInputAdapterInfo;

typedef struct {
    GvoxAdapterBaseInfo base_info;
    void (*write)(GvoxAdapterContext *ctx, size_t position, size_t size, void const *data);
    void (*reserve)(GvoxAdapterContext *ctx, size_t size);
} GvoxOutputAdapterInfo;

typedef struct {
    GvoxAdapterBaseInfo base_info;
    uint32_t (*query_region_flags)(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags);
    GvoxRegion (*load_region)(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags);
    void (*unload_region)(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion *region);
    uint32_t (*sample_region)(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region, GvoxOffset3D const *offset, uint32_t channel_id);
} GvoxParseAdapterInfo;

typedef struct {
    GvoxAdapterBaseInfo base_info;
    void (*serialize_region)(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags);
} GvoxSerializeAdapterInfo;

GvoxContext *gvox_create_context(void);
void gvox_destroy_context(GvoxContext *ctx);

GvoxResult gvox_get_result(GvoxContext *ctx);
void gvox_get_result_message(GvoxContext *ctx, char *const str_buffer, size_t *str_size);
void gvox_pop_result(GvoxContext *ctx);

GvoxAdapter *gvox_register_input_adapter(GvoxContext *ctx, GvoxInputAdapterInfo const *adapter_info);
GvoxAdapter *gvox_get_input_adapter(GvoxContext *ctx, char const *adapter_name);

GvoxAdapter *gvox_register_output_adapter(GvoxContext *ctx, GvoxOutputAdapterInfo const *adapter_info);
GvoxAdapter *gvox_get_output_adapter(GvoxContext *ctx, char const *adapter_name);

GvoxAdapter *gvox_register_parse_adapter(GvoxContext *ctx, GvoxParseAdapterInfo const *adapter_info);
GvoxAdapter *gvox_get_parse_adapter(GvoxContext *ctx, char const *adapter_name);

GvoxAdapter *gvox_register_serialize_adapter(GvoxContext *ctx, GvoxSerializeAdapterInfo const *adapter_info);
GvoxAdapter *gvox_get_serialize_adapter(GvoxContext *ctx, char const *adapter_name);

GvoxAdapterContext *gvox_create_adapter_context(GvoxContext *gvox_ctx, GvoxAdapter *adapter, void *config);
void gvox_destroy_adapter_context(GvoxAdapterContext *ctx);

void gvox_blit_region(
    GvoxAdapterContext *input_ctx, GvoxAdapterContext *output_ctx,
    GvoxAdapterContext *parse_ctx, GvoxAdapterContext *serialize_ctx,
    GvoxRegionRange const *range, uint32_t channel_flags);

// Adapter API

uint32_t gvox_query_region_flags(GvoxBlitContext *blit_ctx, GvoxRegionRange const *range, uint32_t channel_flags);
GvoxRegion gvox_load_region_range(GvoxBlitContext *blit_ctx, GvoxRegionRange const *range, uint32_t channel_flags);
void gvox_unload_region_range(GvoxBlitContext *blit_ctx, GvoxRegion *region, GvoxRegionRange const *range);
uint32_t gvox_sample_region(GvoxBlitContext *blit_ctx, GvoxRegion *region, GvoxOffset3D const *offset, uint32_t channel_id);

void gvox_adapter_push_error(GvoxAdapterContext *ctx, GvoxResult result_code, char const *message);
void gvox_adapter_set_user_pointer(GvoxAdapterContext *ctx, void *ptr);
void *gvox_adapter_get_user_pointer(GvoxAdapterContext *ctx);

void gvox_input_read(GvoxBlitContext *blit_ctx, size_t position, size_t size, void *data);
void gvox_output_write(GvoxBlitContext *blit_ctx, size_t position, size_t size, void const *data);
void gvox_output_reserve(GvoxBlitContext *blit_ctx, size_t size);

#ifdef __cplusplus
}
#endif

#endif
