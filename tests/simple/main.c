#include <stdlib.h>
#include <math.h>

#include "print.h"
#include "scene.h"

#define DO_LOAD 0
#define PRINT_RESULTS 0

int main(void) {
    GVoxContext *gvox = gvox_create_context();

    GVoxScene scene = create_scene(256, 256, 256);
#if PRINT_RESULTS
    printf("\n raw\n");
    print_voxels(scene);
#endif
    gvox_save(gvox, &scene, "tests/simple/compare_scene0_gvox_raw.gvox", "gvox_raw");
    gvox_save(gvox, &scene, "tests/simple/compare_scene0_gvox_u32.gvox", "gvox_u32");
    gvox_save(gvox, &scene, "tests/simple/compare_scene0_gvox_u32_palette.gvox", "gvox_u32_palette");
    gvox_save(gvox, &scene, "tests/simple/compare_scene0_zlib.gvox", "zlib");
    gvox_save(gvox, &scene, "tests/simple/compare_scene0_gzip.gvox", "gzip");
    gvox_save(gvox, &scene, "tests/simple/compare_scene0_magicavoxel.gvox", "magicavoxel");
    gvox_destroy_scene(&scene);

#if DO_LOAD
    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_raw.gvox");
#if PRINT_RESULTS
    printf("\n gvox_raw\n");
    print_voxels(scene);
#endif
    gvox_destroy_scene(scene);

    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32.gvox");
#if PRINT_RESULTS
    printf("\n gvox_u32\n");
    print_voxels(scene);
#endif
    gvox_destroy_scene(scene);

    scene = gvox_load(gvox, "tests/simple/compare_scene0_gvox_u32_palette.gvox");
#if PRINT_RESULTS
    printf("\n gvox_u32_palette\n");
    print_voxels(scene);
#endif
    gvox_destroy_scene(scene);

    scene = gvox_load(gvox, "tests/simple/compare_scene0_zlib.gvox");
#if PRINT_RESULTS
    printf("\n zlib\n");
    print_voxels(scene);
#endif
    gvox_destroy_scene(scene);

    scene = gvox_load(gvox, "tests/simple/compare_scene0_gzip.gvox");
#if PRINT_RESULTS
    printf("\n gzip\n");
    print_voxels(scene);
#endif
    gvox_destroy_scene(scene);

    scene = gvox_load_from_raw(gvox, "tests/simple/compare_scene0_magicavoxel.vox", "magicavoxel");
#if PRINT_RESULTS
    printf("\n magicavoxel\n");
    print_voxels(scene);
#endif
    gvox_destroy_scene(scene);
#endif

    gvox_destroy_context(gvox);
}
