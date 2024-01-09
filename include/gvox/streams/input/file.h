#ifndef GVOX_INPUT_STREAM_FILE_H
#define GVOX_INPUT_STREAM_FILE_H

#include <gvox/gvox.h>

GVOX_STRUCT(GvoxFileInputStreamConfig) {
    char const *filepath;
};

GVOX_FUNC(GvoxInputStreamDescription, gvox_input_stream_file_description, void);

#endif
