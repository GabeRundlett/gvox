#ifndef GVOX_BYTE_BUFFER_OUTPUT_ADAPTER_H
#define GVOX_BYTE_BUFFER_OUTPUT_ADAPTER_H

#include <stdint.h>

typedef struct {
    size_t *out_size;
    uint8_t **out_byte_buffer_ptr;

    void *(*allocate)(size_t size);
} GVoxByteBufferOutputAdapterConfig;

#endif
