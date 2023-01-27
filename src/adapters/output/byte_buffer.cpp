#include <gvox/adapter.h>
#include <gvox/adapters/output/byte_buffer.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <filesystem>

using UserState = struct {
    GVoxByteBufferOutputAdapterConfig config;
    std::vector<uint8_t> bytes;
};

extern "C" void gvox_output_adapter_byte_buffer_begin(GVoxAdapterContext *ctx, void *config) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_adapter_malloc(ctx, sizeof(UserState)));
    gvox_output_adapter_set_user_pointer(ctx, &user_state);
    memset(&user_state, 0, sizeof(UserState));
    if (config != nullptr) {
        user_state.config = *reinterpret_cast<GVoxByteBufferOutputAdapterConfig *>(config);
    } else {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_OUTPUT_ADAPTER, "Can't use this 'bytes' output adapter without a config");
    }
}

extern "C" void gvox_output_adapter_byte_buffer_end(GVoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_output_adapter_get_user_pointer(ctx));
    void *bytes = nullptr;
    if (user_state.config.allocate != nullptr) {
        bytes = user_state.config.allocate(user_state.bytes.size());
    } else {
        bytes = malloc(user_state.bytes.size());
    }
    memcpy(bytes, user_state.bytes.data(), user_state.bytes.size());
    *user_state.config.out_byte_buffer_ptr = static_cast<uint8_t *>(bytes);
    *user_state.config.out_size = user_state.bytes.size();
    user_state.bytes.~vector();
}

extern "C" void gvox_output_adapter_byte_buffer_write(GVoxAdapterContext *ctx, size_t position, size_t size, void const *data) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_output_adapter_get_user_pointer(ctx));
    if (position + size > user_state.bytes.size()) {
        user_state.bytes.resize(position + size);
    }
    memcpy(user_state.bytes.data() + position, data, size);
}

extern "C" void gvox_output_adapter_byte_buffer_reserve(GVoxAdapterContext *ctx, size_t size) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_output_adapter_get_user_pointer(ctx));
    if (size > user_state.bytes.size()) {
        user_state.bytes.resize(size);
    }
}
