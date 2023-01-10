#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <gvox/gvox.h>

#include "print.h"

void f0_serialize_region(GVoxSerializeState *state, GVoxRegion const *region) {
    for (uint32_t zi = 0; zi < region->metadata.extent.z; ++zi) {
        for (uint32_t yi = 0; yi < region->metadata.extent.y; ++yi) {
            for (uint32_t xi = 0; xi < region->metadata.extent.x; ++xi) {
                GVoxExtent3D const pos = {xi, yi, zi};
                GVoxExtent3D const o_pos = {
                    xi + (uint32_t)(state->full_region_metadata->offset.x - region->metadata.offset.x),
                    yi + (uint32_t)(state->full_region_metadata->offset.y - region->metadata.offset.y),
                    zi + (uint32_t)(state->full_region_metadata->offset.z - region->metadata.offset.z),
                };
                GVoxVoxel voxel = gvox_region_sample_voxel(region, &pos);
                uint32_t o_index =
                    o_pos.x +
                    o_pos.y * state->full_region_metadata->extent.x +
                    o_pos.z * state->full_region_metadata->extent.x * state->full_region_metadata->extent.y;
                gvox_serialize_write(
                    state,
                    sizeof(GVoxOffset3D) + sizeof(GVoxExtent3D) + o_index * sizeof(GVoxVoxel),
                    sizeof(GVoxVoxel),
                    &voxel);
            }
        }
    }
}

void f0_serialize_begin(GVoxSerializeState *state) {
    uint32_t full_size = sizeof(GVoxOffset3D) + sizeof(GVoxExtent3D);
    full_size +=
        state->full_region_metadata->extent.x *
        state->full_region_metadata->extent.y *
        state->full_region_metadata->extent.z *
        sizeof(GVoxVoxel);
    gvox_serialize_reserve(state, full_size);
    gvox_serialize_write(
        state,
        0,
        sizeof(GVoxExtent3D),
        &state->full_region_metadata->offset);
    gvox_serialize_write(
        state,
        sizeof(GVoxOffset3D),
        sizeof(GVoxExtent3D),
        &state->full_region_metadata->extent);
}

void f0_serialize_end(GVoxSerializeState *state) {
}

void f0_parse(GVoxParseState *state) {
}

typedef struct {
    char const *filepath;
} O0Config;

typedef struct {
    size_t size;
    void *buffer;
} O0State;

void o0_begin(GVoxSerializeState *state) {
    O0State *user_state = malloc(sizeof(O0State));
    user_state->size = 0;
    user_state->buffer = NULL;
    state->output_user_ptr = user_state;
}

void o0_end(GVoxSerializeState *state) {
    O0State *user_state = (O0State *)(state->output_user_ptr);
    O0Config *config = (O0Config *)(state->output_config);
    printf("sz: %llu\n", user_state->size);
    FILE *o_file = NULL;
    fopen_s(&o_file, config->filepath, "wb");
    fwrite(user_state->buffer, 1, user_state->size, o_file);
    fclose(o_file);
    free(user_state->buffer);
}

void o0_write(GVoxSerializeState *state, size_t position, size_t size, void const *data) {
    O0State *user_state = (O0State *)(state->output_user_ptr);
    if (position + size > user_state->size) {
        user_state->size = (position + size) * 2;
        printf("Reallocating... new size %llu\n", user_state->size);
        user_state->buffer = realloc(user_state->buffer, user_state->size);
    }
    memcpy(user_state->buffer, data, size);
}

void o0_reserve(GVoxSerializeState *state, size_t size) {
    O0State *user_state = (O0State *)(state->output_user_ptr);
    O0Config *config = (O0Config *)(state->output_config);
    if (size > user_state->size) {
        user_state->size = size;
        printf("Reserving... new size %llu\n", user_state->size);
        user_state->buffer = realloc(user_state->buffer, user_state->size);
    }
}

typedef struct {
    char const *filepath;
} I0Config;

typedef struct {
    FILE *file;
} I0State;

void i0_begin(GVoxParseState *state) {
    I0State *user_state = malloc(sizeof(I0State));
    I0Config *config = (I0Config *)(state->input_config);
    fopen_s(&user_state->file, config->filepath, "rb");
    state->input_user_ptr = user_state;
}

void i0_end(GVoxParseState *state) {
    I0State *user_state = (I0State *)(state->input_user_ptr);
}

void i0_read(GVoxParseState *state, size_t position, size_t size, void **data) {
    I0State *user_state = (I0State *)(state->input_user_ptr);
    fseek(user_state->file, position, SEEK_SET);
    size_t byte_n = fread(data, 1, size, user_state->file);
    assert(byte_n == size);
}

int main(void) {
    GVoxContext *gvox_ctx = gvox_create_context();

    GVoxInputAdapterInfo i0_adapter_info = {0};
    i0_adapter_info.name_str = "i0";
    i0_adapter_info.begin = i0_begin;
    i0_adapter_info.end = i0_end;
    i0_adapter_info.read = i0_read;
    GVoxInputAdapter *i0_adapter = gvox_register_input_adapter(gvox_ctx, &i0_adapter_info);

    GVoxOutputAdapterInfo o0_adapter_info = {0};
    o0_adapter_info.name_str = "o0";
    o0_adapter_info.begin = o0_begin;
    o0_adapter_info.end = o0_end;
    o0_adapter_info.write = o0_write;
    o0_adapter_info.reserve = o0_reserve;
    GVoxOutputAdapter *o0_adapter = gvox_register_output_adapter(gvox_ctx, &o0_adapter_info);

    GVoxFormatAdapterInfo f0_adapter_info = {0};
    f0_adapter_info.name_str = "f0";
    f0_adapter_info.serialize_begin = f0_serialize_begin;
    f0_adapter_info.serialize_end = f0_serialize_end;
    f0_adapter_info.serialize_region = f0_serialize_region;
    f0_adapter_info.parse = f0_parse;
    GVoxFormatAdapter *f0_adapter = gvox_register_format_adapter(gvox_ctx, &f0_adapter_info);

    {
        uint32_t voxel_data = 0x00ff00ff;
        GVoxRegion region = {
            .metadata = {
                .offset = {0, 0, 0},
                .extent = {8, 8, 8},
                .flags = GVOX_REGION_IS_UNIFORM_BIT,
            },
            .data = &voxel_data,
        };

        GVoxRegionList regions = {
            .region_n = 1,
            .regions = &region,
        };
        print_voxels(&regions);

        O0Config const o0_config = {
            .filepath = "test.gvox",
        };

        gvox_serialize(o0_adapter, &o0_config, f0_adapter, NULL, &region);
    }

    {
        I0Config const i0_config = {
            .filepath = "test.gvox",
        };

        GVoxRegionList regions = gvox_parse(i0_adapter, &i0_config, f0_adapter, NULL);
        print_voxels(&regions);
    }

    gvox_destroy_context(gvox_ctx);
}
