#include <stdio.h>
#include <stdlib.h>

#include <gvox/gvox.h>

void print_voxels(GVoxScene scene) {
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (!scene.nodes[node_i].voxels)
            continue;
        size_t const voxel_n = scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z;
        float const inv_voxel_n = 1.0f / (float)voxel_n;
        for (size_t zi = 0; zi < scene.nodes[node_i].size_z; ++zi) {
            for (size_t yi = 0; yi < scene.nodes[node_i].size_y; ++yi) {
                for (size_t xi = 0; xi < scene.nodes[node_i].size_x; ++xi) {
                    size_t i = xi + yi * scene.nodes[node_i].size_x + zi * scene.nodes[node_i].size_x * scene.nodes[node_i].size_y;
                    GVoxVoxel vox = scene.nodes[node_i].voxels[i];
                    float brightness = inv_voxel_n * (float)vox.id;
                    if (brightness < 0.0f)
                        brightness = 0.0f;
                    if (brightness > 1.0f)
                        brightness = 1.0f;
                    char c = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"[(size_t)(brightness * 69.0f)];
                    fputc(c, stdout);
                }
                fputc('\n', stdout);
            }
            fputc('\n', stdout);
        }
    }
}

void test_manually_create_gvox_simple(GVoxContext *gvox) {
    GVoxScene scene;
    scene.node_n = 1;
    scene.nodes = malloc(sizeof(GVoxSceneNode) * scene.node_n);
    scene.nodes[0].size_x = 5;
    scene.nodes[0].size_y = 4;
    scene.nodes[0].size_z = 3;
    size_t const voxel_n = scene.nodes[0].size_x * scene.nodes[0].size_y * scene.nodes[0].size_z;
    scene.nodes[0].voxels = malloc(sizeof(GVoxVoxel) * voxel_n);
    for (size_t i = 0; i < voxel_n; ++i) {
        scene.nodes[0].voxels[i].color.x = (float)(rand() % 1000) * 0.001f;
        scene.nodes[0].voxels[i].color.y = (float)(rand() % 1000) * 0.001f;
        scene.nodes[0].voxels[i].color.z = (float)(rand() % 1000) * 0.001f;
        scene.nodes[0].voxels[i].id = (uint32_t)i;
    }
    print_voxels(scene);
    gvox_save(gvox, scene, "tests/simple/voxel_files/test.gvox", "gvox_simple");
    gvox_destroy_scene(scene);
}

void test_load_gvox_simple(GVoxContext *gvox) {
    GVoxScene scene;
    scene = gvox_load(gvox, "tests/simple/voxel_files/test.gvox");
    print_voxels(scene);
    gvox_destroy_scene(scene);
}

void test_load_magicavoxel(GVoxContext *gvox) {
    GVoxScene scene;
    scene = gvox_load_raw(gvox, "tests/simple/voxel_files/magica.vox", "magicavoxel");
    print_voxels(scene);
    gvox_destroy_scene(scene);
}

int main(void) {
    GVoxContext *gvox = gvox_create_context();

    // test_manually_create_gvox_simple(gvox);
    // test_load_gvox_simple(gvox);
    test_load_magicavoxel(gvox);

    gvox_destroy_context(gvox);
}
