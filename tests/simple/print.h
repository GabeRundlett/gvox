#ifndef TESTS_SIMPLE_PRINT_H
#define TESTS_SIMPLE_PRINT_H

#include <stdio.h>
#include <gvox/gvox.h>

#define AVG_REGION 1

void print_voxels(GVoxRegionList const *regions) {
    for (size_t region_i = 0; region_i < regions->region_n; ++region_i) {
        GVoxRegion *region = &(regions->regions[region_i]);
        for (uint32_t zi = 0; zi < region->metadata.extent.z; zi += AVG_REGION) {
            for (uint32_t yi = 0; yi < region->metadata.extent.y; yi += AVG_REGION) {
                for (uint32_t xi = 0; xi < region->metadata.extent.x; xi += AVG_REGION) {
                    GVoxExtent3D const pos = {xi, yi, zi};
                    GVoxVoxel voxel = gvox_region_sample_voxel(region, &pos);
                    uint8_t r = (voxel >> 0x00) & 0xff;
                    uint8_t g = (voxel >> 0x08) & 0xff;
                    uint8_t b = (voxel >> 0x10) & 0xff;
                    printf("\033[38;2;%03d;%03d;%03dm", (uint32_t)(r), (uint32_t)(g), (uint32_t)(b));
                    printf("\033[48;2;%03d;%03d;%03dm", (uint32_t)(r), (uint32_t)(g), (uint32_t)(b));
                    char c = '_';
                    fputc(c, stdout);
                    fputc(c, stdout);
                }
                printf("\033[0m ");
            }
            printf("\033[0m\n");
        }
    }
}

#endif
