#include <gvox/gvox.h>
#include <gvox/adapters/input/byte_buffer.h>

#include <cstdlib>
#include <cstring>

#include <vector>
#include <new>
#include <algorithm>

struct ByteBufferInputUserState {
    std::vector<uint8_t> bytes{};
};

extern "C" void gvox_input_adapter_byte_buffer_create(GvoxAdapterContext *ctx, void const *config) {
    auto *user_state_ptr = malloc(sizeof(ByteBufferInputUserState));
    auto &user_state = *(new (user_state_ptr) ByteBufferInputUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
    auto &user_config = *static_cast<GvoxByteBufferInputAdapterConfig const *>(config);
    user_state.bytes.resize(user_config.size);
    std::copy(user_config.data, user_config.data + user_config.size, user_state.bytes.begin());
}

extern "C" void gvox_input_adapter_byte_buffer_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<ByteBufferInputUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~ByteBufferInputUserState();
    free(&user_state);
}

extern "C" void gvox_input_adapter_byte_buffer_blit_begin(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

extern "C" void gvox_input_adapter_byte_buffer_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

extern "C" void gvox_input_adapter_byte_buffer_read(GvoxAdapterContext *ctx, size_t position, size_t size, void *data) {
    auto &user_state = *static_cast<ByteBufferInputUserState *>(gvox_adapter_get_user_pointer(ctx));
    if (position + size > user_state.bytes.size()) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_INPUT_ADAPTER, "Tried reading past the end of the provided input buffer");
    }
    std::copy(user_state.bytes.data() + position, user_state.bytes.data() + position + size, static_cast<uint8_t *>(data));
}
