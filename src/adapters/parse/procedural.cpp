#include <gvox/gvox.h>
#include <gvox/adapters/parse/procedural.h>

#include <cstdlib>
#include <cmath>
#include <array>

float stable_rand(float x) { return fmod(sin(x * (91.3458f)) * 47453.5453f, 1.0f); }
float stable_rand(float x, float y) { return fmod(sin(x * 12.9898f + y * 78.233f) * 43758.5453f, 1.0f); }
float stable_rand(float x, float y, float z) { return stable_rand(x + stable_rand(z), y + stable_rand(z)); }
float stable_rand(int32_t xi, int32_t yi, int32_t zi) {
    float const x = static_cast<float>(xi) * (1.0f / 8.0f);
    float const y = static_cast<float>(yi) * (1.0f / 8.0f);
    float const z = static_cast<float>(zi) * (1.0f / 8.0f);
    return stable_rand(x, y, z);
}

auto sample_terrain(float x, float y, float z) -> float {
    float const r = stable_rand(x, y, z);
    return sinf(x * 10) * 0.8f + sinf(y * 10) * 0.9f - z * 16.0f + 7.0f + r * 0.5f;
}

auto sample_terrain_i(int32_t xi, int32_t yi, int32_t zi) -> float {
    float const x = static_cast<float>(xi) * (1.0f / 8.0f);
    float const y = static_cast<float>(yi) * (1.0f / 8.0f);
    float const z = static_cast<float>(zi) * (1.0f / 8.0f);
    return sample_terrain(x, y, z);
}

extern "C" void gvox_parse_adapter_procedural_begin([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] void *config) {
}

extern "C" void gvox_parse_adapter_procedural_end([[maybe_unused]] GvoxAdapterContext *ctx) {
}

extern "C" auto gvox_parse_adapter_procedural_query_region_flags([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxRegionRange const *range, [[maybe_unused]] uint32_t channel_id) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_procedural_load_region([[maybe_unused]] GvoxAdapterContext *ctx, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxRegion {
    if (channel_id != GVOX_CHANNEL_ID_COLOR && channel_id != GVOX_CHANNEL_ID_NORMAL) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "procedural 'parser' does not generate anything other than color & normal");
        return {};
    }

    constexpr auto create_color = [](float rf, float gf, float bf) {
        uint32_t const r = static_cast<uint32_t>(std::max(std::min(rf, 1.0f), 0.0f) * 255.0f);
        uint32_t const g = static_cast<uint32_t>(std::max(std::min(gf, 1.0f), 0.0f) * 255.0f);
        uint32_t const b = static_cast<uint32_t>(std::max(std::min(bf, 1.0f), 0.0f) * 255.0f);
        uint32_t const a = 255;
        return (r << 0x00) | (g << 0x08) | (b << 0x10) | (a << 0x18);
    };

    constexpr auto create_normal = [](float xf, float yf, float zf) {
        uint32_t const x = static_cast<uint32_t>(std::max(std::min(xf * 0.5f + 0.5f, 1.0f), 0.0f) * 255.0f);
        uint32_t const y = static_cast<uint32_t>(std::max(std::min(yf * 0.5f + 0.5f, 1.0f), 0.0f) * 255.0f);
        uint32_t const z = static_cast<uint32_t>(std::max(std::min(zf * 0.5f + 0.5f, 1.0f), 0.0f) * 255.0f);
        uint32_t const w = 255;
        return (x << 0x00) | (y << 0x08) | (z << 0x10) | (w << 0x18);
    };

    uint32_t color = create_color(0.6f, 0.7f, 0.9f);
    uint32_t normal = create_normal(0.0f, 0.0f, 0.0f);

    float const val = sample_terrain_i(offset->x, offset->y, offset->z);
    if (val > -0.0f) {
        {
            float const nx_val = sample_terrain_i(offset->x + 1, offset->y, offset->z);
            float const ny_val = sample_terrain_i(offset->x, offset->y + 1, offset->z);
            float const nz_val = sample_terrain_i(offset->x, offset->y, offset->z + 1);
            float const nx = nx_val - val;
            float const ny = ny_val - val;
            float const nz = nz_val - val;
            float const inv_mag = 1.0f / sqrtf(nx * nx + ny * ny + nz * nz);
            normal = create_normal(nx * inv_mag, ny * inv_mag, nz * inv_mag);
        }
        int si = 0;
        for (si = 0; si < 16; ++si) {
            float const sval = sample_terrain_i(offset->x, offset->y, offset->z + si);
            if (sval < -0.0f) {
                break;
            }
        }
        if (si < 2) {
            color = create_color(0.2f, 0.5f, 0.1f);
        } else if (si < 4) {
            color = create_color(0.4f, 0.3f, 0.2f);
        } else {
            float const r = stable_rand(offset->x, offset->y, offset->z);
            if (r < 0.5f) {
                color = create_color(0.36f, 0.34f, 0.34f);
            } else {
                color = create_color(0.25f, 0.24f, 0.23f);
            }
        }
    }

    GvoxRegion const region = {
        .range = {
            .offset = *offset,
            .extent = {1, 1, 1},
        },
        .channels = channel_id,
        .flags = GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL,
        .data = malloc(sizeof(uint32_t) * 2),
    };
    *reinterpret_cast<std::array<uint32_t, 2> *>(region.data) = {color, normal};
    return region;
}

extern "C" void gvox_parse_adapter_procedural_unload_region([[maybe_unused]] GvoxAdapterContext *ctx, GvoxRegion *region) {
    free(region->data);
}

extern "C" auto gvox_parse_adapter_procedural_sample_region([[maybe_unused]] GvoxAdapterContext *ctx, GvoxRegion const *region, [[maybe_unused]] GvoxOffset3D const *offset, uint32_t channel_id) -> uint32_t {
    uint32_t index = 0;
    switch (channel_id) {
    case GVOX_CHANNEL_ID_COLOR: index = 0; break;
    case GVOX_CHANNEL_ID_NORMAL: index = 1; break;
    default:
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "Tried sampling something other than color or normal");
        return 0;
    }
    return (*reinterpret_cast<std::array<uint32_t, 2> const *>(region->data)).at(index);
}
