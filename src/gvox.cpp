#include <gvox/gvox.h>

#include <cassert>
#include <unordered_map>
#include <string>
#include <vector>
#include <array>
#include <algorithm>

#include <mutex>

#if __wasm32__
#include "utils/patch_wasm.h"
#endif

struct _GvoxInputAdapter {
    GvoxInputAdapterInfo info;
};
struct _GvoxOutputAdapter {
    GvoxOutputAdapterInfo info;
};
struct _GvoxParseAdapter {
    GvoxParseAdapterInfo info;
};
struct _GvoxSerializeAdapter {
    GvoxSerializeAdapterInfo info;
};
struct _GvoxContext {
    std::unordered_map<std::string, _GvoxInputAdapter *> input_adapter_table{};
    std::unordered_map<std::string, _GvoxOutputAdapter *> output_adapter_table{};
    std::unordered_map<std::string, _GvoxParseAdapter *> parse_adapter_table{};
    std::unordered_map<std::string, _GvoxSerializeAdapter *> serialize_adapter_table{};
    std::vector<std::pair<std::string, GvoxResult>> errors{};
#if GVOX_ENABLE_THREADSAFETY
    std::mutex mtx{};
#endif
};

template <typename AdapterT>
struct AdapterState {
    AdapterT *adapter;
    void *user_ptr{};
};

struct _GvoxAdapterContext {
    GvoxContext *gvox_context_ptr;
    AdapterState<GvoxInputAdapter> input;
    AdapterState<GvoxOutputAdapter> output;
    AdapterState<GvoxParseAdapter> parse;
    AdapterState<GvoxSerializeAdapter> serialize;
};

#include <adapters.hpp>

auto gvox_create_context(void) -> GvoxContext * {
    auto *ctx = new GvoxContext;
    for (auto const &info : input_adapter_infos) {
        gvox_register_input_adapter(ctx, &info);
    }
    for (auto const &info : output_adapter_infos) {
        gvox_register_output_adapter(ctx, &info);
    }
    for (auto const &info : parse_adapter_infos) {
        gvox_register_parse_adapter(ctx, &info);
    }
    for (auto const &info : serialize_adapter_infos) {
        gvox_register_serialize_adapter(ctx, &info);
    }
    return ctx;
}
void gvox_destroy_context(GvoxContext *ctx) {
    if (ctx == nullptr) {
        return;
    }
    for (auto &[key, adapter] : ctx->input_adapter_table) {
        delete adapter;
    }
    for (auto &[key, adapter] : ctx->output_adapter_table) {
        delete adapter;
    }
    for (auto &[key, adapter] : ctx->parse_adapter_table) {
        delete adapter;
    }
    for (auto &[key, adapter] : ctx->serialize_adapter_table) {
        delete adapter;
    }
    delete ctx;
}

auto gvox_get_result(GvoxContext *ctx) -> GvoxResult {
    if (ctx->errors.empty()) {
        return GVOX_RESULT_SUCCESS;
    }
    auto [msg, id] = ctx->errors.back();
    return id;
}
void gvox_get_result_message(GvoxContext *ctx, char *const str_buffer, size_t *str_size) {
    if (str_buffer != nullptr) {
        if (ctx->errors.empty()) {
            for (size_t i = 0; i < *str_size; ++i) {
                str_buffer[i] = '\0';
            }
            return;
        }
        auto [msg, id] = ctx->errors.back();
        std::copy(msg.begin(), msg.end(), str_buffer);
    } else if (str_size != nullptr) {
        if (ctx->errors.empty()) {
            *str_size = 0;
            return;
        }
        auto [msg, id] = ctx->errors.back();
        *str_size = msg.size();
    }
}
void gvox_pop_result(GvoxContext *ctx) {
    ctx->errors.pop_back();
}

