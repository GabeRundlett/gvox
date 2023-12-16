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

GVOX_STRUCT(GvoxOffset1D) { int64_t x; };
GVOX_STRUCT(GvoxExtent1D) { uint64_t x; };
GVOX_STRUCT(GvoxOffset2D) { int64_t x, y; };
GVOX_STRUCT(GvoxExtent2D) { uint64_t x, y; };
GVOX_STRUCT(GvoxOffset3D) { int64_t x, y, z; };
GVOX_STRUCT(GvoxExtent3D) { uint64_t x, y, z; };
GVOX_STRUCT(GvoxOffset4D) { int64_t x, y, z, w; };
GVOX_STRUCT(GvoxExtent4D) { uint64_t x, y, z, w; };
GVOX_STRUCT(GvoxOffset5D) { int64_t x, y, z, w, v; };
GVOX_STRUCT(GvoxExtent5D) { uint64_t x, y, z, w, v; };
GVOX_STRUCT(GvoxOffset6D) { int64_t x, y, z, w, v, u; };
GVOX_STRUCT(GvoxExtent6D) { uint64_t x, y, z, w, v, u; };

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
