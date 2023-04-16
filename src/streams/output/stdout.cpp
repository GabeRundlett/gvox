#include <gvox/gvox.h>
#include <gvox/streams/output/stdout.h>

#include <iostream>
#include <string_view>
#include <cassert>

struct GvoxStdoutOutputStream {
    GvoxStdoutOutputStream(GvoxStdoutOutputStreamConfig const &config);

    auto write(uint8_t *data, size_t size) -> GvoxResult;
};

GvoxStdoutOutputStream::GvoxStdoutOutputStream(GvoxStdoutOutputStreamConfig const &) {
}

auto GvoxStdoutOutputStream::write(uint8_t *data, size_t size) -> GvoxResult {
    auto chars = reinterpret_cast<char *>(data);
    std::cout << std::string_view{chars, chars + size};
    return GVOX_SUCCESS;
}

GvoxResult gvox_output_stream_stdout_create(void **self, void const *config_ptr) {
    GvoxStdoutOutputStreamConfig config;
    if (config_ptr) {
        config = *static_cast<GvoxStdoutOutputStreamConfig const *>(config_ptr);
    } else {
        config = {};
    }
    *self = new GvoxStdoutOutputStream(config);
    return GVOX_SUCCESS;
}

GvoxResult gvox_output_stream_stdout_write(void *self, uint8_t *data, size_t size) {
    return static_cast<GvoxStdoutOutputStream *>(self)->write(data, size);
}

GvoxResult gvox_output_stream_stdout_seek(void *, long, GvoxSeekOrigin) {
    assert(false && "Bruh");
    return GVOX_ERROR_UNKNOWN;
}

void gvox_output_stream_stdout_destroy(void *self) {
    std::cout << std::flush;
    delete static_cast<GvoxStdoutOutputStream *>(self);
}
