#include <gvox/gvox.h>
#include <gvox/adapters/output/stdout.h>

#include <iostream>

extern "C" void gvox_output_adapter_stdout_create([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] void *config) {
}

extern "C" void gvox_output_adapter_stdout_destroy([[maybe_unused]] GvoxAdapterContext *ctx) {
}

extern "C" void gvox_output_adapter_stdout_blit_begin([[maybe_unused]] GvoxBlitContext *blit_ctx, [[maybe_unused]] GvoxAdapterContext *ctx) {
}

extern "C" void gvox_output_adapter_stdout_blit_end([[maybe_unused]] GvoxBlitContext *blit_ctx, [[maybe_unused]] GvoxAdapterContext *ctx) {
}

extern "C" void gvox_output_adapter_stdout_write([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] size_t position, size_t size, void const *data) {
    auto str = std::string_view{reinterpret_cast<char const *>(data), size};
    std::cout << str;
}

extern "C" void gvox_output_adapter_stdout_reserve([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] size_t size) {
}
