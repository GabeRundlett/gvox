#ifndef GVOX_FORMATS_GVOX_RAW
#define GVOX_FORMATS_GVOX_RAW

#include <gvox/gvox.h>

GVoxPayload gvox_simple_create_payload(GVoxScene scene);
void gvox_simple_destroy_payload(GVoxPayload payload);
GVoxScene gvox_simple_parse_payload(GVoxPayload payload);

#endif
