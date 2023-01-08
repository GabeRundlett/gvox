#include <stdlib.h>
#include <math.h>

#include <gvox/gvox.h>

int main(void) {
    GVoxContext *gvox_ctx = gvox_create_context();

    GVoxParseContext *parse_ctx = gvox_create_parse_context(gvox_ctx, "gvox_raw", "gvox_optimal");

    uint8_t *data;
    gvox_parse_context_begin(parse_ctx, data);

    GVoxBounds region_bounds = gvox_parse_context_get_input_bounds(parse_ctx);
    GVoxSerialRegion *serial_region = gvox_parse_context_serialize(parse_ctx, region_bounds);
    GVoxOffset3D sample_pos = {0, 0, 0};
    GVoxVoxel voxel = gvox_serial_region_sample(serial_region, sample_pos);

    gvox_parse_context_end(parse_ctx);

    gvox_destroy_parse_context(gvox_ctx, parse_ctx);
    gvox_destroy_context(gvox_ctx);
}
