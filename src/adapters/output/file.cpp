#include <gvox/gvox.h>
#include <gvox/adapters/output/file.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <filesystem>
#include <new>

struct OutputFileUserState {
    std::filesystem::path path{};
    std::vector<uint8_t> bytes{};
};

extern "C" void gvox_output_adapter_file_begin(GvoxAdapterContext *ctx, void *config) {
    auto *user_state_ptr = malloc(sizeof(OutputFileUserState));
    auto &user_state = *(new (user_state_ptr) OutputFileUserState());
    gvox_output_adapter_set_user_pointer(ctx, user_state_ptr);

    if (config != nullptr) {
        auto *user_config = reinterpret_cast<GvoxFileOutputAdapterConfig *>(config);
        user_state.path = user_config->filepath;
    } else {
        user_state.path = "gvox_file_out.bin";
    }
}

extern "C" void gvox_output_adapter_file_end(GvoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<OutputFileUserState *>(gvox_output_adapter_get_user_pointer(ctx));
    FILE *o_file = nullptr;
    fopen_s(&o_file, user_state.path.string().c_str(), "wb");
    fwrite(user_state.bytes.data(), 1, user_state.bytes.size(), o_file);
    fclose(o_file);
    user_state.~OutputFileUserState();
    free(&user_state);
}

extern "C" void gvox_output_adapter_file_reserve(GvoxAdapterContext *ctx, size_t size) {
    auto &user_state = *reinterpret_cast<OutputFileUserState *>(gvox_output_adapter_get_user_pointer(ctx));
    if (size > user_state.bytes.size()) {
        user_state.bytes.resize(size);
    }
}

extern "C" void gvox_output_adapter_file_write(GvoxAdapterContext *ctx, size_t position, size_t size, void const *data) {
    auto &user_state = *reinterpret_cast<OutputFileUserState *>(gvox_output_adapter_get_user_pointer(ctx));
    gvox_output_adapter_file_reserve(ctx, position + size);
    memcpy(user_state.bytes.data() + position, data, size);
}
