#include <gvox/stream.h>
#include <gvox/streams/output/stdout.h>

#include <iostream>
#include <string_view>

struct GvoxStdoutOutputStream {
    explicit GvoxStdoutOutputStream(GvoxStdoutOutputStreamConfig const &config);

    static auto write(void *data, size_t size) -> GvoxResult;
};

GvoxStdoutOutputStream::GvoxStdoutOutputStream(GvoxStdoutOutputStreamConfig const & /*unused*/) {
}

auto GvoxStdoutOutputStream::write(void *data, size_t size) -> GvoxResult {
    auto *chars = reinterpret_cast<char *>(data);
    std::cout << std::string_view{chars, chars + size};
    return GVOX_SUCCESS;
}

auto gvox_output_stream_stdout_description() GVOX_FUNC_ATTRIB->GvoxOutputStreamDescription {
    return GvoxOutputStreamDescription{
        .create = [](void **self, GvoxOutputStreamCreateCbArgs const *args) -> GvoxResult {
            GvoxStdoutOutputStreamConfig config;
            if (args->config != nullptr) {
                config = *static_cast<GvoxStdoutOutputStreamConfig const *>(args->config);
            } else {
                config = {};
            }
            *self = new GvoxStdoutOutputStream(config);
            return GVOX_SUCCESS;
        },
        .write = [](void *self, GvoxOutputStream /*unused*/, void *data, size_t size) -> GvoxResult {
            return static_cast<GvoxStdoutOutputStream *>(self)->write(data, size);
        },
        .seek = [](void * /*unused*/, GvoxOutputStream /*unused*/, int64_t /*unused*/, GvoxSeekOrigin /*unused*/) -> GvoxResult {
            return GVOX_ERROR_UNKNOWN;
        },
        .tell = [](void * /*unused*/, GvoxOutputStream /*unused*/) -> int64_t {
            return GVOX_ERROR_UNKNOWN;
        },
        .destroy = [](void *self) { delete static_cast<GvoxStdoutOutputStream *>(self); },
    };
}
