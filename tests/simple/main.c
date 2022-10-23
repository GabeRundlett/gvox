#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <gvox/gvox.h>

void print_voxels(GVoxScene scene) {
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (!scene.nodes[node_i].voxels)
            continue;

        for (size_t zi = 0; zi < scene.nodes[node_i].size_z; ++zi) {
            for (size_t yi = 0; yi < scene.nodes[node_i].size_y; ++yi) {
                for (size_t xi = 0; xi < scene.nodes[node_i].size_x; ++xi) {
                    size_t i = xi + (scene.nodes[node_i].size_y - 1 - yi) * scene.nodes[node_i].size_x + (scene.nodes[node_i].size_z - 1 - zi) * scene.nodes[node_i].size_x * scene.nodes[node_i].size_y;
                    GVoxVoxel vox = scene.nodes[node_i].voxels[i];
                    float brightness = (vox.color.x + vox.color.y + vox.color.z) * 1.0f / 3.0f;
                    if (brightness < 0.0f)
                        brightness = 0.0f;
                    if (brightness > 1.0f)
                        brightness = 1.0f;
                    char c = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"[(size_t)(brightness * 69.0f)];
                    if (vox.id == 0)
                        c = '.';
                    fputc(c, stdout);
                }
                // fputc('\n', stdout);
                fputc(' ', stdout);
            }
            fputc('\n', stdout);
        }
    }
}

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
