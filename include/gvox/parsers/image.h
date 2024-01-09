#ifndef GVOX_PARSER_IMAGE_H
#define GVOX_PARSER_IMAGE_H

#include <gvox/gvox.h>

GVOX_STRUCT(GvoxImageParserConfig) {
    uint8_t _pad;
};

GVOX_FUNC(GvoxParserDescription, gvox_parser_image_description, void);

#endif
