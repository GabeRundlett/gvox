#include <gvox/gvox.h>
#include <adapters/procedural.h>

#include <cstdlib>
#include <cmath>
#include <array>

float stable_rand(float x) { return fmod(sin(x * (91.3458f)) * 47453.5453f, 1.0f); }
float stable_rand(float x, float y) { return fmod(sin(x * 12.9898f + y * 78.233f) * 43758.5453f, 1.0f); }
float stable_rand(float x, float y, float z) { return stable_rand(x + stable_rand(z), y + stable_rand(z)); }
float stable_rand(int32_t xi, int32_t yi, int32_t zi) {
    float const x = (static_cast<float>(xi) + 0.5f) * (1.0f / 8.0f);
    float const y = (static_cast<float>(yi) + 0.5f) * (1.0f / 8.0f);
    float const z = (static_cast<float>(zi) + 0.5f) * (1.0f / 8.0f);
    return stable_rand(x, y, z);
}

auto sample_terrain(float x, float y, float z) -> float {
    return -(x * x + y * y + z * z) + 0.25f;
}

auto sample_terrain_i(int32_t xi, int32_t yi, int32_t zi) -> float {
    float const x = (static_cast<float>(xi) + 0.5f) * (1.0f / 8.0f);
    float const y = (static_cast<float>(yi) + 0.5f) * (1.0f / 8.0f);
    float const z = (static_cast<float>(zi) + 0.5f) * (1.0f / 8.0f);
    return sample_terrain(x, y, z);
}

extern "C" void procedural_create(GvoxAdapterContext *, void *) {
}
extern "C" void procedural_destroy(GvoxAdapterContext *) {
}
extern "C" void procedural_blit_begin(GvoxBlitContext *, GvoxAdapterContext *) {
}
extern "C" void procedural_blit_end(GvoxBlitContext *, GvoxAdapterContext *) {
}

extern "C" auto procedural_query_region_flags(GvoxBlitContext *, GvoxAdapterContext *, GvoxRegionRange const *, uint32_t) -> uint32_t {
    return 0;
}

extern "C" auto procedural_load_region(GvoxBlitContext *, GvoxAdapterContext *ctx, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxRegion {
    if (channel_id != GVOX_CHANNEL_ID_COLOR && channel_id != GVOX_CHANNEL_ID_NORMAL && channel_id != GVOX_CHANNEL_ID_MATERIAL_ID) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "procedural 'parser' does not generate anything other than color & normal");
        return {};
    }
    constexpr auto create_color = [](float rf, float gf, float bf, uint32_t const a) {
        uint32_t const r = static_cast<uint32_t>(std::max(std::min(rf, 1.0f), 0.0f) * 255.0f);
        uint32_t const g = static_cast<uint32_t>(std::max(std::min(gf, 1.0f), 0.0f) * 255.0f);
        uint32_t const b = static_cast<uint32_t>(std::max(std::min(bf, 1.0f), 0.0f) * 255.0f);
        return (r << 0x00) | (g << 0x08) | (b << 0x10) | (a << 0x18);
    };
    constexpr auto create_normal = [](float xf, float yf, float zf) {
        uint32_t const x = static_cast<uint32_t>(std::max(std::min(xf * 0.5f + 0.5f, 1.0f), 0.0f) * 255.0f);
        uint32_t const y = static_cast<uint32_t>(std::max(std::min(yf * 0.5f + 0.5f, 1.0f), 0.0f) * 255.0f);
        uint32_t const z = static_cast<uint32_t>(std::max(std::min(zf * 0.5f + 0.5f, 1.0f), 0.0f) * 255.0f);
        uint32_t const w = 0;
        return (x << 0x00) | (y << 0x08) | (z << 0x10) | (w << 0x18);
    };
    uint32_t color = create_color(0.6f, 0.7f, 0.9f, 0u);
    uint32_t normal = create_normal(0.0f, 0.0f, 0.0f);
    uint32_t id = 0;
    float const val = sample_terrain_i(offset->x, offset->y, offset->z);
    if (val >= 0.0f) {
        {
            float const nx_val = sample_terrain_i(offset->x - 1, offset->y, offset->z);
            float const ny_val = sample_terrain_i(offset->x, offset->y - 1, offset->z);
            float const nz_val = sample_terrain_i(offset->x, offset->y, offset->z - 1);
            float const px_val = sample_terrain_i(offset->x + 1, offset->y, offset->z);
            float const py_val = sample_terrain_i(offset->x, offset->y + 1, offset->z);
            float const pz_val = sample_terrain_i(offset->x, offset->y, offset->z + 1);
            if (nx_val < 0.0f || ny_val < 0.0f || nz_val < 0.0f || px_val < 0.0f || py_val < 0.0f || pz_val < 0.0f) {
                float const nx = px_val - val;
                float const ny = py_val - val;
                float const nz = pz_val - val;
                float const inv_mag = 1.0f / sqrtf(nx * nx + ny * ny + nz * nz);
                normal = create_normal(nx * inv_mag, ny * inv_mag, nz * inv_mag);
            }
        }
        int si = 0;
        for (si = 0; si < 16; ++si) {
            float const sval = sample_terrain_i(offset->x, offset->y, offset->z + si);
            if (sval < -0.0f) {
                break;
            }
        }
        if (si < 2) {
            color = create_color(0.2f, 0.5f, 0.1f, 1u);
            id = 1u;
        } else if (si < 4) {
            color = create_color(0.4f, 0.3f, 0.2f, 1u);
            id = 2u;
        } else {
            float const r = stable_rand(offset->x, offset->y, offset->z);
            if (r < 0.5f) {
                color = create_color(0.36f, 0.34f, 0.34f, 1u);
            } else {
                color = create_color(0.25f, 0.24f, 0.23f, 1u);
            }
            id = 3u;
        }
    }
    GvoxRegion const region = {
        .range = {
            .offset = *offset,
            .extent = {1, 1, 1},
        },
        .channels = channel_id,
        .flags = GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID,
        .data = malloc(sizeof(uint32_t) * 3),
    };
    *static_cast<std::array<uint32_t, 3> *>(region.data) = {color, normal, id};
    return region;
}

extern "C" void procedural_unload_region(GvoxBlitContext *, GvoxAdapterContext *, GvoxRegion *region) {
    free(region->data);
}

extern "C" auto procedural_sample_region(GvoxBlitContext *, GvoxAdapterContext *ctx, GvoxRegion const *region, GvoxOffset3D const *, uint32_t channel_id) -> uint32_t {
    uint32_t index = 0;
    switch (channel_id) {
    case GVOX_CHANNEL_ID_COLOR: index = 0; break;
    case GVOX_CHANNEL_ID_NORMAL: index = 1; break;
    case GVOX_CHANNEL_ID_MATERIAL_ID: index = 2; break;
    default:
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "Tried sampling something other than color or normal");
        return 0;
    }
    return (*static_cast<std::array<uint32_t, 3> const *>(region->data)).at(index);
}
