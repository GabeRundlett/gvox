#ifndef GVOX_CONTAINER_RAW_H
#define GVOX_CONTAINER_RAW_H

#include <gvox/gvox.h>

GVOX_STRUCT(GvoxRawContainerConfig) {
    GvoxVoxelDesc voxel_desc;
};

GVOX_FUNC(GvoxContainerDescription, gvox_container_raw_description, void);

GVOX_STRUCT(GvoxRaw3dContainerConfig) {
    GvoxVoxelDesc voxel_desc;
};

GVOX_FUNC(GvoxContainerDescription, gvox_container_raw3d_description, void);

GVOX_STRUCT(GvoxBoundedRawContainerConfig) {
    GvoxVoxelDesc voxel_desc;
    GvoxExtent extent;
    void *pre_allocated_buffer;
};

GVOX_FUNC(GvoxContainerDescription, gvox_container_bounded_raw_description, void);

GVOX_STRUCT(GvoxBoundedRaw3dContainerConfig) {
    GvoxVoxelDesc voxel_desc;
    GvoxExtent3D extent;
    void *pre_allocated_buffer;
};

GVOX_FUNC(GvoxContainerDescription, gvox_container_bounded_raw3d_description, void);

#endif