auto gvox_register_input_adapter(GvoxContext *ctx, GvoxInputAdapterInfo const *adapter_info) -> GvoxInputAdapter * {
    auto *result = new _GvoxInputAdapter{
        *adapter_info,
    };
    ctx->input_adapter_table[adapter_info->name_str] = result;
    return result;
}
auto gvox_get_input_adapter(GvoxContext *ctx, char const *adapter_name) -> GvoxInputAdapter * {
    return ctx->input_adapter_table.at(adapter_name);
}

auto gvox_register_output_adapter(GvoxContext *ctx, GvoxOutputAdapterInfo const *adapter_info) -> GvoxOutputAdapter * {
    auto *result = new _GvoxOutputAdapter{
        *adapter_info,
    };
    ctx->output_adapter_table[adapter_info->name_str] = result;
    return result;
}
auto gvox_get_output_adapter(GvoxContext *ctx, char const *adapter_name) -> GvoxOutputAdapter * {
    return ctx->output_adapter_table.at(adapter_name);
}

auto gvox_register_parse_adapter(GvoxContext *ctx, GvoxParseAdapterInfo const *adapter_info) -> GvoxParseAdapter * {
    auto *result = new _GvoxParseAdapter{
        *adapter_info,
    };
    ctx->parse_adapter_table[adapter_info->name_str] = result;
    return result;
}
auto gvox_get_parse_adapter(GvoxContext *ctx, char const *adapter_name) -> GvoxParseAdapter * {
    return ctx->parse_adapter_table.at(adapter_name);
}

auto gvox_register_serialize_adapter(GvoxContext *ctx, GvoxSerializeAdapterInfo const *adapter_info) -> GvoxSerializeAdapter * {
    auto *result = new _GvoxSerializeAdapter{
        *adapter_info,
    };
    ctx->serialize_adapter_table[adapter_info->name_str] = result;
    return result;
}
auto gvox_get_serialize_adapter(GvoxContext *ctx, char const *adapter_name) -> GvoxSerializeAdapter * {
    return ctx->serialize_adapter_table.at(adapter_name);
}

auto gvox_create_adapter_context(
    GvoxContext *gvox_ctx,
    GvoxInputAdapter *input_adapter, void *input_config,
    GvoxOutputAdapter *output_adapter, void *output_config,
    GvoxParseAdapter *parse_adapter, void *parse_config,
    GvoxSerializeAdapter *serialize_adapter, void *serialize_config) -> GvoxAdapterContext * {
    auto *ctx = new GvoxAdapterContext{
        .gvox_context_ptr = gvox_ctx,
        .input = {
            .adapter = input_adapter,
            .user_ptr = {},
        },
        .output = {
            .adapter = output_adapter,
            .user_ptr = {},
        },
        .parse = {
            .adapter = parse_adapter,
            .user_ptr = {},
        },
        .serialize = {
            .adapter = serialize_adapter,
            .user_ptr = {},
        },
    };
    if (ctx->input.adapter != nullptr) {
        ctx->input.adapter->info.begin(ctx, input_config);
    }
    if (ctx->output.adapter != nullptr) {
        ctx->output.adapter->info.begin(ctx, output_config);
    }
    if (ctx->parse.adapter != nullptr) {
        ctx->parse.adapter->info.begin(ctx, parse_config);
    }
    if (ctx->serialize.adapter != nullptr) {
        ctx->serialize.adapter->info.begin(ctx, serialize_config);
    }
    return ctx;
}
void gvox_destroy_adapter_context(GvoxAdapterContext *ctx) {
    if (ctx->serialize.adapter != nullptr) {
        ctx->serialize.adapter->info.end(ctx);
    }
    if (ctx->parse.adapter != nullptr) {
        ctx->parse.adapter->info.end(ctx);
    }
    if (ctx->output.adapter != nullptr) {
        ctx->output.adapter->info.end(ctx);
    }
    if (ctx->input.adapter != nullptr) {
        ctx->input.adapter->info.end(ctx);
    }

    delete ctx;
}
void gvox_translate_region(GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    if (ctx->serialize.adapter == nullptr) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_INVALID_PARAMETER, "[TRANSLATE ERROR]: The given serialize adapter was null");
        return;
    }
    // auto t0 = std::chrono::high_resolution_clock::now();
    ctx->serialize.adapter->info.serialize_region(ctx, range, channel_flags);
    // auto t1 = std::chrono::high_resolution_clock::now();
    // std::cout << "Elapsed: " << std::chrono::duration<float>(t1 - t0).count() << std::endl;
}

