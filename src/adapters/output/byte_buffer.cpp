#include <gvox/gvox.h>
#include <gvox/adapters/output/byte_buffer.h>

#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <vector>
#include <new>

struct ByteBufferOutputUserState {
    GvoxByteBufferOutputAdapterConfig config{};
    std::vector<uint8_t> bytes{};
};

// Base
extern "C" void gvox_output_adapter_byte_buffer_create(GvoxAdapterContext *ctx, void const *config) {
    auto *user_state_ptr = malloc(sizeof(ByteBufferOutputUserState));
    auto &user_state = *(new (user_state_ptr) ByteBufferOutputUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
    if (config != nullptr) {
        user_state.config = *static_cast<GvoxByteBufferOutputAdapterConfig const *>(config);
    } else {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_OUTPUT_ADAPTER, "Can't use this 'bytes' output adapter without a config");
    }
}

extern "C" void gvox_output_adapter_byte_buffer_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<ByteBufferOutputUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~ByteBufferOutputUserState();
    free(&user_state);
}

extern "C" void gvox_output_adapter_byte_buffer_blit_begin(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

extern "C" void gvox_output_adapter_byte_buffer_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<ByteBufferOutputUserState *>(gvox_adapter_get_user_pointer(ctx));
    void *bytes = nullptr;
    // NOTE: This needs to be manually cleaned up by the user!
    if (user_state.config.allocate != nullptr) {
        bytes = user_state.config.allocate(user_state.bytes.size());
    } else {
        bytes = malloc(user_state.bytes.size());
    }
    std::copy(user_state.bytes.begin(), user_state.bytes.end(), static_cast<uint8_t *>(bytes));
    *user_state.config.out_byte_buffer_ptr = static_cast<uint8_t *>(bytes);
    *user_state.config.out_size = user_state.bytes.size();
}

// General
extern "C" void gvox_output_adapter_byte_buffer_write(GvoxAdapterContext *ctx, size_t position, size_t size, void const *data) {
    auto &user_state = *static_cast<ByteBufferOutputUserState *>(gvox_adapter_get_user_pointer(ctx));
    if (position + size > user_state.bytes.size()) {
        user_state.bytes.resize(position + size);
    }
    const auto *bytes = static_cast<uint8_t const *>(data);
    std::copy(bytes, bytes + size, user_state.bytes.data() + position);
}

extern "C" void gvox_output_adapter_byte_buffer_reserve(GvoxAdapterContext *ctx, size_t size) {
    auto &user_state = *static_cast<ByteBufferOutputUserState *>(gvox_adapter_get_user_pointer(ctx));
    if (size > user_state.bytes.size()) {
        user_state.bytes.resize(size);
    }
}
