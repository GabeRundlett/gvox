#include <gvox/gvox.h>
#include <gvox/adapters/output/stdout.h>

#include <iostream>

// Base
extern "C" void gvox_output_adapter_stdout_create(GvoxAdapterContext * /*unused*/, void const * /*unused*/) {
}

extern "C" void gvox_output_adapter_stdout_destroy(GvoxAdapterContext * /*unused*/) {
}

extern "C" void gvox_output_adapter_stdout_blit_begin(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

extern "C" void gvox_output_adapter_stdout_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

// General
extern "C" void gvox_output_adapter_stdout_write(GvoxAdapterContext * /*unused*/, size_t /*unused*/, size_t size, void const *data) {
    auto str = std::string_view{static_cast<char const *>(data), size};
    std::cout << str;
}

extern "C" void gvox_output_adapter_stdout_reserve(GvoxAdapterContext * /*unused*/, size_t /*unused*/) {
}
