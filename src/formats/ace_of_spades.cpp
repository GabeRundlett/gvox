#include <gvox/gvox.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <iostream>
#include <vector>

#if GVOX_FORMAT_ACE_OF_SPADES_BUILT_STATIC
#define EXPORT
#else
#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport)
#endif
#endif

struct AceOfSpadesContext {
    AceOfSpadesContext();
    ~AceOfSpadesContext() = default;

    static auto create_payload(GVoxScene scene) -> GVoxPayload;
    static void destroy_payload(GVoxPayload payload);
    static auto parse_payload(GVoxPayload payload) -> GVoxScene;
};

AceOfSpadesContext::AceOfSpadesContext() = default;

auto AceOfSpadesContext::create_payload(GVoxScene scene) -> GVoxPayload {
    GVoxPayload result = {};
    assert(scene.node_n == 1);
    assert(scene.nodes[0].size_x == 512);
    assert(scene.nodes[0].size_y == 512);
    assert((scene.nodes[0].size_z % 64) == 0);
    auto data = std::vector<uint8_t>{};
    auto const MAP_Z = scene.nodes[0].size_z;
    auto sample_map = [&](size_t x, size_t y, size_t z) -> uint32_t {
        auto voxel_i = x + y * scene.nodes[0].size_x + z * scene.nodes[0].size_x * scene.nodes[0].size_y;
        return scene.nodes[0].voxels[voxel_i].id;
    };
    auto is_surface = [&](size_t x, size_t y, size_t z) -> bool {
        if (sample_map(x, y, z) == 0)
            return false;
        if (x > 0 && sample_map(x - 1, y, z) == 0)
            return true;
        if (x + 1 < scene.nodes[0].size_x && sample_map(x + 1, y, z) == 0)
            return true;
        if (y > 0 && sample_map(x, y - 1, z) == 0)
            return true;
        if (y + 1 < scene.nodes[0].size_y && sample_map(x, y + 1, z) == 0)
            return true;
        if (z > 0 && sample_map(x, y, z - 1) == 0)
            return true;
        if (z + 1 < scene.nodes[0].size_z && sample_map(x, y, z + 1) == 0)
            return true;
        return false;
    };
    auto write_color = [](std::vector<uint8_t> &data, uint32_t color) -> void {
        // assume color is ARGB native, but endianness is unknown

        // file format endianness is ARGB little endian, i.e. B,G,R,A
        data.push_back((uint8_t)(color >> 0));
        data.push_back((uint8_t)(color >> 8));
        data.push_back((uint8_t)(color >> 16));
        data.push_back((uint8_t)(color >> 24));
    };
    size_t i, j, k;
    for (j = 0; j < scene.nodes[0].size_x; ++j) {
        for (i = 0; i < scene.nodes[0].size_y; ++i) {
            k = 0;
            while (k < MAP_Z) {
                size_t z;
                size_t air_start;
                size_t top_colors_start;
                size_t top_colors_end; // exclusive
                size_t bottom_colors_start;
                size_t bottom_colors_end; // exclusive
                size_t top_colors_len;
                size_t bottom_colors_len;
                size_t colors;
                // find the air region
                air_start = k;
                while (k < MAP_Z && !sample_map(i, j, k))
                    ++k;
                // find the top region
                top_colors_start = k;
                while (k < MAP_Z && is_surface(i, j, k))
                    ++k;
                top_colors_end = k;
                // now skip past the solid voxels
                while (k < MAP_Z && sample_map(i, j, k) && !is_surface(i, j, k))
                    ++k;
                // at the end of the solid voxels, we have colored voxels.
                // in the "normal" case they're bottom colors; but it's
                // possible to have air-color-solid-color-solid-color-air,
                // which we encode as air-color-solid-0, 0-color-solid-air
                // so figure out if we have any bottom colors at this point
                bottom_colors_start = k;
                z = k;
                while (z < MAP_Z && is_surface(i, j, z))
                    ++z;
                if (z == MAP_Z || 0)
                    ; // in this case, the bottom colors of this span are empty, because we'l emit as top colors
                else {
                    // otherwise, these are real bottom colors so we can write them
                    while (is_surface(i, j, k))
                        ++k;
                }
                bottom_colors_end = k;
                // now we're ready to write a span
                top_colors_len = top_colors_end - top_colors_start;
                bottom_colors_len = bottom_colors_end - bottom_colors_start;
                colors = top_colors_len + bottom_colors_len;
                if (k == MAP_Z) {
                    data.push_back(0); // last span
                } else {
                    data.push_back(static_cast<uint8_t>(colors + 1));
                }
                data.push_back(static_cast<uint8_t>(top_colors_start));
                data.push_back(static_cast<uint8_t>(top_colors_end - 1));
                data.push_back(static_cast<uint8_t>(air_start));
                for (z = 0; z < top_colors_len; ++z)
                    write_color(data, sample_map(i, j, top_colors_start + z));
                for (z = 0; z < bottom_colors_len; ++z)
                    write_color(data, sample_map(i, j, bottom_colors_start + z));
            }
        }
    }
    result.data = new uint8_t[data.size()];
    result.size = data.size();
    std::copy(data.begin(), data.end(), result.data);
    return result;
}

