#include <stdlib.h>
#include <math.h>

#include "print.h"
#include "scene.h"

int main(void) {
    GVoxContext *gvox = gvox_create_context();

    GVoxScene scene = create_scene(8, 8, 8);
    printf("\n raw\n");
    print_voxels(scene);

    gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_simple.gvox", "gvox_simple");
    gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
    gvox_save(gvox, scene, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_u32_palette");
    gvox_save(gvox, scene, "tests/simple/compare_scene0_magicavoxel.gvox", "magicavoxel");
    gvox_destroy_scene(scene);

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
