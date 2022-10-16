#ifndef GVOX_FORMATS_MAGICAVOXEL
#define GVOX_FORMATS_MAGICAVOXEL

#include <gvox/gvox.h>

GVoxPayload magicavoxel_create_payload(GVoxScene scene);
void magicavoxel_destroy_payload(GVoxPayload payload);
GVoxScene magicavoxel_parse_payload(GVoxPayload payload);

#endif
