#include <gvox/gvox.h>
#include <gvox/adapters/output/file.h>

#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>
#include <new>

struct OutputFileUserState {
    std::filesystem::path path{};
    std::vector<uint8_t> bytes{};
};

extern "C" void gvox_output_adapter_file_create(GvoxAdapterContext *ctx, void *config) {
    auto *user_state_ptr = malloc(sizeof(OutputFileUserState));
    auto &user_state = *(new (user_state_ptr) OutputFileUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
    if (config != nullptr) {
        auto *user_config = static_cast<GvoxFileOutputAdapterConfig *>(config);
        user_state.path = user_config->filepath;
    } else {
        user_state.path = "gvox_file_out.bin";
    }
}

extern "C" void gvox_output_adapter_file_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<OutputFileUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~OutputFileUserState();
    free(&user_state);
}

extern "C" void gvox_output_adapter_file_blit_begin(GvoxBlitContext *, GvoxAdapterContext *, void *) {
}

extern "C" void gvox_output_adapter_file_blit_end(GvoxBlitContext *, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<OutputFileUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto file = std::ofstream(user_state.path, std::ios_base::binary);
    file.write(reinterpret_cast<char const *>(user_state.bytes.data()), static_cast<std::streamsize>(user_state.bytes.size()));
}

extern "C" void gvox_output_adapter_file_reserve(GvoxAdapterContext *ctx, size_t size) {
    auto &user_state = *static_cast<OutputFileUserState *>(gvox_adapter_get_user_pointer(ctx));
    if (size > user_state.bytes.size()) {
        user_state.bytes.resize(size);
    }
}

extern "C" void gvox_output_adapter_file_write(GvoxAdapterContext *ctx, size_t position, size_t size, void const *data) {
    auto &user_state = *static_cast<OutputFileUserState *>(gvox_adapter_get_user_pointer(ctx));
    gvox_output_adapter_file_reserve(ctx, position + size);
    auto bytes = static_cast<uint8_t const *>(data);
    std::copy(bytes, bytes + size, user_state.bytes.data() + position);
}
