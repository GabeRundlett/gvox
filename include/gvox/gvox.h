#ifndef GVOX_GVOX_H
#define GVOX_GVOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <gvox/core.h>
#include <gvox/format.h>
#include <gvox/adapter.h>

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

GVOX_STRUCT(GvoxOffset) {
    size_t count;
    int64_t *const data;
};
GVOX_STRUCT(GvoxExtent) {
    size_t count;
    uint64_t *const data;
};

GVOX_STRUCT(GvoxRange) {
    GvoxOffset offset;
    GvoxExtent extent;
};

GVOX_STRUCT(GvoxFillInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainer dst;
    GvoxRange range;
    void *data;
    GvoxVoxelDesc data_voxel_desc;
};

GVOX_STRUCT(GvoxBlitInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxParser src;
    GvoxSerializer dst;
};

GVOX_STRUCT(GvoxSerializeInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainer src;
    GvoxInputAdapter dst;
};

// Consumer API

GvoxResult GVOX_EXPORT gvox_blit(GvoxBlitInfo const *info) GVOX_FUNC_ATTRIB;
GvoxResult GVOX_EXPORT gvox_fill(GvoxFillInfo const *info) GVOX_FUNC_ATTRIB;
void GVOX_EXPORT gvox_init(void) GVOX_FUNC_ATTRIB;

#ifdef __cplusplus
}
#endif

#endif
