#define _CRT_SECURE_NO_WARNINGS

#include <gvox/gvox.h>
#include <gvox/streams/input/file.h>

#include <cstdio>
#include <cstddef>
#include <cstdint>
#include <new>

struct GvoxFileInputStream {
    GvoxFileInputStreamConfig config{};
    // std::ifstream file_handle;
    FILE *file_handle{};

    explicit GvoxFileInputStream(GvoxFileInputStreamConfig const &a_config);

    auto read(void *data, size_t size) const -> GvoxResult;
    [[nodiscard]] auto seek(int64_t offset, GvoxSeekOrigin origin) const -> GvoxResult;
    [[nodiscard]] auto tell() const -> int64_t;
};

GvoxFileInputStream::GvoxFileInputStream(GvoxFileInputStreamConfig const &a_config)
    : config{a_config},
      // file_handle{std::ifstream{config.filepath, std::ios::binary}} {
      file_handle{fopen(config.filepath, "rb")} {
}

auto GvoxFileInputStream::read(void *data, size_t size) const -> GvoxResult {
    // file_handle.read(reinterpret_cast<char *>(data), static_cast<std::streamsize>(size));
    auto size_read = fread(data, size, 1, file_handle);
    if (size_read != size) {
        return GVOX_ERROR_UNKNOWN;
    }
    return GVOX_SUCCESS;
}

auto GvoxFileInputStream::seek(int64_t offset, GvoxSeekOrigin origin) const -> GvoxResult {
    // auto seekdir = std::ios_base::seekdir{};
    // switch (origin) {
    // case GVOX_SEEK_ORIGIN_BEG: seekdir = std::ios::beg; break;
    // case GVOX_SEEK_ORIGIN_CUR: seekdir = std::ios::cur; break;
    // case GVOX_SEEK_ORIGIN_END: seekdir = std::ios::end; break;
    // default: break;
    // }
    // file_handle.seekg(offset, seekdir);
    auto seekdir = SEEK_CUR;
    switch (origin) {
    case GVOX_SEEK_ORIGIN_BEG: seekdir = SEEK_SET; break;
    case GVOX_SEEK_ORIGIN_CUR: seekdir = SEEK_CUR; break;
    case GVOX_SEEK_ORIGIN_END: seekdir = SEEK_END; break;
    default: break;
    }
    auto ret = fseek(file_handle, static_cast<long>(offset), seekdir);
    if (ret != 0) {
        return GVOX_ERROR_UNKNOWN;
    }
    return GVOX_SUCCESS;
}

auto GvoxFileInputStream::tell() const -> int64_t {
    return ftell(file_handle);
    // return static_cast<int64_t>(file_handle.tellg());
}

auto gvox_input_stream_file_description() GVOX_FUNC_ATTRIB->GvoxInputStreamDescription {
    return GvoxInputStreamDescription{
        .create = [](void **self, GvoxInputStreamCreateCbArgs const *args) -> GvoxResult {
            if (args->config == nullptr) {
                return GVOX_ERROR_INVALID_ARGUMENT;
            }
            auto *result = new (std::nothrow) GvoxFileInputStream(*static_cast<GvoxFileInputStreamConfig const *>(args->config));
            // if (!result->file_handle.is_open()) {
            //     return GVOX_ERROR_UNKNOWN;
            // }
            if (result->file_handle != nullptr) {
                delete result;
                return GVOX_ERROR_UNKNOWN;
            }
            *self = result;
            return GVOX_SUCCESS;
        },
        .read = [](void *self, GvoxInputStream, void *data, size_t size) -> GvoxResult {
            return static_cast<GvoxFileInputStream *>(self)->read(data, size);
        },
        .seek = [](void *self, GvoxInputStream, int64_t offset, GvoxSeekOrigin origin) -> GvoxResult {
            return static_cast<GvoxFileInputStream *>(self)->seek(offset, origin);
        },
        .tell = [](void *self, GvoxInputStream) -> int64_t {
            return static_cast<GvoxFileInputStream *>(self)->tell();
        },
        .destroy = [](void *self) { delete static_cast<GvoxFileInputStream *>(self); },
    };
}
