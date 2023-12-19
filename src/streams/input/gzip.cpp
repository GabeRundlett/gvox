#include <gvox/stream.h>
#include <gvox/streams/input/gzip.h>

#include <new>
#include <vector>
#include <algorithm>
#include <cstdint>
#include <cstddef>
#include <cstring>

#include <gzip/decompress.hpp>

struct GzipInputStream {
    std::vector<uint8_t> bytes{};
    int64_t current_read_head{};

    explicit GzipInputStream(GzipInputStreamConfig const &config);

    auto read(GvoxInputStream next_handle, void *data, size_t size) -> GvoxResult;
    auto seek(GvoxInputStream next_handle, int64_t offset, GvoxSeekOrigin origin) -> GvoxResult;
    auto tell(GvoxInputStream next_handle) -> int64_t;

    auto make_prepared(GvoxInputStream next_handle) -> GvoxResult;
};

GzipInputStream::GzipInputStream(GzipInputStreamConfig const & /*unused*/) {
}

auto GzipInputStream::read(GvoxInputStream next_handle, void *data, size_t size) -> GvoxResult {
    auto prepare_result = make_prepared(next_handle);
    if (prepare_result != GVOX_SUCCESS) {
        return prepare_result;
    }
    auto position = current_read_head;
    current_read_head += static_cast<int64_t>(size);
    if (static_cast<size_t>(current_read_head) > bytes.size()) {
        return GVOX_ERROR_UNKNOWN;
    }
    std::copy(bytes.data() + position, bytes.data() + position + size, static_cast<uint8_t *>(data));
    return GVOX_SUCCESS;
}

auto GzipInputStream::seek(GvoxInputStream next_handle, int64_t offset, GvoxSeekOrigin origin) -> GvoxResult {
    auto prepare_result = make_prepared(next_handle);
    if (prepare_result != GVOX_SUCCESS) {
        return prepare_result;
    }
    switch (origin) {
    case GVOX_SEEK_ORIGIN_BEG: current_read_head = 0 + offset; break;
    case GVOX_SEEK_ORIGIN_END: current_read_head = static_cast<int64_t>(bytes.size()) + offset; break;
    case GVOX_SEEK_ORIGIN_CUR: current_read_head = current_read_head + offset; break;
    default: break;
    }
    return GVOX_SUCCESS;
}

auto GzipInputStream::tell(GvoxInputStream next_handle) -> int64_t {
    auto prepare_result = make_prepared(next_handle);
    if (prepare_result != GVOX_SUCCESS) {
        return prepare_result;
    }
    return current_read_head;
}

#define HANDLE_RES             \
    if (res != GVOX_SUCCESS) { \
        return res;            \
    }
auto GzipInputStream::make_prepared(GvoxInputStream next_handle) -> GvoxResult {
    auto res = GVOX_SUCCESS;
    res = gvox_input_seek(next_handle, 0, GVOX_SEEK_ORIGIN_END);
    HANDLE_RES;
    auto size = gvox_input_tell(next_handle);
    if (size == 0) {
        return GVOX_ERROR_UNKNOWN;
    }
    res = gvox_input_seek(next_handle, 0, GVOX_SEEK_ORIGIN_BEG);
    HANDLE_RES;
    std::vector<uint8_t> zipped_bytes{};
    zipped_bytes.resize(static_cast<size_t>(size));
    res = gvox_input_read(next_handle, zipped_bytes.data(), zipped_bytes.size());
    HANDLE_RES;

    try {
        auto decompressed_str = gzip::decompress(reinterpret_cast<char *>(zipped_bytes.data()), zipped_bytes.size());
        bytes.resize(decompressed_str.size());
        memcpy(bytes.data(), decompressed_str.data(), bytes.size());
    } catch (...) {
        return GVOX_ERROR_UNKNOWN;
    }
    return res;
}

auto gvox_input_stream_gzip_description() GVOX_FUNC_ATTRIB->GvoxInputStreamDescription {
    return GvoxInputStreamDescription{
        .create = [](void **self, GvoxInputStreamCreateCbArgs const *args) -> GvoxResult {
            GzipInputStreamConfig config;
            if (args->config != nullptr) {
                config = *static_cast<GzipInputStreamConfig const *>(args->config);
            } else {
                config = {};
            }
            *self = new (std::nothrow) GzipInputStream(config);
            return GVOX_SUCCESS;
        },
        .read = [](void *self, GvoxInputStream next_handle, void *data, size_t size) -> GvoxResult {
            return static_cast<GzipInputStream *>(self)->read(next_handle, data, size);
        },
        .seek = [](void *self, GvoxInputStream next_handle, int64_t offset, GvoxSeekOrigin origin) -> GvoxResult {
            return static_cast<GzipInputStream *>(self)->seek(next_handle, offset, origin);
        },
        .tell = [](void *self, GvoxInputStream next_handle) -> int64_t {
            return static_cast<GzipInputStream *>(self)->tell(next_handle);
        },
        .destroy = [](void *self) { delete static_cast<GzipInputStream *>(self); },
    };
}
