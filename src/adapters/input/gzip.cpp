#include <gvox/adapter.h>
#include <gvox/adapters/input/gzip.h>

#include <vector>
#include <algorithm>

#include <gzip/decompress.hpp>

struct GzipInputAdapter {
    std::vector<uint8_t> bytes{};
    long current_read_head{};

    explicit GzipInputAdapter(GzipInputAdapterConfig const &config);

    auto read(GvoxInputAdapter next_handle, uint8_t *data, size_t size) -> GvoxResult;
    auto seek(GvoxInputAdapter next_handle, long offset, GvoxSeekOrigin origin) -> GvoxResult;
    auto tell(GvoxInputAdapter next_handle) -> long;

    auto make_prepared(GvoxInputAdapter next_handle) -> GvoxResult;
};

GzipInputAdapter::GzipInputAdapter(GzipInputAdapterConfig const &) {
}

auto GzipInputAdapter::read(GvoxInputAdapter next_handle, uint8_t *data, size_t size) -> GvoxResult {
    auto prepare_result = make_prepared(next_handle);
    if (prepare_result != GVOX_SUCCESS) {
        return prepare_result;
    }
    auto position = current_read_head;
    current_read_head += static_cast<long>(size);
    if (static_cast<size_t>(current_read_head) > bytes.size()) {
        return GVOX_ERROR_UNKNOWN;
    }
    std::copy(bytes.data() + position, bytes.data() + position + size, static_cast<uint8_t *>(data));
    return GVOX_SUCCESS;
}

auto GzipInputAdapter::seek(GvoxInputAdapter next_handle, long offset, GvoxSeekOrigin origin) -> GvoxResult {
    auto prepare_result = make_prepared(next_handle);
    if (prepare_result != GVOX_SUCCESS) {
        return prepare_result;
    }
    switch (origin) {
    case GVOX_SEEK_ORIGIN_BEG: current_read_head = 0 + offset; break;
    case GVOX_SEEK_ORIGIN_END: current_read_head = static_cast<long>(bytes.size()) + offset; break;
    case GVOX_SEEK_ORIGIN_CUR: current_read_head = current_read_head + offset; break;
    default: break;
    }
    return GVOX_SUCCESS;
}

auto GzipInputAdapter::tell(GvoxInputAdapter next_handle) -> long {
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
auto GzipInputAdapter::make_prepared(GvoxInputAdapter next_handle) -> GvoxResult {
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
    zipped_bytes.resize(size);
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

auto gvox_input_adapter_gzip_create(void **self, GvoxInputAdapterCreateCbArgs const *args) -> GvoxResult {
    GzipInputAdapterConfig config;
    if (args->config != nullptr) {
        config = *static_cast<GzipInputAdapterConfig const *>(args->config);
    } else {
        config = {};
    }
    *self = new GzipInputAdapter(config);
    return GVOX_SUCCESS;
}
auto gvox_input_adapter_gzip_read(void *self, GvoxInputAdapter next_handle, uint8_t *data, size_t size) -> GvoxResult {
    return static_cast<GzipInputAdapter *>(self)->read(next_handle, data, size);
}
auto gvox_input_adapter_gzip_seek(void *self, GvoxInputAdapter next_handle, long offset, GvoxSeekOrigin origin) -> GvoxResult {
    return static_cast<GzipInputAdapter *>(self)->seek(next_handle, offset, origin);
}
auto gvox_input_adapter_gzip_tell(void *self, GvoxInputAdapter next_handle) -> long {
    return static_cast<GzipInputAdapter *>(self)->tell(next_handle);
}
void gvox_input_adapter_gzip_destroy(void *self) {
    delete static_cast<GzipInputAdapter *>(self);
}
