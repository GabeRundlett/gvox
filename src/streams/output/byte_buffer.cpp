#include <gvox/gvox.h>
#include <gvox/streams/output/byte_buffer.h>

#include <vector>
#include <algorithm>

struct GvoxByteBufferOutputStream {
    std::vector<uint8_t> bytes{};
    long current_read_head{};

    GvoxByteBufferOutputStream(GvoxByteBufferOutputStreamConfig const &config);

    auto write(uint8_t *data, size_t size) -> GvoxResult;
    auto seek(long offset, GvoxSeekOrigin origin) -> GvoxResult;
};

GvoxByteBufferOutputStream::GvoxByteBufferOutputStream(GvoxByteBufferOutputStreamConfig const &config) {
    // bytes.resize(config.size);
    // std::copy(config.data, config.data + config.size, bytes.begin());
}

auto GvoxByteBufferOutputStream::write(uint8_t *data, size_t size) -> GvoxResult {
    // auto position = current_read_head;
    // current_read_head += size;
    // if (current_read_head > bytes.size()) {
    //     return GVOX_ERROR_UNKNOWN;
    // }
    // std::copy(user_state.bytes.data() + position, user_state.bytes.data() + position + size, static_cast<uint8_t *>(data));
    // return GVOX_SUCCESS;
    return GVOX_ERROR_UNKNOWN;
}

auto GvoxByteBufferOutputStream::seek(long offset, GvoxSeekOrigin origin) -> GvoxResult {
    switch (origin) {
    case GVOX_SEEK_ORIGIN_BEG: current_read_head = 0 + offset; break;
    case GVOX_SEEK_ORIGIN_END: current_read_head = bytes.size() + offset; break;
    case GVOX_SEEK_ORIGIN_CUR: current_read_head = current_read_head + offset; break;
    default: break;
    }
    return GVOX_SUCCESS;
}

GvoxResult gvox_output_stream_byte_buffer_create(void **self, void const *config_ptr) {
    GvoxByteBufferOutputStreamConfig config;
    if (config_ptr) {
        config = *static_cast<GvoxByteBufferOutputStreamConfig const *>(config_ptr);
    } else {
        config = {};
    }
    *self = new GvoxByteBufferOutputStream(config);
    return GVOX_SUCCESS;
}

GvoxResult gvox_output_stream_byte_buffer_write(void *self, uint8_t *data, size_t size) {
    return static_cast<GvoxByteBufferOutputStream *>(self)->write(data, size);
}

GvoxResult gvox_output_stream_byte_buffer_seek(void *self, long offset, GvoxSeekOrigin origin) {
    return static_cast<GvoxByteBufferOutputStream *>(self)->seek(offset, origin);
}

void gvox_output_stream_byte_buffer_destroy(void *self) {
    delete static_cast<GvoxByteBufferOutputStream *>(self);
}
