#ifndef GVOX_INPUT_STREAM_FILE_H
#define GVOX_INPUT_STREAM_FILE_H

#include <gvox/stream.h>

typedef struct {
    char const *filepath;
} GvoxFileInputStreamConfig;

GVOX_EXPORT GvoxInputStreamDescription gvox_input_stream_file_description(void) GVOX_FUNC_ATTRIB;

#endif