void AceOfSpadesContext::destroy_payload(GVoxPayload payload) {
    delete[] payload.data;
}

auto AceOfSpadesContext::parse_payload(GVoxPayload payload) -> GVoxScene {
    GVoxScene result = {};

    result.node_n = 1;
    result.nodes = (GVoxSceneNode *)std::malloc(sizeof(GVoxSceneNode) * result.node_n);

    result.nodes[0].size_x = 512;
    result.nodes[0].size_y = 512;
    result.nodes[0].size_z = 256;
    size_t const voxels_n = result.nodes[0].size_x * result.nodes[0].size_y * result.nodes[0].size_z;
    size_t const voxels_size = voxels_n * sizeof(GVoxVoxel);
    result.nodes[0].voxels = (GVoxVoxel *)std::malloc(voxels_size);

    auto set_geom = [&result](size_t x, size_t y, size_t z, int t) {
        auto voxel_i = x + (result.nodes[0].size_y - 1 - y) * result.nodes[0].size_x + (result.nodes[0].size_z - 1 - z) * result.nodes[0].size_x * result.nodes[0].size_y;
        result.nodes[0].voxels[voxel_i].id = static_cast<uint32_t>(t);
    };

    auto set_color = [&result](size_t x, size_t y, size_t z, uint32_t u32_voxel) {
        float r = static_cast<float>((u32_voxel >> 0x10) & 0xff) / 255.0f;
        float g = static_cast<float>((u32_voxel >> 0x08) & 0xff) / 255.0f;
        float b = static_cast<float>((u32_voxel >> 0x00) & 0xff) / 255.0f;
        auto voxel_i = x + (result.nodes[0].size_y - 1 - y) * result.nodes[0].size_x + (result.nodes[0].size_z - 1 - z) * result.nodes[0].size_x * result.nodes[0].size_y;
        result.nodes[0].voxels[voxel_i].color = {r, g, b};
    };

    auto *v = payload.data;
    [[maybe_unused]] auto *base = v;

    size_t x, y, z;
    for (y = 0; y < result.nodes[0].size_y; ++y) {
        for (x = 0; x < result.nodes[0].size_x; ++x) {
            for (z = 0; z < result.nodes[0].size_z; ++z) {
                set_geom(x, y, z, 1);
            }
            z = 0;
            for (;;) {
                uint32_t *color;
                size_t number_4byte_chunks = v[0];
                size_t top_color_start = v[1], top_color_end = v[2];
                size_t bottom_color_start, bottom_color_end;
                size_t len_top, len_bottom;
                for (size_t i = z; i < top_color_start; i++)
                    set_geom(x, y, i, 0);
                color = (uint32_t *)(v + 4);
                for (z = top_color_start; z <= top_color_end; z++)
                    set_color(x, y, z, *color++);
                len_bottom = top_color_end - top_color_start + 1;
                // check for end of data marker
                if (number_4byte_chunks == 0) {
                    // infer ACTUAL number of 4-byte chunks from the length of the color data
                    v += 4 * (len_bottom + 1);
                    break;
                }
                // infer the number of bottom colors in next span from chunk length
                len_top = (number_4byte_chunks - 1) - len_bottom;
                // now skip the v pointer past the data to the beginning of the next span
                v += v[0] * 4;
                bottom_color_end = v[3]; // aka air start
                bottom_color_start = bottom_color_end - len_top;
                for (z = bottom_color_start; z < bottom_color_end; ++z) {
                    set_color(x, y, z, *color++);
                }
            }
        }
    }

    assert(v - base == payload.size);

    return result;
}

extern "C" EXPORT auto gvox_format_ace_of_spades_create_context() -> void * {
    auto *result = new AceOfSpadesContext{};
    return result;
}

extern "C" EXPORT void gvox_format_ace_of_spades_destroy_context(void *context_ptr) {
    auto *self = reinterpret_cast<AceOfSpadesContext *>(context_ptr);
    delete self;
}

extern "C" EXPORT auto gvox_format_ace_of_spades_create_payload(void *context_ptr, GVoxScene scene) -> GVoxPayload {
    auto *self = reinterpret_cast<AceOfSpadesContext *>(context_ptr);
    return self->create_payload(scene);
}

extern "C" EXPORT void gvox_format_ace_of_spades_destroy_payload(void *context_ptr, GVoxPayload payload) {
    auto *self = reinterpret_cast<AceOfSpadesContext *>(context_ptr);
    self->destroy_payload(payload);
}

extern "C" EXPORT auto gvox_format_ace_of_spades_parse_payload(void *context_ptr, GVoxPayload payload) -> GVoxScene {
    auto *self = reinterpret_cast<AceOfSpadesContext *>(context_ptr);
    return self->parse_payload(payload);
}
