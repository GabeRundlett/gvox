#ifndef GVOX_CONTAINER_BOUNDED_RAW_H
#define GVOX_CONTAINER_BOUNDED_RAW_H

#include <gvox/format.h>

typedef struct {
    GvoxVoxelDesc voxel_desc;
    GvoxExtent extent;
    void *pre_allocated_buffer;
} GvoxBoundedRawContainerConfig;

#endif
