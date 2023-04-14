#ifndef GVOX_FILE_IO_ADAPTER_H
#define GVOX_FILE_IO_ADAPTER_H

typedef enum {
    GVOX_FILE_IO_ADAPTER_OPEN_MODE_INPUT,
    GVOX_FILE_IO_ADAPTER_OPEN_MODE_OUTPUT,
} GvoxFileIoAdapterOpenMode;

typedef struct {
    char const *filepath;
    GvoxFileIoAdapterOpenMode open_mode;
} GvoxFileIoAdapterConfig;

#endif
