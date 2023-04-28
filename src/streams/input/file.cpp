#include <gvox/gvox.h>
#include <gvox/streams/input/file.h>

#include <fstream>

struct GvoxFileInputStream {
    GvoxFileInputStreamConfig config{};
    std::ifstream file_handle;

    explicit GvoxFileInputStream(GvoxFileInputStreamConfig const &a_config);

    auto read(uint8_t *data, size_t size) -> GvoxResult;
    auto seek(long offset, GvoxSeekOrigin origin) -> GvoxResult;
};

GvoxFileInputStream::GvoxFileInputStream(GvoxFileInputStreamConfig const &a_config) : config{a_config} {
    file_handle = std::ifstream{config.filepath, std::ios::binary};
}

auto GvoxFileInputStream::read(uint8_t *data, size_t size) -> GvoxResult {
    file_handle.read(reinterpret_cast<char *>(data), static_cast<std::streamsize>(size));
    return GVOX_SUCCESS;
}

auto GvoxFileInputStream::seek(long offset, GvoxSeekOrigin origin) -> GvoxResult {
    auto seekdir = std::ios_base::seekdir{};
    switch (origin) {
    case GVOX_SEEK_ORIGIN_BEG: seekdir = std::ios::beg; break;
    case GVOX_SEEK_ORIGIN_END: seekdir = std::ios::end; break;
    case GVOX_SEEK_ORIGIN_CUR: seekdir = std::ios::cur; break;
    default: break;
    }
    file_handle.seekg(offset, seekdir);
    return GVOX_SUCCESS;
}

auto gvox_input_stream_file_create(void **self, GvoxInputStreamCreateCbArgs const *args) -> GvoxResult {
    if (args->config == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    auto *result = new GvoxFileInputStream(*static_cast<GvoxFileInputStreamConfig const *>(args->config));
    if (!result->file_handle.is_open()) {
        return GVOX_ERROR_UNKNOWN;
    }
    *self = result;
    return GVOX_SUCCESS;
}

auto gvox_input_stream_file_read(void *self, uint8_t *data, size_t size) -> GvoxResult {
    return static_cast<GvoxFileInputStream *>(self)->read(data, size);
}

auto gvox_input_stream_file_seek(void *self, long offset, GvoxSeekOrigin origin) -> GvoxResult {
    return static_cast<GvoxFileInputStream *>(self)->seek(offset, origin);
}

void gvox_input_stream_file_destroy(void *self) {
    delete static_cast<GvoxFileInputStream *>(self);
}
