#ifndef GVOX_OUTPUT_STREAM_BYTE_BUFFER_H
#define GVOX_OUTPUT_STREAM_BYTE_BUFFER_H

#include <gvox/stream.h>

typedef struct {
    uint8_t _pad;
} GvoxByteBufferOutputStreamConfig;

GVOX_EXPORT GvoxOutputStreamDescription gvox_output_stream_byte_buffer_description(void) GVOX_FUNC_ATTRIB;

#endif
