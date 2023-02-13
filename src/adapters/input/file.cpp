#include <gvox/gvox.h>
#include <gvox/adapters/input/file.h>

#include <cstdlib>
#include <cstring>

#include <filesystem>
#include <fstream>

#include <new>

#if GVOX_ENABLE_THREADSAFETY
#include <mutex>
#endif

struct FileInputUserState {
    std::filesystem::path path{};
    std::ifstream file{};
    size_t byte_offset{};
#if GVOX_ENABLE_THREADSAFETY
    std::mutex mtx{};
#endif
};

extern "C" void gvox_input_adapter_file_create(GvoxAdapterContext *ctx, void *config) {
    auto *user_state_ptr = malloc(sizeof(FileInputUserState));
    auto &user_state = *(new (user_state_ptr) FileInputUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
    auto &user_config = *static_cast<GvoxFileInputAdapterConfig *>(config);
    user_state.byte_offset = user_config.byte_offset;
    user_state.path = user_config.filepath;
}

extern "C" void gvox_input_adapter_file_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<FileInputUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~FileInputUserState();
    free(&user_state);
}

extern "C" void gvox_input_adapter_file_blit_begin(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<FileInputUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.file.open(user_state.path, std::ios::binary);
}

extern "C" void gvox_input_adapter_file_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<FileInputUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.file.close();
}

extern "C" void gvox_input_adapter_file_read(GvoxAdapterContext *ctx, size_t position, size_t size, void *data) {
    auto &user_state = *static_cast<FileInputUserState *>(gvox_adapter_get_user_pointer(ctx));
#if GVOX_ENABLE_THREADSAFETY
    auto lock = std::lock_guard{user_state.mtx};
#endif
    user_state.file.seekg(static_cast<std::streamoff>(position), std::ios_base::beg);
    user_state.file.read(static_cast<char *>(data), static_cast<std::streamsize>(size));
}
