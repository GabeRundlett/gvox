#ifndef GVOX_INPUT_STREAM_BYTE_BUFFER_H
#define GVOX_INPUT_STREAM_BYTE_BUFFER_H

#include <gvox/stream.h>

GVOX_STRUCT(GvoxByteBufferInputStreamConfig) {
    uint8_t const *data;
    size_t size;
};

GVOX_FUNC(GvoxInputStreamDescription, gvox_input_stream_byte_buffer_description, void);

#endif
