#ifndef GVOX_VOXLAP_PARSE_ADAPTER_H
#define GVOX_VOXLAP_PARSE_ADAPTER_H

typedef struct {
    // If the config is null, or the value is -1, the default will be used.

    // By default, the size of the map is 1024x1024x256
    uint32_t size_x;
    uint32_t size_y;
    uint32_t size_z;

    // This determines whether to make the insides of the map hollow or solid
    // By default, this is set to 1.
    uint8_t make_solid;

    // This designates whether the data should be parsed as Ace of Spades,
    // since Ace of Spades maps have no file header.
    // By default, this is set to 0.
    uint8_t is_ace_of_spades;
} GvoxVoxlapParseAdapterConfig;

#endif
