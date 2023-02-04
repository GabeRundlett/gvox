#include <gvox/gvox.h>
#include <gvox/adapters/input/byte_buffer.h>

#include <cstdlib>
#include <cstring>

#include <vector>
#include <new>

struct ByteBufferInputUserState {
    std::vector<uint8_t> bytes = {};
};

extern "C" void gvox_input_adapter_byte_buffer_begin(GvoxAdapterContext *ctx, void *config) {
    auto *user_state_ptr = malloc(sizeof(ByteBufferInputUserState));
    auto &user_state = *(new (user_state_ptr) ByteBufferInputUserState());
    gvox_input_adapter_set_user_pointer(ctx, user_state_ptr);

    auto &user_config = *reinterpret_cast<GvoxByteBufferInputAdapterConfig *>(config);
    user_state.bytes.resize(user_config.size);
    memcpy(user_state.bytes.data(), user_config.data, user_config.size);
}

extern "C" void gvox_input_adapter_byte_buffer_end([[maybe_unused]] GvoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<ByteBufferInputUserState *>(gvox_input_adapter_get_user_pointer(ctx));
    user_state.~ByteBufferInputUserState();
    free(&user_state);
}

extern "C" void gvox_input_adapter_byte_buffer_read(GvoxAdapterContext *ctx, size_t position, size_t size, void *data) {
    auto &user_state = *reinterpret_cast<ByteBufferInputUserState *>(gvox_input_adapter_get_user_pointer(ctx));
    if (position + size > user_state.bytes.size()) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_INPUT_ADAPTER, "Tried reading past the end of the provided input buffer");
    }
    memcpy(data, user_state.bytes.data() + position, size);
}
