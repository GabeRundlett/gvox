#ifndef GVOX_PROCEDURAL_PARSE_ADAPTER_H
#define GVOX_PROCEDURAL_PARSE_ADAPTER_H

#include <gvox/gvox.h>

#ifdef __cplusplus
extern "C" {
#endif

void gvox_parse_adapter_procedural_begin(GvoxAdapterContext *ctx, void *config);
void gvox_parse_adapter_procedural_end(GvoxAdapterContext *ctx);
uint32_t gvox_parse_adapter_procedural_query_region_flags(GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_id);
GvoxRegion gvox_parse_adapter_procedural_load_region(GvoxAdapterContext *ctx, GvoxOffset3D const *offset, uint32_t channel_id);
void gvox_parse_adapter_procedural_unload_region(GvoxAdapterContext *ctx, GvoxRegion *region);
uint32_t gvox_parse_adapter_procedural_sample_region(GvoxAdapterContext *ctx, GvoxRegion const *region, GvoxOffset3D const *offset, uint32_t channel_id);

// This adapter has no config

#ifdef __cplusplus
}
#endif

#endif
