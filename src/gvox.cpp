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

struct _GvoxAdapter {
    GvoxAdapterBaseInfo base_info;
};
struct GvoxInputAdapter {
    GvoxInputAdapterInfo info;
};
struct GvoxOutputAdapter {
    GvoxOutputAdapterInfo info;
};
struct GvoxParseAdapter {
    GvoxParseAdapterInfo info;
};
struct GvoxSerializeAdapter {
    GvoxSerializeAdapterInfo info;
};
struct _GvoxContext {
    std::unordered_map<std::string, GvoxInputAdapter *> input_adapter_table{};
    std::unordered_map<std::string, GvoxOutputAdapter *> output_adapter_table{};
    std::unordered_map<std::string, GvoxParseAdapter *> parse_adapter_table{};
    std::unordered_map<std::string, GvoxSerializeAdapter *> serialize_adapter_table{};
    std::vector<std::pair<std::string, GvoxResult>> errors{};
#if GVOX_ENABLE_THREADSAFETY
    std::mutex mtx{};
#endif
};
struct _GvoxAdapterContext {
    GvoxContext *gvox_context_ptr;
    GvoxAdapter *adapter;
    void *user_ptr;
};
struct _GvoxBlitContext {
    GvoxAdapterContext *i_ctx;
    GvoxAdapterContext *o_ctx;
    GvoxAdapterContext *p_ctx;
    GvoxAdapterContext *s_ctx;
    uint32_t channel_flags;
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

auto gvox_register_input_adapter(GvoxContext *ctx, GvoxInputAdapterInfo const *adapter_info) -> GvoxAdapter * {
    auto adapter_iter = ctx->input_adapter_table.find(adapter_info->base_info.name_str);
    if (adapter_iter != ctx->input_adapter_table.end()) {
        ctx->errors.emplace_back("[GVOX CONTEXT ERROR]: Tried registering an adapter with an adapter already registered with the same name", GVOX_RESULT_ERROR_UNKNOWN);
        return nullptr;
    }
    auto *result = new GvoxInputAdapter{
        *adapter_info,
    };
    ctx->input_adapter_table[adapter_info->base_info.name_str] = result;
    return reinterpret_cast<GvoxAdapter *>(result);
}
auto gvox_get_input_adapter(GvoxContext *ctx, char const *adapter_name) -> GvoxAdapter * {
    auto adapter_iter = ctx->input_adapter_table.find(adapter_name);
    if (adapter_iter == ctx->input_adapter_table.end()) {
        return nullptr;
    } else {
        auto &[name, adapter] = *adapter_iter;
        return reinterpret_cast<GvoxAdapter *>(adapter);
    }
}

auto gvox_register_output_adapter(GvoxContext *ctx, GvoxOutputAdapterInfo const *adapter_info) -> GvoxAdapter * {
    auto adapter_iter = ctx->output_adapter_table.find(adapter_info->base_info.name_str);
    if (adapter_iter != ctx->output_adapter_table.end()) {
        ctx->errors.emplace_back("[GVOX CONTEXT ERROR]: Tried registering an adapter while an adapter is already registered with the same name", GVOX_RESULT_ERROR_UNKNOWN);
        return nullptr;
    }
    auto *result = new GvoxOutputAdapter{
        *adapter_info,
    };
    ctx->output_adapter_table[adapter_info->base_info.name_str] = result;
    return reinterpret_cast<GvoxAdapter *>(result);
}
auto gvox_get_output_adapter(GvoxContext *ctx, char const *adapter_name) -> GvoxAdapter * {
    auto adapter_iter = ctx->output_adapter_table.find(adapter_name);
    if (adapter_iter == ctx->output_adapter_table.end()) {
        return nullptr;
    } else {
        auto &[name, adapter] = *adapter_iter;
        return reinterpret_cast<GvoxAdapter *>(adapter);
    }
}

auto gvox_register_parse_adapter(GvoxContext *ctx, GvoxParseAdapterInfo const *adapter_info) -> GvoxAdapter * {
    auto adapter_iter = ctx->parse_adapter_table.find(adapter_info->base_info.name_str);
    if (adapter_iter != ctx->parse_adapter_table.end()) {
        ctx->errors.emplace_back("[GVOX CONTEXT ERROR]: Tried registering an adapter while an adapter is already registered with the same name", GVOX_RESULT_ERROR_UNKNOWN);
        return nullptr;
    }
    auto *result = new GvoxParseAdapter{
        *adapter_info,
    };
    ctx->parse_adapter_table[adapter_info->base_info.name_str] = result;
    return reinterpret_cast<GvoxAdapter *>(result);
}
auto gvox_get_parse_adapter(GvoxContext *ctx, char const *adapter_name) -> GvoxAdapter * {
    auto adapter_iter = ctx->parse_adapter_table.find(adapter_name);
    if (adapter_iter == ctx->parse_adapter_table.end()) {
        return nullptr;
    } else {
        auto &[name, adapter] = *adapter_iter;
        return reinterpret_cast<GvoxAdapter *>(adapter);
    }
}

auto gvox_register_serialize_adapter(GvoxContext *ctx, GvoxSerializeAdapterInfo const *adapter_info) -> GvoxAdapter * {
    auto adapter_iter = ctx->serialize_adapter_table.find(adapter_info->base_info.name_str);
    if (adapter_iter != ctx->serialize_adapter_table.end()) {
        ctx->errors.emplace_back("[GVOX CONTEXT ERROR]: Tried registering an adapter while an adapter is already registered with the same name", GVOX_RESULT_ERROR_UNKNOWN);
        return nullptr;
    }
    auto *result = new GvoxSerializeAdapter{
        *adapter_info,
    };
    ctx->serialize_adapter_table[adapter_info->base_info.name_str] = result;
    return reinterpret_cast<GvoxAdapter *>(result);
}
auto gvox_get_serialize_adapter(GvoxContext *ctx, char const *adapter_name) -> GvoxAdapter * {
    auto adapter_iter = ctx->serialize_adapter_table.find(adapter_name);
    if (adapter_iter == ctx->serialize_adapter_table.end()) {
        return nullptr;
    } else {
        auto &[name, adapter] = *adapter_iter;
        return reinterpret_cast<GvoxAdapter *>(adapter);
    }
}

void gvox_adapter_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    if (ctx != nullptr && ctx->adapter != nullptr) {
        ctx->adapter->base_info.blit_begin(blit_ctx, ctx);
    }
}
void gvox_adapter_blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    if (ctx != nullptr && ctx->adapter != nullptr) {
        ctx->adapter->base_info.blit_end(blit_ctx, ctx);
    }
}

