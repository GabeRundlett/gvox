#ifndef GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_H
#define GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_H

typedef enum {
    GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_DOWNSCALE_MODE_NEAREST,
    GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_DOWNSCALE_MODE_LINEAR,
} GvoxColoredTextSerializeAdapterDownscaleMode;

typedef struct {
    // By default, this should be 1.
    // If set to n, then each 'pixel' will represent n*n*n voxels
    uint32_t downscale_factor;
    // By default, this is ..._NEAREST.
    GvoxColoredTextSerializeAdapterDownscaleMode downscale_mode;
    // By default, this is GVOX_CHANNEL_ID_COLOR
    uint32_t channel_id;
    // Doesn't make sense to have a default for this, when the default channel is color data
    uint32_t non_color_max_value;
} GvoxColoredTextSerializeAdapterConfig;

#endif
