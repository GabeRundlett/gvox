#include <gvox/gvox.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <algorithm>
#include <unordered_set>

#include <iostream>

#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport) __stdcall
#endif

static constexpr auto CHUNK_SIZE = 8;

uint32_t ceil_log2(size_t x) {
    static const size_t t[6] = {
        0xFFFFFFFF00000000ull,
        0x00000000FFFF0000ull,
        0x000000000000FF00ull,
        0x00000000000000F0ull,
        0x000000000000000Cull,
        0x0000000000000002ull};

    uint32_t y = (((x & (x - 1)) == 0) ? 0 : 1);
    int j = 32;
    int i;

    for (i = 0; i < 6; i++) {
        int k = (((x & t[i]) == 0) ? 0 : j);
        y += k;
        x >>= k;
        j >>= 1;
    }

    return y;
}

struct PaletteCompressor {
    // std::vector<> palettes;

    size_t chunk_variance(GVoxSceneNode const &node, size_t chunk_x, size_t chunk_y, size_t chunk_z) {
        auto tile_set = std::unordered_set<uint32_t>{};
        size_t ox = chunk_x * CHUNK_SIZE;
        size_t oy = chunk_y * CHUNK_SIZE;
        size_t oz = chunk_z * CHUNK_SIZE;
        // std::cout << "o: " << ox << " " << oy << " " << oz << "\t";

        for (size_t zi = 0; zi < CHUNK_SIZE; ++zi) {
            for (size_t yi = 0; yi < CHUNK_SIZE; ++yi) {
                for (size_t xi = 0; xi < CHUNK_SIZE; ++xi) {
                    // TODO: fix for non-multiple sizes of CHUNK_SIZE
                    size_t px = ox + xi;
                    size_t py = oy + yi;
                    size_t pz = oz + zi;
                    size_t index = px + py * node.size_x + pz * node.size_x * node.size_y;
                    auto const &i_vox = node.voxels[index];

                    uint32_t u32_voxel = 0;
                    uint32_t r = static_cast<uint32_t>(std::max(std::min(i_vox.color.x, 1.0f), 0.0f) * 255.0f);
                    uint32_t g = static_cast<uint32_t>(std::max(std::min(i_vox.color.y, 1.0f), 0.0f) * 255.0f);
                    uint32_t b = static_cast<uint32_t>(std::max(std::min(i_vox.color.z, 1.0f), 0.0f) * 255.0f);
                    uint32_t i = i_vox.id;
                    u32_voxel = u32_voxel | (r << 0x00);
                    u32_voxel = u32_voxel | (g << 0x08);
                    u32_voxel = u32_voxel | (b << 0x10);
                    u32_voxel = u32_voxel | (i << 0x18);

                    tile_set.insert(u32_voxel);
                }
            }
        }

        size_t variants = tile_set.size();

        size_t size = 0;

        // info (contains whether there's a palette chunk)
        size += sizeof(uint32_t);

        // insert palette
        size += sizeof(uint32_t) * variants;

        if (variants != 1) {
            // insert palette chunk
            size_t bits_per_variant = ceil_log2(variants);
            size += (CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE * bits_per_variant) / 8;
        }

        return size;
    }

    size_t node_precomp(GVoxSceneNode const &node) {
        size_t size = 0;

        // for the size xyz
        size += sizeof(size_t) * 3;

        assert((node.size_x % CHUNK_SIZE) == 0);
        assert((node.size_y % CHUNK_SIZE) == 0);
        assert((node.size_z % CHUNK_SIZE) == 0);

        // these can be inferred and therefore don't need to be stored
        size_t chunk_nx = (node.size_x + CHUNK_SIZE - 1) / CHUNK_SIZE;
        size_t chunk_ny = (node.size_y + CHUNK_SIZE - 1) / CHUNK_SIZE;
        size_t chunk_nz = (node.size_z + CHUNK_SIZE - 1) / CHUNK_SIZE;

        // for the node's whole size
        size += sizeof(size_t) * 1;

        for (size_t zi = 0; zi < chunk_nz; ++zi) {
            for (size_t yi = 0; yi < chunk_ny; ++yi) {
                for (size_t xi = 0; xi < chunk_nx; ++xi) {
                    size += chunk_variance(node, xi, yi, zi);
                }
            }
        }

        std::cout << size << std::endl;

        return size;
    }

    GVoxPayload create(GVoxScene const &scene) {
        GVoxPayload result = {};
        result.size += sizeof(size_t);
        for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
            if (!scene.nodes[node_i].voxels)
                continue;
            result.size += node_precomp(scene.nodes[node_i]);
        }
        result.data = new uint8_t[result.size];
        return result;
    }
};

extern "C" {
GVoxPayload EXPORT gvox_create_payload(GVoxScene scene) {
    PaletteCompressor compressor;
    return compressor.create(scene);
}

void EXPORT gvox_destroy_payload(GVoxPayload payload) {
    delete[] payload.data;
}

GVoxScene EXPORT gvox_parse_payload(GVoxPayload payload) {
    GVoxScene result = {};
    // uint8_t *buffer_ptr = (uint8_t *)payload.data;
    // uint8_t *buffer_sentinel = (uint8_t *)payload.data + payload.size;
    // result.node_n = *(size_t *)buffer_ptr;
    // buffer_ptr += sizeof(result.node_n);
    // result.nodes = (GVoxSceneNode *)malloc(sizeof(GVoxSceneNode) * result.node_n);
    // size_t node_i = 0;
    // while (buffer_ptr < buffer_sentinel) {
    //     result.nodes[node_i].size_x = *(size_t *)buffer_ptr;
    //     buffer_ptr += sizeof(result.nodes[node_i].size_x);
    //     result.nodes[node_i].size_y = *(size_t *)buffer_ptr;
    //     buffer_ptr += sizeof(result.nodes[node_i].size_y);
    //     result.nodes[node_i].size_z = *(size_t *)buffer_ptr;
    //     buffer_ptr += sizeof(result.nodes[node_i].size_z);
    //     size_t voxels_n = result.nodes[node_i].size_x * result.nodes[node_i].size_y * result.nodes[node_i].size_z;
    //     size_t voxels_size = voxels_n * sizeof(GVoxVoxel);
    //     result.nodes[node_i].voxels = (GVoxVoxel *)malloc(voxels_size);
    //     for (size_t voxel_i = 0; voxel_i < voxels_n; ++voxel_i) {
    //         uint32_t u32_voxel = 0;
    //         memcpy(&u32_voxel, buffer_ptr + voxel_i * sizeof(uint32_t), sizeof(uint32_t));
    //         float r = ((u32_voxel >> 0x00) & 0xff) * 1.0f / 255.0f;
    //         float g = ((u32_voxel >> 0x08) & 0xff) * 1.0f / 255.0f;
    //         float b = ((u32_voxel >> 0x10) & 0xff) * 1.0f / 255.0f;
    //         uint32_t i = (u32_voxel >> 0x18) & 0xff;
    //         result.nodes[node_i].voxels[voxel_i] = GVoxVoxel{
    //             .color = {r, g, b},
    //             .id = i,
    //         };
    //     }
    //     buffer_ptr += voxels_size;
    //     ++node_i;
    // }
    return result;
}
}