auto gvox_create_adapter_context(GvoxContext *gvox_ctx, GvoxAdapter *adapter, void const *config) -> GvoxAdapterContext * {
    auto *ctx = new GvoxAdapterContext{
        .gvox_context_ptr = gvox_ctx,
        .adapter = adapter,
        .user_ptr = {},
    };
    if (ctx->adapter != nullptr) {
        ctx->adapter->base_info.create(ctx, config);
    }
    return ctx;
}
void gvox_destroy_adapter_context(GvoxAdapterContext *ctx) {
    if ((ctx != nullptr) && ctx->adapter != nullptr) {
        ctx->adapter->base_info.destroy(ctx);
    }
    delete ctx;
}

static void gvox_blit_region_impl(
    GvoxAdapterContext *input_ctx, GvoxAdapterContext *output_ctx,
    GvoxAdapterContext *parse_ctx, GvoxAdapterContext *serialize_ctx,
    GvoxRegionRange const *requested_range,
    uint32_t channel_flags,
    GvoxBlitMode blit_mode) {
    if (parse_ctx->adapter == nullptr) {
        gvox_adapter_push_error(serialize_ctx, GVOX_RESULT_ERROR_INVALID_PARAMETER, "[BLIT ERROR]: The parse adapter mustn't be null");
        return;
    }
    if (serialize_ctx->adapter == nullptr) {
        gvox_adapter_push_error(serialize_ctx, GVOX_RESULT_ERROR_INVALID_PARAMETER, "[BLIT ERROR]: The serialize adapter mustn't be null");
        return;
    }
    auto blit_ctx = GvoxBlitContext{
        .i_ctx = input_ctx,
        .o_ctx = output_ctx,
        .p_ctx = parse_ctx,
        .s_ctx = serialize_ctx,
        .channel_flags = channel_flags,
    };

    gvox_adapter_blit_begin(&blit_ctx, blit_ctx.i_ctx);
    gvox_adapter_blit_begin(&blit_ctx, blit_ctx.o_ctx);
    gvox_adapter_blit_begin(&blit_ctx, blit_ctx.p_ctx);

    GvoxRegionRange actual_range;
    if (requested_range != nullptr) {
        actual_range = *requested_range;
    } else {
        actual_range = reinterpret_cast<GvoxParseAdapter *>(parse_ctx->adapter)->info.query_parsable_range(&blit_ctx, parse_ctx);
    }

    gvox_adapter_blit_begin(&blit_ctx, blit_ctx.s_ctx);
    switch (blit_mode) {
    default:
    case GVOX_BLIT_MODE_PARSE_DRIVEN:
        reinterpret_cast<GvoxSerializeAdapter *>(serialize_ctx->adapter)->info.parse_driven_begin(&blit_ctx, parse_ctx, &actual_range);
        reinterpret_cast<GvoxParseAdapter *>(parse_ctx->adapter)->info.parse_region(&blit_ctx, parse_ctx, &actual_range, channel_flags);
        break;
    case GVOX_BLIT_MODE_SERIALIZE_DRIVEN:
        reinterpret_cast<GvoxSerializeAdapter *>(serialize_ctx->adapter)->info.serialize_region(&blit_ctx, serialize_ctx, &actual_range, channel_flags);
        break;
    }
    gvox_adapter_blit_end(&blit_ctx, blit_ctx.i_ctx);
    gvox_adapter_blit_end(&blit_ctx, blit_ctx.o_ctx);
    gvox_adapter_blit_end(&blit_ctx, blit_ctx.p_ctx);
    gvox_adapter_blit_end(&blit_ctx, blit_ctx.s_ctx);
}

