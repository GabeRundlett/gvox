#ifndef GVOX_PARSER_MAGICAVOXEL_H
#define GVOX_PARSER_MAGICAVOXEL_H

#include <gvox/stream.h>

typedef struct {
    uint8_t _pad;
} MagicavoxelParserConfig;

GVOX_EXPORT GvoxParserDescription gvox_parser_magicavoxel_description(void) GVOX_FUNC_ATTRIB;

#endif
