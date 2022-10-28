#include <stdlib.h>
#include <math.h>

#include "print.h"

int main(void) {
    GVoxContext *gvox = gvox_create_context();

    GVoxScene scene;
    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_simple.gvox");
    printf("\n gvox_simple\n");
    print_voxels(scene);
    gvox_destroy_scene(scene);

    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32.gvox");
    printf("\n gvox_u32\n");
    print_voxels(scene);
    gvox_destroy_scene(scene);

    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
    printf("\n gvox_u32_palette\n");
    print_voxels(scene);
    gvox_destroy_scene(scene);

    scene = gvox_load_raw(gvox, "tests/simple/compare_scene0_magicavoxel.vox", "magicavoxel");
    printf("\n magicavoxel\n");
    print_voxels(scene);
    gvox_destroy_scene(scene);

    gvox_destroy_context(gvox);
}
