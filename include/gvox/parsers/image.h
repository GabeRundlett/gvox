#ifndef GVOX_PARSER_IMAGE_H
#define GVOX_PARSER_IMAGE_H

#include <gvox/stream.h>

typedef struct {
    uint8_t _pad;
} GvoxImageParserConfig;

GVOX_EXPORT GvoxParserDescription gvox_parser_image_description(void) GVOX_FUNC_ATTRIB;

#endif
