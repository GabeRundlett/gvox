#ifndef GVOX_CONTAINER_OPENVDB_H
#define GVOX_CONTAINER_OPENVDB_H

#include <gvox/gvox.h>

GVOX_STRUCT(GvoxOpenvdbContainerConfig) {
    uint8_t _pad;
};

GVOX_FUNC(GvoxContainerDescription, gvox_container_openvdb_description, void);

#endif
