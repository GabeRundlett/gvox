#ifndef GVOX_GVOX_H
#define GVOX_GVOX_H

#include <gvox/core.h>
#include <gvox/format.h>
#include <gvox/stream.h>

GVOX_STRUCT(GvoxVolume) {
    uint32_t n_dimensions;
    void *data;
    uint32_t *extent;
    float *transform;
};

#define _GVOX_DECL_OFFSET_EXTENT_ND(N)                  \
    GVOX_STRUCT(GvoxOffset##N##D) { int64_t data[N]; }; \
    GVOX_STRUCT(GvoxExtent##N##D) { uint64_t data[N]; };

_GVOX_DECL_OFFSET_EXTENT_ND(1)
_GVOX_DECL_OFFSET_EXTENT_ND(2)
_GVOX_DECL_OFFSET_EXTENT_ND(3)
_GVOX_DECL_OFFSET_EXTENT_ND(4)
_GVOX_DECL_OFFSET_EXTENT_ND(5)
_GVOX_DECL_OFFSET_EXTENT_ND(6)

#undef _GVOX_DECL_OFFSET_EXTENT_ND

GVOX_STRUCT(GvoxFillInfo) {
    GvoxStructType struct_type;
    void const *next;
    void const *src_data;
    GvoxVoxelDesc src_desc;
    GvoxContainer dst;
    GvoxRange range;
};

GVOX_STRUCT(GvoxMoveInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainer *src_containers;
    GvoxRange *src_container_ranges;
    GvoxOffset *src_dst_offsets;
    uint32_t src_container_n;
    GvoxContainer dst;
};

GVOX_STRUCT(GvoxBlitInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxParser src;
    GvoxSerializer dst;
};

GVOX_STRUCT(GvoxSampleInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainer src;
    GvoxSample const *samples;
    uint32_t sample_n;
};

// Consumer API

GVOX_EXPORT GvoxResult gvox_fill(GvoxFillInfo const *info) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_move(GvoxMoveInfo const *info) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_blit(GvoxBlitInfo const *info) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_sample(GvoxSampleInfo const *info) GVOX_FUNC_ATTRIB;

#endif
