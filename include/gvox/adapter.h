#ifndef GVOX_ADAPTER_H
#define GVOX_ADAPTER_H

#include <gvox/gvox.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GVOX_REGION_FLAG_UNIFORM 0x00000001

typedef struct {
    GVoxRegionRange range;
    uint32_t channels;
    uint32_t flags;
    void *data;
} GVoxRegion;

typedef struct {
    char const *name_str;
    void (*begin)(GVoxAdapterContext *ctx, void *config);
    void (*end)(GVoxAdapterContext *ctx);
    void (*read)(GVoxAdapterContext *ctx, size_t position, size_t size, void *data);
} GVoxInputAdapterInfo;

typedef struct {
    char const *name_str;
    void (*begin)(GVoxAdapterContext *ctx, void *config);
    void (*end)(GVoxAdapterContext *ctx);
    void (*write)(GVoxAdapterContext *ctx, size_t position, size_t size, void const *data);
    void (*reserve)(GVoxAdapterContext *ctx, size_t size);
} GVoxOutputAdapterInfo;

typedef struct {
    char const *name_str;
    void (*begin)(GVoxAdapterContext *ctx, void *config);
    void (*end)(GVoxAdapterContext *ctx);
    uint32_t (*query_region_flags)(GVoxAdapterContext *ctx, GVoxRegionRange const *range, uint32_t channel_index);
    void (*load_region)(GVoxAdapterContext *ctx, GVoxOffset3D const *offset, uint32_t channel_index);
    uint32_t (*sample_data)(GVoxAdapterContext *ctx, GVoxRegion const *region, GVoxOffset3D const *offset, uint32_t channel_index);
} GVoxParseAdapterInfo;

typedef struct {
    char const *name_str;
    void (*begin)(GVoxAdapterContext *ctx, void *config);
    void (*end)(GVoxAdapterContext *ctx);
    void (*serialize_region)(GVoxAdapterContext *ctx, GVoxRegionRange const *range);
} GVoxSerializeAdapterInfo;

GVoxInputAdapter *gvox_register_input_adapter(GVoxContext *ctx, GVoxInputAdapterInfo const *adapter_info);
GVoxOutputAdapter *gvox_register_output_adapter(GVoxContext *ctx, GVoxOutputAdapterInfo const *adapter_info);
GVoxParseAdapter *gvox_register_parse_adapter(GVoxContext *ctx, GVoxParseAdapterInfo const *adapter_info);
GVoxSerializeAdapter *gvox_register_serialize_adapter(GVoxContext *ctx, GVoxSerializeAdapterInfo const *adapter_info);

void gvox_adapter_push_error(GVoxAdapterContext *ctx, GVoxResult result_code, char const *message);
void *gvox_adapter_malloc(GVoxAdapterContext *ctx, size_t size);

void gvox_input_adapter_set_user_pointer(GVoxAdapterContext *ctx, void *ptr);
void gvox_output_adapter_set_user_pointer(GVoxAdapterContext *ctx, void *ptr);
void gvox_parse_adapter_set_user_pointer(GVoxAdapterContext *ctx, void *ptr);
void gvox_serialize_adapter_set_user_pointer(GVoxAdapterContext *ctx, void *ptr);

void *gvox_input_adapter_get_user_pointer(GVoxAdapterContext *ctx);
void *gvox_output_adapter_get_user_pointer(GVoxAdapterContext *ctx);
void *gvox_parse_adapter_get_user_pointer(GVoxAdapterContext *ctx);
void *gvox_serialize_adapter_get_user_pointer(GVoxAdapterContext *ctx);

// To be used by the parse adapter
void gvox_make_region_available(GVoxAdapterContext *ctx, GVoxRegion const *region);
void gvox_input_read(GVoxAdapterContext *ctx, size_t position, size_t size, void *data);

// To be used by the serialize adapter
void gvox_output_write(GVoxAdapterContext *ctx, size_t position, size_t size, void const *data);
void gvox_output_reserve(GVoxAdapterContext *ctx, size_t size);

#ifdef __cplusplus
}
#endif

#endif
