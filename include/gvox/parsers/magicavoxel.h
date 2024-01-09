#ifndef GVOX_PARSER_MAGICAVOXEL_H
#define GVOX_PARSER_MAGICAVOXEL_H

#include <gvox/gvox.h>

GVOX_STRUCT(MagicavoxelParserConfig) {
    // Optional object name filter.
    char const *object_name;
};

GVOX_FUNC(GvoxParserDescription, gvox_parser_magicavoxel_description, void);

GVOX_STRUCT(MagicavoxelXrawParserConfig) {
    uint8_t _pad;
};

GVOX_FUNC(GvoxParserDescription, gvox_parser_magicavoxel_xraw_description, void);

#endif
