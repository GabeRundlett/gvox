#include <gvox/adapter.h>
#include <gvox/adapters/input/file.h>

#include <fstream>

struct GvoxFileInputAdapter {
    GvoxFileInputAdapterConfig config{};
    std::ifstream file_handle;

    explicit GvoxFileInputAdapter(GvoxFileInputAdapterConfig const &a_config);

    auto read(uint8_t *data, size_t size) -> GvoxResult;
    auto seek(long offset, GvoxSeekOrigin origin) -> GvoxResult;
    auto tell() -> long;
};

GvoxFileInputAdapter::GvoxFileInputAdapter(GvoxFileInputAdapterConfig const &a_config) : config{a_config} {
    file_handle = std::ifstream{config.filepath, std::ios::binary};
}

auto GvoxFileInputAdapter::read(uint8_t *data, size_t size) -> GvoxResult {
    file_handle.read(reinterpret_cast<char *>(data), static_cast<std::streamsize>(size));
    return GVOX_SUCCESS;
}

auto GvoxFileInputAdapter::seek(long offset, GvoxSeekOrigin origin) -> GvoxResult {
    auto seekdir = std::ios_base::seekdir{};
    switch (origin) {
    case GVOX_SEEK_ORIGIN_BEG: seekdir = std::ios::beg; break;
    case GVOX_SEEK_ORIGIN_CUR: seekdir = std::ios::cur; break;
    case GVOX_SEEK_ORIGIN_END: seekdir = std::ios::end; break;
    default: break;
    }
    file_handle.seekg(offset, seekdir);
    return GVOX_SUCCESS;
}

auto GvoxFileInputAdapter::tell() -> long {
    return static_cast<long>(file_handle.tellg());
}

auto gvox_input_adapter_file_create(void **self, GvoxInputAdapterCreateCbArgs const *args) -> GvoxResult {
    if (args->config == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    auto *result = new GvoxFileInputAdapter(*static_cast<GvoxFileInputAdapterConfig const *>(args->config));
    if (!result->file_handle.is_open()) {
        return GVOX_ERROR_UNKNOWN;
    }
    *self = result;
    return GVOX_SUCCESS;
}
auto gvox_input_adapter_file_read(void *self, GvoxInputAdapter, uint8_t *data, size_t size) -> GvoxResult {
    return static_cast<GvoxFileInputAdapter *>(self)->read(data, size);
}
auto gvox_input_adapter_file_seek(void *self, GvoxInputAdapter, long offset, GvoxSeekOrigin origin) -> GvoxResult {
    return static_cast<GvoxFileInputAdapter *>(self)->seek(offset, origin);
}
auto gvox_input_adapter_file_tell(void *self, GvoxInputAdapter) -> long {
    return static_cast<GvoxFileInputAdapter *>(self)->tell();
}
void gvox_input_adapter_file_destroy(void *self) {
    delete static_cast<GvoxFileInputAdapter *>(self);
}
