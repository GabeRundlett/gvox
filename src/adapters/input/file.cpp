#include <gvox/gvox.h>
#include <gvox/adapters/input/file.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <new>

struct FileInputUserState {
    FILE *file;
    size_t byte_offset;
};

extern "C" void gvox_input_adapter_file_begin(GvoxAdapterContext *ctx, void *config) {
    auto *user_state_ptr = gvox_adapter_malloc(ctx, sizeof(FileInputUserState));
    auto &user_state = *(new (user_state_ptr) FileInputUserState());
    gvox_input_adapter_set_user_pointer(ctx, user_state_ptr);

    auto &user_config = *reinterpret_cast<GvoxFileInputAdapterConfig *>(config);
    user_state.byte_offset = user_config.byte_offset;
    fopen_s(&user_state.file, user_config.filepath, "rb");
}

extern "C" void gvox_input_adapter_file_end(GvoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<FileInputUserState *>(gvox_input_adapter_get_user_pointer(ctx));
    fclose(user_state.file);
    user_state.~FileInputUserState();
}

extern "C" void gvox_input_adapter_file_read(GvoxAdapterContext *ctx, size_t position, size_t size, void *data) {
    auto &user_state = *reinterpret_cast<FileInputUserState *>(gvox_input_adapter_get_user_pointer(ctx));
    auto ret = fseek(user_state.file, static_cast<long>(position + user_state.byte_offset), SEEK_SET);
    if (ret != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_INPUT_ADAPTER, "Failed to set the file read head");
        return;
    }
    [[maybe_unused]] size_t const byte_n = fread(data, 1, size, user_state.file);
}
