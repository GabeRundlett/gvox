#ifndef GVOX_PARSER_MAGICAVOXEL_H
#define GVOX_PARSER_MAGICAVOXEL_H

#include <gvox/stream.h>

typedef struct {
    // Optional object name filter.
    char const *object_name;
} MagicavoxelParserConfig;

GVOX_EXPORT GvoxParserDescription gvox_parser_magicavoxel_description(void) GVOX_FUNC_ATTRIB;

#endif
