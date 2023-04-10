#include <gvox/gvox.h>

typedef struct _GvoxContainer *GvoxContainer;

typedef struct {
    uint32_t type;
    void *p_next;
    GvoxContainer container;
    GvoxRegionRange *range;
    uint32_t channel_flags;
} GvoxLoadInfo;

typedef struct {
    uint32_t dimension_n;
    float *translation;
    float *rotation;
    float *scale;
} GvoxTransform;

typedef struct {
    
} GvoxSamplerCreateInfo;

typedef struct {
    uint32_t type;
    void *p_next;
    GvoxTransform transform;
} GvoxLoadTransform;

enum GvoxLoadTypes {
    GVOX_TYPE_LOAD_INFO,
    GVOX_TYPE_LOAD_TRANSFORM,
};

int main() {
    GvoxContext *gvox_ctx = gvox_create_context();

    {
        GvoxContainer png_container = gvox_create_container(
            gvox_ctx,
            gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "png"), NULL),
            NULL);

        // Load the color data into the container
        float translation[2] = {0, 0};
        float rotation[2] = {0, 0};
        float scale[2] = {1, 1};
        GvoxTransform transform;
        transform.dimension_n = 2;
        transform.translation = translation; // can all be NULL
        transform.rotation = rotation;
        transform.scale = scale;

        GvoxLoadInfo load_info;
        load_info.type = GVOX_TYPE_LOAD_INFO;
        load_info.p_next = NULL;
        load_info.container = png_container;
        load_info.transform = transform;
        load_info.range = NULL;
        load_info.channel_flags = GVOX_CHANNEL_BIT_COLOR;

        gvox_load(gvox_ctx, load_info);
    }

    // uint8_t *data = NULL;
    // size_t size = 0;

    // GvoxContainer gvp_container = gvox_create_container(
    //     gvox_ctx,
    //     gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "gvox_palette"), NULL),
    //     gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "gvox_palette"), NULL));

    // GvoxRegionRange region_range = {
    //     .offset = {-4, -4, -4},
    //     .extent = {+8, +8, +8},
    // };
    // gvox_blit_region(
    //     png_container,
    //     i_ctx, o_ctx, p_ctx, s_ctx,
    //     &region_range,
    //     GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_NORMAL | GVOX_CHANNEL_BIT_MATERIAL_ID);

    // gvox_destroy_adapter_context(i_ctx);
    // gvox_destroy_adapter_context(o_ctx);
    // gvox_destroy_adapter_context(p_ctx);
    // gvox_destroy_adapter_context(s_ctx);
}
