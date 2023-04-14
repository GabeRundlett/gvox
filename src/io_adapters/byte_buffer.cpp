#include <gvox/gvox.h>
#include <gvox/io_adapters/byte_buffer.h>

struct GvoxByteBufferIoAdapter {
    GvoxByteBufferIoAdapterConfig config{};

    GvoxByteBufferIoAdapter(GvoxByteBufferIoAdapterConfig const &a_config);
};

GvoxByteBufferIoAdapter::GvoxByteBufferIoAdapter(GvoxByteBufferIoAdapterConfig const &a_config) : config{a_config} {
}

GvoxResult gvox_io_adapter_byte_buffer_create(void **self, void const *config_ptr) {
    GvoxByteBufferIoAdapterConfig config;
    if (config_ptr) {
        config = *static_cast<GvoxByteBufferIoAdapterConfig const *>(config_ptr);
    } else {
        config = {};
    }
    *self = new GvoxByteBufferIoAdapter(config);
    return GVOX_SUCCESS;
}

GvoxResult gvox_io_adapter_byte_buffer_input_read(void * /*self*/, uint8_t * /*data*/, size_t /*size*/) {
    return GVOX_ERROR_UNKNOWN;
}

GvoxResult gvox_io_adapter_byte_buffer_input_seek(void * /*self*/, long /*offset*/, GvoxSeekOrigin /*origin*/) {
    return GVOX_ERROR_UNKNOWN;
}

void gvox_io_adapter_byte_buffer_destroy(void *self) {
    delete static_cast<GvoxByteBufferIoAdapter *>(self);
}
