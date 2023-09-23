#include <gvox/stream.h>
#include <gvox/streams/output/byte_buffer.h>

#include <vector>
#include <algorithm>

struct GvoxByteBufferOutputStream {
    std::vector<uint8_t> bytes{};
    long current_read_head{};

    GvoxByteBufferOutputStream(GvoxByteBufferOutputStreamConfig const &config);

    auto write(uint8_t *data, size_t size) -> GvoxResult;
    auto seek(long offset, GvoxSeekOrigin origin) -> GvoxResult;
    auto tell() -> long;
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

auto GvoxByteBufferOutputStream::tell() -> long {
    return current_read_head;
}

auto gvox_output_stream_byte_buffer_description() GVOX_FUNC_ATTRIB->GvoxOutputStreamDescription {
    return GvoxOutputStreamDescription{
        .create = [](void **self, GvoxOutputStreamCreateCbArgs const *args) -> GvoxResult {
            GvoxByteBufferOutputStreamConfig config;
            if (args->config) {
                config = *static_cast<GvoxByteBufferOutputStreamConfig const *>(args->config);
            } else {
                config = {};
            }
            *self = new GvoxByteBufferOutputStream(config);
            return GVOX_SUCCESS;
        },
        .write = [](void *self, GvoxOutputStream next_handle, uint8_t *data, size_t size) -> GvoxResult {
            return static_cast<GvoxByteBufferOutputStream *>(self)->write(data, size);
        },
        .seek = [](void *self, GvoxOutputStream next_handle, long offset, GvoxSeekOrigin origin) -> GvoxResult {
            return static_cast<GvoxByteBufferOutputStream *>(self)->seek(offset, origin);
        },
        .tell = [](void *self, GvoxOutputStream next_handle) -> long {
            return static_cast<GvoxByteBufferOutputStream *>(self)->tell();
        },
        .destroy = [](void *self) { delete static_cast<GvoxByteBufferOutputStream *>(self); },
    };
}
