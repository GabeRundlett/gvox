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
    // Doesn't make sense to have a default for this, when the default channel is color data
    uint32_t non_color_max_value;
    // By default, this should be 0
    // if set to 1, each layer will be printed below the last
    uint8_t vertical;
} GvoxColoredTextSerializeAdapterConfig;

#endif
