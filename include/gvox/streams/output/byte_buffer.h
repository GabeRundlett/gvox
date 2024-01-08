#ifndef GVOX_OUTPUT_STREAM_BYTE_BUFFER_H
#define GVOX_OUTPUT_STREAM_BYTE_BUFFER_H

#include <gvox/stream.h>

GVOX_STRUCT(GvoxByteBufferOutputStreamConfig) {
    uint8_t _pad;
};

GVOX_FUNC(GvoxOutputStreamDescription, gvox_output_stream_byte_buffer_description, void);

#endif
