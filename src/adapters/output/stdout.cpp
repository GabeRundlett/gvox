#include <gvox/gvox.h>
#include <gvox/adapters/output/stdout.h>

#include <iostream>

extern "C" void gvox_output_adapter_stdout_create(GvoxAdapterContext *, void *) {
}

extern "C" void gvox_output_adapter_stdout_destroy(GvoxAdapterContext *) {
}

extern "C" void gvox_output_adapter_stdout_blit_begin(GvoxBlitContext *, GvoxAdapterContext *) {
}

extern "C" void gvox_output_adapter_stdout_blit_end(GvoxBlitContext *, GvoxAdapterContext *) {
}

extern "C" void gvox_output_adapter_stdout_write(GvoxAdapterContext *, size_t, size_t size, void const *data) {
    auto str = std::string_view{static_cast<char const *>(data), size};
    std::cout << str;
}

extern "C" void gvox_output_adapter_stdout_reserve(GvoxAdapterContext *, size_t) {
}
