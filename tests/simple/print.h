#ifndef TESTS_SIMPLE_PRINT_H
#define TESTS_SIMPLE_PRINT_H

#include <stdio.h>
#include <gvox/gvox.h>

#define PRINT_MODE 0

void print_voxels(GVoxScene scene) {
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (!scene.nodes[node_i].voxels)
            continue;

        for (size_t zi = 0; zi < scene.nodes[node_i].size_z; ++zi) {
            for (size_t yi = 0; yi < scene.nodes[node_i].size_y; ++yi) {
                for (size_t xi = 0; xi < scene.nodes[node_i].size_x; ++xi) {
                    size_t i = xi + (scene.nodes[node_i].size_y - 1 - yi) * scene.nodes[node_i].size_x + (scene.nodes[node_i].size_z - 1 - zi) * scene.nodes[node_i].size_x * scene.nodes[node_i].size_y;
                    GVoxVoxel vox = scene.nodes[node_i].voxels[i];
#if PRINT_MODE == 0
                    float brightness = (vox.color.x + vox.color.y + vox.color.z) * 1.0f / 3.0f;
                    if (brightness < 0.0f)
                        brightness = 0.0f;
                    if (brightness > 1.0f)
                        brightness = 1.0f;
                    char c = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"[(size_t)(brightness * 69.0f)];
                    if (vox.id == 0)
                        c = '.';
#elif PRINT_MODE == 1
                    char c = '*';
                    uint8_t r = (uint8_t)(vox.color.x * 255);
                    if (r == 0)
                        c = ' ';
                    if (r == 255)
                        c = '#';
#elif PRINT_MODE == 2
                    char c = " .'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"[(size_t)(vox.id)];
#elif PRINT_MODE == 3
                    printf("\033[48;2;%03d;%03d;%03dm", (uint32_t)(vox.color.x * 255), (uint32_t)(vox.color.y * 255), (uint32_t)(vox.color.z * 255));
                    char c = '_';
#endif
                    fputc(c, stdout);
                }
                printf("\033[0m ");
            }
            printf("\033[0m\n");
        }
    }
}

#endif
