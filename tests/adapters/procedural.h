#ifndef GVOX_PROCEDURAL_PARSE_ADAPTER_H
#define GVOX_PROCEDURAL_PARSE_ADAPTER_H

#include <gvox/gvox.h>

#ifdef __cplusplus
extern "C" {
#endif

void procedural_create(GvoxAdapterContext *ctx, void const *config);
void procedural_destroy(GvoxAdapterContext *ctx);
void procedural_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags);
void procedural_blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx);

GvoxParseAdapterDetails procedural_query_details(void);
uint32_t procedural_query_region_flags(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags);

GvoxRegion procedural_load_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags);
void procedural_unload_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion *region);
GvoxRegionRange procedural_query_parsable_range(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx);
uint32_t procedural_sample_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region, GvoxOffset3D const *offset, uint32_t channel_id);

void procedural_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags);

// This adapter has no config

#ifdef __cplusplus
}
#endif

#endif
