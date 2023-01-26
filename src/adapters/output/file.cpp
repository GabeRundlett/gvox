#include <gvox/adapter.h>
#include <gvox/adapters/output/file.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <filesystem>

using FileState = struct {
    size_t size;
    size_t buffer_size;
    void *buffer;
    std::filesystem::path path;
};

extern "C" void gvox_output_adapter_file_begin(GVoxAdapterContext *ctx, void *config) {
    auto &user_state = *reinterpret_cast<FileState *>(gvox_output_adapter_malloc(ctx, sizeof(FileState)));
    gvox_output_adapter_set_user_pointer(ctx, &user_state);
    memset(&user_state, 0, sizeof(FileState));
    if (config != nullptr) {
        auto *file_config = reinterpret_cast<GVoxFileOutputAdapterConfig *>(config);
        user_state.path = file_config->filepath;
    } else {
        user_state.path = "gvox_file_out.bin";
    }
}

extern "C" void gvox_output_adapter_file_end(GVoxAdapterContext *ctx) {
    auto *user_state = reinterpret_cast<FileState *>(gvox_output_adapter_get_user_pointer(ctx));
    FILE *o_file = nullptr;
    fopen_s(&o_file, user_state->path.string().c_str(), "wb");
    fwrite(user_state->buffer, 1, user_state->size, o_file);
    fclose(o_file);
    free(user_state->buffer);
}

extern "C" void gvox_output_adapter_file_write(GVoxAdapterContext *ctx, size_t position, size_t size, void const *data) {
    auto *user_state = reinterpret_cast<FileState *>(gvox_output_adapter_get_user_pointer(ctx));
    if (position + size > user_state->buffer_size) {
        user_state->buffer_size = (position + size) * 2;
        user_state->buffer = realloc(user_state->buffer, user_state->buffer_size);
    }
    user_state->size = std::max(user_state->size, position + size);
    memcpy((uint8_t *)(user_state->buffer) + position, data, size);
}

extern "C" void gvox_output_adapter_file_reserve(GVoxAdapterContext *ctx, size_t size) {
    auto *user_state = reinterpret_cast<FileState *>(gvox_output_adapter_get_user_pointer(ctx));
    if (size > user_state->buffer_size) {
        user_state->buffer_size = size;
        user_state->buffer = realloc(user_state->buffer, user_state->buffer_size);
    }
}
