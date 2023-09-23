#include <gvox/stream.h>
#include <gvox/streams/input/byte_buffer.h>

#include <vector>
#include <algorithm>

struct GvoxByteBufferInputStream {
    std::vector<uint8_t> bytes{};
    long current_read_head{};

    explicit GvoxByteBufferInputStream(GvoxByteBufferInputStreamConfig const &config);

    auto read(uint8_t *data, size_t size) -> GvoxResult;
    auto seek(long offset, GvoxSeekOrigin origin) -> GvoxResult;
    auto tell() -> long;
};

GvoxByteBufferInputStream::GvoxByteBufferInputStream(GvoxByteBufferInputStreamConfig const &config) {
    bytes.resize(config.size);
    std::copy(config.data, config.data + config.size, bytes.begin());
}

auto GvoxByteBufferInputStream::read(uint8_t *data, size_t size) -> GvoxResult {
    auto position = current_read_head;
    current_read_head += static_cast<long>(size);
    if (static_cast<size_t>(current_read_head) > bytes.size()) {
        return GVOX_ERROR_UNKNOWN;
    }
    std::copy(bytes.data() + position, bytes.data() + position + size, static_cast<uint8_t *>(data));
    return GVOX_SUCCESS;
}

auto GvoxByteBufferInputStream::seek(long offset, GvoxSeekOrigin origin) -> GvoxResult {
    switch (origin) {
    case GVOX_SEEK_ORIGIN_BEG: current_read_head = 0 + offset; break;
    case GVOX_SEEK_ORIGIN_END: current_read_head = static_cast<long>(bytes.size()) + offset; break;
    case GVOX_SEEK_ORIGIN_CUR: current_read_head = current_read_head + offset; break;
    default: break;
    }
    return GVOX_SUCCESS;
}

auto GvoxByteBufferInputStream::tell() -> long {
    return current_read_head;
}

namespace {
    auto create(void **self, GvoxInputStreamCreateCbArgs const *args) -> GvoxResult {
        GvoxByteBufferInputStreamConfig config;
        if (args->config != nullptr) {
            config = *static_cast<GvoxByteBufferInputStreamConfig const *>(args->config);
        } else {
            config = {};
        }
        *self = new GvoxByteBufferInputStream(config);
        return GVOX_SUCCESS;
    }
    auto read(void *self, GvoxInputStream /*unused*/, uint8_t *data, size_t size) -> GvoxResult {
        return static_cast<GvoxByteBufferInputStream *>(self)->read(data, size);
    }
    auto seek(void *self, GvoxInputStream /*unused*/, long offset, GvoxSeekOrigin origin) -> GvoxResult {
        return static_cast<GvoxByteBufferInputStream *>(self)->seek(offset, origin);
    }
    auto tell(void *self, GvoxInputStream /*unused*/) -> long {
        return static_cast<GvoxByteBufferInputStream *>(self)->tell();
    }
    void destroy(void *self) {
        delete static_cast<GvoxByteBufferInputStream *>(self);
    }
} // namespace

auto gvox_input_stream_byte_buffer_description() GVOX_FUNC_ATTRIB->GvoxInputStreamDescription {
    return GvoxInputStreamDescription{.create = create, .read = read, .seek = seek, .tell = tell, .destroy = destroy};
}
