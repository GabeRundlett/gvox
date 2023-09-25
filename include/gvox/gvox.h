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
    void *src_data;
    GvoxVoxelDesc src_desc;
    GvoxContainer dst;
    GvoxRange range;
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
    GvoxOffset offset;
    void *dst;
    GvoxVoxelDesc dst_voxel_desc;
};

// Consumer API

GVOX_EXPORT GvoxResult gvox_fill(GvoxFillInfo const *info) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_blit(GvoxBlitInfo const *info) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_sample(GvoxSampleInfo const *info) GVOX_FUNC_ATTRIB;

GVOX_EXPORT GvoxResult gvox_translate_format(void const *src_data, GvoxFormat src_format, void *dst_data, GvoxFormat dst_format) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_translate_voxel(void const *src_data, GvoxVoxelDesc src_desc, void *dst_data, GvoxVoxelDesc dst_desc) GVOX_FUNC_ATTRIB;

#endif
