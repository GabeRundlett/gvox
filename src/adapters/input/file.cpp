#include <gvox/adapter.h>
#include <gvox/adapters/input/file.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

using UserState = struct {
    FILE *file;
};

extern "C" void gvox_input_adapter_file_begin(GVoxAdapterContext *ctx, void *config) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_adapter_malloc(ctx, sizeof(UserState)));
    gvox_input_adapter_set_user_pointer(ctx, &user_state);
    auto &user_config = *reinterpret_cast<GVoxFileInputAdapterConfig *>(config);
    fopen_s(&user_state.file, user_config.filepath, "rb");
}

extern "C" void gvox_input_adapter_file_end(GVoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_input_adapter_get_user_pointer(ctx));
    fclose(user_state.file);
}

extern "C" void gvox_input_adapter_file_read(GVoxAdapterContext *ctx, size_t position, size_t size, void *data) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_input_adapter_get_user_pointer(ctx));
    auto ret = fseek(user_state.file, static_cast<long>(position), SEEK_SET);
    if (ret != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_INPUT_ADAPTER, "Failed to set the file read head");
        return;
    }
    [[maybe_unused]] size_t const byte_n = fread(data, 1, size, user_state.file);
}
