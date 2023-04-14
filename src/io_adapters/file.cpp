#include <gvox/gvox.h>
#include <gvox/io_adapters/file.h>

#include <variant>
#include <fstream>

struct GvoxFileIoAdapter {
    GvoxFileIoAdapterConfig config{};
    std::variant<std::ifstream, std::ofstream> file_handle;

    GvoxFileIoAdapter(GvoxFileIoAdapterConfig const &a_config);

    auto input_read(uint8_t *data, size_t size) -> GvoxResult;
    auto input_seek(long offset, GvoxSeekOrigin origin) -> GvoxResult;
};

GvoxFileIoAdapter::GvoxFileIoAdapter(GvoxFileIoAdapterConfig const &a_config) : config{a_config} {
    if (config.open_mode == GVOX_FILE_IO_ADAPTER_OPEN_MODE_INPUT) {
        file_handle = std::ifstream{config.filepath, std::ios::binary};
    } else {
        file_handle = std::ofstream{config.filepath, std::ios::binary};
    }
}

auto GvoxFileIoAdapter::input_read(uint8_t *data, size_t size) -> GvoxResult {
    if (!std::holds_alternative<std::ifstream>(file_handle)) {
        return GVOX_ERROR_UNKNOWN;
    }
    std::get<std::ifstream>(file_handle).read(reinterpret_cast<char *>(data), static_cast<std::streamsize>(size));
    return GVOX_SUCCESS;
}

auto GvoxFileIoAdapter::input_seek(long offset, GvoxSeekOrigin origin) -> GvoxResult {
    if (!std::holds_alternative<std::ifstream>(file_handle)) {
        return GVOX_ERROR_UNKNOWN;
    }
    auto seekdir = std::ios_base::seekdir{};
    switch (origin) {
    case GVOX_SEEK_ORIGIN_BEG: seekdir = std::ios::beg; break;
    case GVOX_SEEK_ORIGIN_END: seekdir = std::ios::end; break;
    case GVOX_SEEK_ORIGIN_CUR: seekdir = std::ios::cur; break;
    default: break;
    }
    std::get<std::ifstream>(file_handle).seekg(offset, seekdir);
    return GVOX_SUCCESS;
}

GvoxResult gvox_io_adapter_file_create(void **self, void const *config_ptr) {
    if (!config_ptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    auto *result = new GvoxFileIoAdapter(*static_cast<GvoxFileIoAdapterConfig const *>(config_ptr));
    if (auto *i_file = std::get_if<std::ifstream>(&result->file_handle)) {
        if (!i_file->is_open()) {
            return GVOX_ERROR_UNKNOWN;
        }
    }
    if (auto *o_file = std::get_if<std::ofstream>(&result->file_handle)) {
        if (!o_file->is_open()) {
            return GVOX_ERROR_UNKNOWN;
        }
    }
    *self = result;
    return GVOX_SUCCESS;
}

GvoxResult gvox_io_adapter_file_input_read(void *self, uint8_t *data, size_t size) {
    return static_cast<GvoxFileIoAdapter *>(self)->input_read(data, size);
}

GvoxResult gvox_io_adapter_file_input_seek(void *self, long offset, GvoxSeekOrigin origin) {
    return static_cast<GvoxFileIoAdapter *>(self)->input_seek(offset, origin);
}

void gvox_io_adapter_file_destroy(void *self) {
    delete static_cast<GvoxFileIoAdapter *>(self);
}
