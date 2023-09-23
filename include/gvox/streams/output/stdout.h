#ifndef GVOX_OUTPUT_STREAM_STDOUT_H
#define GVOX_OUTPUT_STREAM_STDOUT_H

#include <gvox/stream.h>

typedef struct {
    uint8_t _pad;
} GvoxStdoutOutputStreamConfig;

GVOX_EXPORT GvoxOutputStreamDescription gvox_output_stream_stdout_description(void) GVOX_FUNC_ATTRIB;

#endif
