#ifndef GVOX_OUTPUT_STREAM_STDOUT_H
#define GVOX_OUTPUT_STREAM_STDOUT_H

#include <gvox/gvox.h>

GVOX_STRUCT(GvoxStdoutOutputStreamConfig) {
    uint8_t _pad;
};

GVOX_FUNC(GvoxOutputStreamDescription, gvox_output_stream_stdout_description, void);

#endif
