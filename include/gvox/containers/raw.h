#ifndef GVOX_CONTAINER_RAW_H
#define GVOX_CONTAINER_RAW_H

#include <gvox/stream.h>
#include <gvox/format.h>

typedef struct {
    GvoxVoxelDesc voxel_desc;
} GvoxRawContainerConfig;

GVOX_EXPORT GvoxContainerDescription gvox_container_raw_description(void) GVOX_FUNC_ATTRIB;

typedef struct {
    GvoxVoxelDesc voxel_desc;
} GvoxRaw3dContainerConfig;

GVOX_EXPORT GvoxContainerDescription gvox_container_raw3d_description(void) GVOX_FUNC_ATTRIB;

typedef struct {
    GvoxVoxelDesc voxel_desc;
    GvoxExtent extent;
    void *pre_allocated_buffer;
} GvoxBoundedRawContainerConfig;

GVOX_EXPORT GvoxContainerDescription gvox_container_bounded_raw_description(void) GVOX_FUNC_ATTRIB;

typedef struct {
    GvoxVoxelDesc voxel_desc;
    GvoxExtent3D extent;
    void *pre_allocated_buffer;
} GvoxBoundedRaw3dContainerConfig;

GVOX_EXPORT GvoxContainerDescription gvox_container_bounded_raw3d_description(void) GVOX_FUNC_ATTRIB;

#endif
