#include <gvox/adapter.h>
#include <gvox/adapters/output/file.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <filesystem>

using UserState = struct {
    std::filesystem::path path;
    std::vector<uint8_t> bytes;
};

extern "C" void gvox_output_adapter_file_begin(GVoxAdapterContext *ctx, void *config) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_adapter_malloc(ctx, sizeof(UserState)));
    gvox_output_adapter_set_user_pointer(ctx, &user_state);
    memset(&user_state, 0, sizeof(UserState));
    if (config != nullptr) {
        auto *user_config = reinterpret_cast<GVoxFileOutputAdapterConfig *>(config);
        user_state.path = user_config->filepath;
    } else {
        user_state.path = "gvox_file_out.bin";
    }
}

extern "C" void gvox_output_adapter_file_end(GVoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_output_adapter_get_user_pointer(ctx));
    FILE *o_file = nullptr;
    fopen_s(&o_file, user_state.path.string().c_str(), "wb");
    fwrite(user_state.bytes.data(), 1, user_state.bytes.size(), o_file);
    fclose(o_file);
    user_state.bytes.~vector();
    user_state.path.~path();
}

extern "C" void gvox_output_adapter_file_write(GVoxAdapterContext *ctx, size_t position, size_t size, void const *data) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_output_adapter_get_user_pointer(ctx));
    if (position + size > user_state.bytes.size()) {
        user_state.bytes.resize(position + size);
    }
    memcpy(user_state.bytes.data() + position, data, size);
}

extern "C" void gvox_output_adapter_file_reserve(GVoxAdapterContext *ctx, size_t size) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_output_adapter_get_user_pointer(ctx));
    if (size > user_state.bytes.size()) {
        user_state.bytes.resize(size);
    }
}
