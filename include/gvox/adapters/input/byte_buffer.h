#ifndef GVOX_INPUT_ADAPTER_BYTE_BUFFER_H
#define GVOX_INPUT_ADAPTER_BYTE_BUFFER_H

#include <stdint.h>

typedef struct {
    uint8_t const *data;
    size_t size;
} GvoxByteBufferInputAdapterConfig;

#endif
