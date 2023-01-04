#ifndef TESTS_SIMPLE_PRINT_H
#define TESTS_SIMPLE_PRINT_H

#include <stdio.h>
#include <gvox/gvox.h>

#define PRINT_MODE 3

void print_voxels(GVoxScene scene) {
    const size_t AVG_REGION = 8;

    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (!scene.nodes[node_i].voxels)
            continue;

        for (size_t zi = 0; zi < scene.nodes[node_i].size_z; zi += AVG_REGION) {
            for (size_t yi = 0; yi < scene.nodes[node_i].size_y; yi += AVG_REGION) {
                for (size_t xi = 0; xi < scene.nodes[node_i].size_x; xi += AVG_REGION) {
                    size_t i = xi + yi * scene.nodes[node_i].size_x + (scene.nodes[node_i].size_z - 1 - zi) * scene.nodes[node_i].size_x * scene.nodes[node_i].size_y;
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

                    vox.color.x = 0.0f;
                    vox.color.y = 0.0f;
                    vox.color.z = 0.0f;
                    for (size_t a_zi = 0; a_zi < AVG_REGION; ++a_zi) {
                        for (size_t a_yi = 0; a_yi < AVG_REGION; ++a_yi) {
                            for (size_t a_xi = 0; a_xi < AVG_REGION; ++a_xi) {
                                size_t temp_i = (xi + a_xi) + (yi + a_yi) * scene.nodes[node_i].size_x + (scene.nodes[node_i].size_z - 1 - (zi + a_zi)) * scene.nodes[node_i].size_x * scene.nodes[node_i].size_y;
                                GVoxVoxel temp_vox = scene.nodes[node_i].voxels[temp_i];
                                vox.color.x += temp_vox.color.x;
                                vox.color.y += temp_vox.color.y;
                                vox.color.z += temp_vox.color.z;
                            }
                        }
                    }
                    float factor = 1.0f / (AVG_REGION * AVG_REGION * AVG_REGION);
                    vox.color.x *= factor;
                    vox.color.y *= factor;
                    vox.color.z *= factor;

                    printf("\033[38;2;%03d;%03d;%03dm", (uint32_t)(vox.color.x * 255), (uint32_t)(vox.color.y * 255), (uint32_t)(vox.color.z * 255));
                    printf("\033[48;2;%03d;%03d;%03dm", (uint32_t)(vox.color.x * 255), (uint32_t)(vox.color.y * 255), (uint32_t)(vox.color.z * 255));
                    char c = '_';
#endif
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
