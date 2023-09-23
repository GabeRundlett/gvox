#ifndef GVOX_INPUT_STREAM_GZIP_H
#define GVOX_INPUT_STREAM_GZIP_H

#include <gvox/stream.h>

typedef struct {
    uint8_t _pad;
} GzipInputStreamConfig;

GVOX_EXPORT GvoxInputStreamDescription gvox_input_stream_gzip_description(void) GVOX_FUNC_ATTRIB;

#endif
