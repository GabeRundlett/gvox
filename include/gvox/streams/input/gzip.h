#ifndef GVOX_INPUT_STREAM_GZIP_H
#define GVOX_INPUT_STREAM_GZIP_H

#include <gvox/gvox.h>

GVOX_STRUCT(GzipInputStreamConfig) {
    uint8_t _pad;
};

GVOX_FUNC(GvoxInputStreamDescription, gvox_input_stream_gzip_description, void);

#endif