auto gvox_load_region(GvoxAdapterContext *ctx, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxRegion {
    return ctx->parse.adapter->info.load_region(ctx, offset, channel_id);
}
void gvox_unload_region(GvoxAdapterContext *ctx, GvoxRegion *region) {
    ctx->parse.adapter->info.unload_region(ctx, region);
}
auto gvox_sample_region(GvoxAdapterContext *ctx, GvoxRegion *region, GvoxOffset3D const *offset, uint32_t channel_id) -> uint32_t {
    auto in_region_offset = GvoxOffset3D{
        offset->x - region->range.offset.x,
        offset->y - region->range.offset.y,
        offset->z - region->range.offset.z,
    };
    return ctx->parse.adapter->info.sample_region(ctx, region, &in_region_offset, channel_id);
}
auto gvox_query_region_flags(GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_id) -> uint32_t {
    return ctx->parse.adapter->info.query_region_flags(ctx, range, channel_id);
}

void gvox_adapter_push_error(GvoxAdapterContext *ctx, GvoxResult result_code, char const *message) {
#if GVOX_ENABLE_THREADSAFETY
    auto lock = std::lock_guard{ctx->gvox_context_ptr->mtx};
#endif
    ctx->gvox_context_ptr->errors.emplace_back("[GVOX ADAPTER ERROR]: " + std::string(message), result_code);
#if !GVOX_BUILD_FOR_RUST && !GVOX_BUILD_FOR_ODIN
    assert(0 && message);
#endif
}

void gvox_input_adapter_set_user_pointer(GvoxAdapterContext *ctx, void *ptr) {
    ctx->input.user_ptr = ptr;
}
void gvox_output_adapter_set_user_pointer(GvoxAdapterContext *ctx, void *ptr) {
    ctx->output.user_ptr = ptr;
}
void gvox_parse_adapter_set_user_pointer(GvoxAdapterContext *ctx, void *ptr) {
    ctx->parse.user_ptr = ptr;
}
void gvox_serialize_adapter_set_user_pointer(GvoxAdapterContext *ctx, void *ptr) {
    ctx->serialize.user_ptr = ptr;
}

auto gvox_input_adapter_get_user_pointer(GvoxAdapterContext *ctx) -> void * {
    return ctx->input.user_ptr;
}
auto gvox_output_adapter_get_user_pointer(GvoxAdapterContext *ctx) -> void * {
    return ctx->output.user_ptr;
}
auto gvox_parse_adapter_get_user_pointer(GvoxAdapterContext *ctx) -> void * {
    return ctx->parse.user_ptr;
}
auto gvox_serialize_adapter_get_user_pointer(GvoxAdapterContext *ctx) -> void * {
    return ctx->serialize.user_ptr;
}

// To be used by the parse adapter
void gvox_input_read(GvoxAdapterContext *ctx, size_t position, size_t size, void *data) {
    ctx->input.adapter->info.read(ctx, position, size, data);
}

// To be used by the serialize adapter
void gvox_output_write(GvoxAdapterContext *ctx, size_t position, size_t size, void const *data) {
    ctx->output.adapter->info.write(ctx, position, size, data);
}

void gvox_output_reserve(GvoxAdapterContext *ctx, size_t size) {
    ctx->output.adapter->info.reserve(ctx, size);
}
