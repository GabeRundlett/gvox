#ifndef GVOX_BYTE_BUFFER_INPUT_ADAPTER_H
#define GVOX_BYTE_BUFFER_INPUT_ADAPTER_H

#include <stdint.h>

typedef struct {
    uint8_t const *data;
    size_t size;
} GvoxByteBufferInputAdapterConfig;

#endif
