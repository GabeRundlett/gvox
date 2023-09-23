#ifndef GVOX_CONTAINER_BOUNDED_RAW_H
#define GVOX_CONTAINER_BOUNDED_RAW_H

#include <gvox/stream.h>
#include <gvox/format.h>

typedef struct {
    GvoxVoxelDesc voxel_desc;
    GvoxExtent extent;
    void *pre_allocated_buffer;
} GvoxBoundedRawContainerConfig;

GVOX_EXPORT GvoxContainerDescription gvox_container_bounded_raw_description(void) GVOX_FUNC_ATTRIB;

#endif