void gvox_blit_region(
    GvoxAdapterContext *input_ctx, GvoxAdapterContext *output_ctx,
    GvoxAdapterContext *parse_ctx, GvoxAdapterContext *serialize_ctx,
    GvoxRegionRange const *requested_range,
    uint32_t channel_flags) {
    if (parse_ctx->adapter == nullptr) {
        gvox_adapter_push_error(serialize_ctx, GVOX_RESULT_ERROR_INVALID_PARAMETER, "[BLIT ERROR]: The parse adapter mustn't be null");
        return;
    }
    auto *parse_adapter = reinterpret_cast<GvoxParseAdapter *>(parse_ctx->adapter);
    // Backwards compat cope
    auto preferred_blit_mode = GVOX_BLIT_MODE_DONT_CARE;
    if (parse_adapter->info.query_details != nullptr) {
        preferred_blit_mode = parse_adapter->info.query_details().preferred_blit_mode;
    }

    gvox_blit_region_impl(
        input_ctx, output_ctx,
        parse_ctx, serialize_ctx,
        requested_range,
        channel_flags,
        preferred_blit_mode);
}

void gvox_blit_region_parse_driven(
    GvoxAdapterContext *input_ctx, GvoxAdapterContext *output_ctx,
    GvoxAdapterContext *parse_ctx, GvoxAdapterContext *serialize_ctx,
    GvoxRegionRange const *requested_range,
    uint32_t channel_flags) {
    gvox_blit_region_impl(
        input_ctx, output_ctx,
        parse_ctx, serialize_ctx,
        requested_range,
        channel_flags,
        GVOX_BLIT_MODE_PARSE_DRIVEN);
}

void gvox_blit_region_serialize_driven(
    GvoxAdapterContext *input_ctx, GvoxAdapterContext *output_ctx,
    GvoxAdapterContext *parse_ctx, GvoxAdapterContext *serialize_ctx,
    GvoxRegionRange const *requested_range,
    uint32_t channel_flags) {
    gvox_blit_region_impl(
        input_ctx, output_ctx,
        parse_ctx, serialize_ctx,
        requested_range,
        channel_flags,
        GVOX_BLIT_MODE_SERIALIZE_DRIVEN);
}

