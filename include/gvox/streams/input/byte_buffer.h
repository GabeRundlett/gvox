#ifndef GVOX_INPUT_STREAM_BYTE_BUFFER_H
#define GVOX_INPUT_STREAM_BYTE_BUFFER_H

#include <gvox/stream.h>

typedef struct {
    uint8_t const *data;
    size_t size;
} GvoxByteBufferInputStreamConfig;

GVOX_EXPORT GvoxInputStreamDescription gvox_input_stream_byte_buffer_description(void) GVOX_FUNC_ATTRIB;

#endif
