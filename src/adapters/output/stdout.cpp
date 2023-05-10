#include <gvox/gvox.h>
#include <gvox/adapters/output/stdout.h>

#include <iostream>
#include <string_view>
#include <cassert>

struct GvoxStdoutOutputAdapter {
    explicit GvoxStdoutOutputAdapter(GvoxStdoutOutputAdapterConfig const &config);

    static auto write(uint8_t *data, size_t size) -> GvoxResult;
};

GvoxStdoutOutputAdapter::GvoxStdoutOutputAdapter(GvoxStdoutOutputAdapterConfig const & /*unused*/) {
}

auto GvoxStdoutOutputAdapter::write(uint8_t *data, size_t size) -> GvoxResult {
    auto *chars = reinterpret_cast<char *>(data);
    std::cout << std::string_view{chars, chars + size};
    return GVOX_SUCCESS;
}

auto gvox_output_adapter_stdout_create(void **self, GvoxOutputAdapterCreateCbArgs const *args) -> GvoxResult {
    GvoxStdoutOutputAdapterConfig config;
    if (args->config != nullptr) {
        config = *static_cast<GvoxStdoutOutputAdapterConfig const *>(args->config);
    } else {
        config = {};
    }
    *self = new GvoxStdoutOutputAdapter(config);
    return GVOX_SUCCESS;
}
auto gvox_output_adapter_stdout_write(void *self, GvoxOutputAdapter /*unused*/, uint8_t *data, size_t size) -> GvoxResult {
    return static_cast<GvoxStdoutOutputAdapter *>(self)->write(data, size);
}
auto gvox_output_adapter_stdout_seek(void * /*unused*/, GvoxOutputAdapter /*unused*/, long /*unused*/, GvoxSeekOrigin /*unused*/) -> GvoxResult {
    assert(false && "Why are you trying to seek in stdout?!");
    return GVOX_ERROR_UNKNOWN;
}
auto gvox_output_adapter_stdout_tell(void * /*unused*/, GvoxOutputAdapter /*unused*/) -> long {
    assert(false && "Why are you trying to seek in stdout?!");
    return GVOX_ERROR_UNKNOWN;
}
void gvox_output_adapter_stdout_destroy(void *self) {
    std::cout << std::flush;
    delete static_cast<GvoxStdoutOutputAdapter *>(self);
}
