#ifndef GVOX_CONTAINER_RAW_H
#define GVOX_CONTAINER_RAW_H

#include <gvox/stream.h>
#include <gvox/format.h>

typedef struct {
    GvoxVoxelDesc voxel_desc;
} GvoxRaw3dContainerConfig;

GVOX_EXPORT GvoxContainerDescription gvox_container_raw3d_description(void) GVOX_FUNC_ATTRIB;

#endif
