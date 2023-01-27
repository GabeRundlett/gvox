#include <gvox/adapter.h>
#include <gvox/adapters/input/byte_buffer.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using UserState = struct {
    std::vector<uint8_t> bytes;
};

extern "C" void gvox_input_adapter_byte_buffer_begin(GVoxAdapterContext *ctx, void *config) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_adapter_malloc(ctx, sizeof(UserState)));
    gvox_input_adapter_set_user_pointer(ctx, &user_state);
    memset(&user_state, 0, sizeof(UserState));
    auto &user_config = *reinterpret_cast<GVoxByteBufferInputAdapterConfig *>(config);
    user_state.bytes.resize(user_config.size);
    memcpy(user_state.bytes.data(), user_config.data, user_config.size);
}

extern "C" void gvox_input_adapter_byte_buffer_end([[maybe_unused]] GVoxAdapterContext *ctx) {
}

extern "C" void gvox_input_adapter_byte_buffer_read(GVoxAdapterContext *ctx, size_t position, size_t size, void *data) {
    auto &user_state = *reinterpret_cast<UserState *>(gvox_input_adapter_get_user_pointer(ctx));
    if (position + size > user_state.bytes.size()) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_INPUT_ADAPTER, "Tried reading past the end of the provided input buffer");
    }
    memcpy(data, user_state.bytes.data() + position, size);
}