// Adapter API

// Input
void gvox_input_read(GvoxBlitContext *blit_ctx, size_t position, size_t size, void *data) {
    auto &i_adapter = *reinterpret_cast<GvoxInputAdapter *>(blit_ctx->i_ctx->adapter);
    i_adapter.info.read(reinterpret_cast<GvoxAdapterContext *>(blit_ctx->i_ctx), position, size, data);
}
// Output
void gvox_output_write(GvoxBlitContext *blit_ctx, size_t position, size_t size, void const *data) {
    auto &o_adapter = *reinterpret_cast<GvoxOutputAdapter *>(blit_ctx->o_ctx->adapter);
    o_adapter.info.write(reinterpret_cast<GvoxAdapterContext *>(blit_ctx->o_ctx), position, size, data);
}
void gvox_output_reserve(GvoxBlitContext *blit_ctx, size_t size) {
    auto &o_adapter = *reinterpret_cast<GvoxOutputAdapter *>(blit_ctx->o_ctx->adapter);
    o_adapter.info.reserve(reinterpret_cast<GvoxAdapterContext *>(blit_ctx->o_ctx), size);
}
// General
void gvox_adapter_push_error(GvoxAdapterContext *ctx, GvoxResult result_code, char const *message) {
#if GVOX_ENABLE_THREADSAFETY
    auto lock = std::lock_guard{ctx->gvox_context_ptr->mtx};
#endif
    ctx->gvox_context_ptr->errors.emplace_back("[GVOX ADAPTER ERROR]: " + std::string(message), result_code);
#if !GVOX_BUILD_FOR_RUST && !GVOX_BUILD_FOR_ODIN
    assert(0 && message);
#endif
}
void gvox_adapter_set_user_pointer(GvoxAdapterContext *ctx, void *ptr) {
    ctx->user_ptr = ptr;
}
auto gvox_adapter_get_user_pointer(GvoxAdapterContext *ctx) -> void * {
    return ctx->user_ptr;
}
auto gvox_sample_region(GvoxBlitContext *blit_ctx, GvoxRegion const *region, GvoxOffset3D const *offset, uint32_t channel_id) -> uint32_t {
    auto &p_adapter = *reinterpret_cast<GvoxParseAdapter *>(blit_ctx->p_ctx->adapter);
    auto offset_copy = *offset;
    return p_adapter.info.sample_region(blit_ctx, reinterpret_cast<GvoxAdapterContext *>(blit_ctx->p_ctx), region, &offset_copy, channel_id);
}

// Serialize Driven
auto gvox_query_region_flags(GvoxBlitContext *blit_ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> uint32_t {
    auto &p_adapter = *reinterpret_cast<GvoxParseAdapter *>(blit_ctx->p_ctx->adapter);
    return p_adapter.info.query_region_flags(blit_ctx, reinterpret_cast<GvoxAdapterContext *>(blit_ctx->p_ctx), range, channel_flags);
}
auto gvox_load_region_range(GvoxBlitContext *blit_ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    auto &p_adapter = *reinterpret_cast<GvoxParseAdapter *>(blit_ctx->p_ctx->adapter);
    return p_adapter.info.load_region(blit_ctx, reinterpret_cast<GvoxAdapterContext *>(blit_ctx->p_ctx), range, channel_flags);
}
void gvox_unload_region_range(GvoxBlitContext *blit_ctx, GvoxRegion *region, GvoxRegionRange const * /*range*/) {
    auto &p_adapter = *reinterpret_cast<GvoxParseAdapter *>(blit_ctx->p_ctx->adapter);
    p_adapter.info.unload_region(blit_ctx, reinterpret_cast<GvoxAdapterContext *>(blit_ctx->p_ctx), region);
}

// Parse Driven
void gvox_emit_region(GvoxBlitContext *blit_ctx, GvoxRegion const *region) {
    auto &s_adapter = *reinterpret_cast<GvoxSerializeAdapter *>(blit_ctx->s_ctx->adapter);
    s_adapter.info.receive_region(blit_ctx, reinterpret_cast<GvoxAdapterContext *>(blit_ctx->s_ctx), region);
}
