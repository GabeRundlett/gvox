#include <gvox/gvox.h>
#include <gvox/adapters/output/byte_buffer.h>

#include <vector>
#include <algorithm>

struct GvoxByteBufferOutputAdapter {
    std::vector<uint8_t> bytes{};
    long current_read_head{};

    GvoxByteBufferOutputAdapter(GvoxByteBufferOutputAdapterConfig const &config);

    auto write(uint8_t *data, size_t size) -> GvoxResult;
    auto seek(long offset, GvoxSeekOrigin origin) -> GvoxResult;
    auto tell() -> long;
};

GvoxByteBufferOutputAdapter::GvoxByteBufferOutputAdapter(GvoxByteBufferOutputAdapterConfig const &config) {
    // bytes.resize(config.size);
    // std::copy(config.data, config.data + config.size, bytes.begin());
}

auto GvoxByteBufferOutputAdapter::write(uint8_t *data, size_t size) -> GvoxResult {
    // auto position = current_read_head;
    // current_read_head += size;
    // if (current_read_head > bytes.size()) {
    //     return GVOX_ERROR_UNKNOWN;
    // }
    // std::copy(user_state.bytes.data() + position, user_state.bytes.data() + position + size, static_cast<uint8_t *>(data));
    // return GVOX_SUCCESS;
    return GVOX_ERROR_UNKNOWN;
}

auto GvoxByteBufferOutputAdapter::seek(long offset, GvoxSeekOrigin origin) -> GvoxResult {
    switch (origin) {
    case GVOX_SEEK_ORIGIN_BEG: current_read_head = 0 + offset; break;
    case GVOX_SEEK_ORIGIN_END: current_read_head = bytes.size() + offset; break;
    case GVOX_SEEK_ORIGIN_CUR: current_read_head = current_read_head + offset; break;
    default: break;
    }
    return GVOX_SUCCESS;
}

auto GvoxByteBufferOutputAdapter::tell() -> long {
    return current_read_head;
}

GvoxResult gvox_output_adapter_byte_buffer_create(void **self, GvoxOutputAdapterCreateCbArgs const *args) {
    GvoxByteBufferOutputAdapterConfig config;
    if (args->config) {
        config = *static_cast<GvoxByteBufferOutputAdapterConfig const *>(args->config);
    } else {
        config = {};
    }
    *self = new GvoxByteBufferOutputAdapter(config);
    return GVOX_SUCCESS;
}
GvoxResult gvox_output_adapter_byte_buffer_write(void *self, GvoxOutputAdapter next_handle, uint8_t *data, size_t size) {
    return static_cast<GvoxByteBufferOutputAdapter *>(self)->write(data, size);
}
GvoxResult gvox_output_adapter_byte_buffer_seek(void *self, GvoxOutputAdapter next_handle, long offset, GvoxSeekOrigin origin) {
    return static_cast<GvoxByteBufferOutputAdapter *>(self)->seek(offset, origin);
}
auto gvox_output_adapter_byte_adapter_tell(void *self, GvoxOutputAdapter next_handle) -> long {
    return static_cast<GvoxByteBufferOutputAdapter *>(self)->tell();
}
void gvox_output_adapter_byte_buffer_destroy(void *self) {
    delete static_cast<GvoxByteBufferOutputAdapter *>(self);
}
