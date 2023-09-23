#ifndef GVOX_CONTAINER_RAW_H
#define GVOX_CONTAINER_RAW_H

#include <gvox/stream.h>
#include <gvox/format.h>

typedef struct {
    GvoxVoxelDesc voxel_desc;
} GvoxRawContainerConfig;

GVOX_EXPORT GvoxContainerDescription gvox_container_raw_description(void) GVOX_FUNC_ATTRIB;

#endif
