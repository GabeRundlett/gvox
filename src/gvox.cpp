#include <gvox/gvox.h>
#include <gvox/adapter.h>

#include <cassert>
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include <array>
#include <algorithm>

#if __wasm32__
#include "utils/patch_wasm.h"
#endif

struct _GVoxInputAdapter {
    GVoxInputAdapterInfo info;
};
struct _GVoxOutputAdapter {
    GVoxOutputAdapterInfo info;
};
struct _GVoxParseAdapter {
    GVoxParseAdapterInfo info;
};
struct _GVoxSerializeAdapter {
    GVoxSerializeAdapterInfo info;
};
struct _GVoxContext {
    std::unordered_map<std::string, _GVoxInputAdapter *> input_adapter_table = {};
    std::unordered_map<std::string, _GVoxOutputAdapter *> output_adapter_table = {};
    std::unordered_map<std::string, _GVoxParseAdapter *> parse_adapter_table = {};
    std::unordered_map<std::string, _GVoxSerializeAdapter *> serialize_adapter_table = {};
    std::vector<std::pair<std::string, GVoxResult>> errors = {};
};

template <typename AdapterT>
struct AdapterState {
    AdapterT *adapter;
    void *user_ptr{};
    std::vector<void *> allocations;
    std::vector<std::pair<std::string, GVoxResult>> errors;
};

struct _GVoxAdapterContext {
    AdapterState<GVoxInputAdapter> input;
    AdapterState<GVoxOutputAdapter> output;
    AdapterState<GVoxParseAdapter> parse;
    AdapterState<GVoxSerializeAdapter> serialize;
    std::vector<GVoxRegion> regions;
    std::vector<std::pair<std::string, GVoxResult>> errors;
};

#include <adapters.hpp>

auto gvox_create_context(void) -> GVoxContext * {
    auto *ctx = new GVoxContext;
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
void gvox_destroy_context(GVoxContext *ctx) {
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

auto gvox_register_input_adapter(GVoxContext *ctx, GVoxInputAdapterInfo const *adapter_info) -> GVoxInputAdapter * {
    auto *result = new _GVoxInputAdapter{
        *adapter_info,
    };
    ctx->input_adapter_table[adapter_info->name_str] = result;
    return result;
}
auto gvox_get_input_adapter(GVoxContext *ctx, char const *adapter_name) -> GVoxInputAdapter * {
    return ctx->input_adapter_table.at(adapter_name);
}

auto gvox_register_output_adapter(GVoxContext *ctx, GVoxOutputAdapterInfo const *adapter_info) -> GVoxOutputAdapter * {
    auto *result = new _GVoxOutputAdapter{
        *adapter_info,
    };
    ctx->output_adapter_table[adapter_info->name_str] = result;
    return result;
}
auto gvox_get_output_adapter(GVoxContext *ctx, char const *adapter_name) -> GVoxOutputAdapter * {
    return ctx->output_adapter_table.at(adapter_name);
}

auto gvox_register_parse_adapter(GVoxContext *ctx, GVoxParseAdapterInfo const *adapter_info) -> GVoxParseAdapter * {
    auto *result = new _GVoxParseAdapter{
        *adapter_info,
    };
    ctx->parse_adapter_table[adapter_info->name_str] = result;
    return result;
}
auto gvox_get_parse_adapter(GVoxContext *ctx, char const *adapter_name) -> GVoxParseAdapter * {
    return ctx->parse_adapter_table.at(adapter_name);
}

auto gvox_register_serialize_adapter(GVoxContext *ctx, GVoxSerializeAdapterInfo const *adapter_info) -> GVoxSerializeAdapter * {
    auto *result = new _GVoxSerializeAdapter{
        *adapter_info,
    };
    ctx->serialize_adapter_table[adapter_info->name_str] = result;
    return result;
}
auto gvox_get_serialize_adapter(GVoxContext *ctx, char const *adapter_name) -> GVoxSerializeAdapter * {
    return ctx->serialize_adapter_table.at(adapter_name);
}

auto gvox_create_adapter_context(
    GVoxInputAdapter *input_adapter, void *input_config,
    GVoxOutputAdapter *output_adapter, void *output_config,
    GVoxParseAdapter *parse_adapter, void *parse_config,
    GVoxSerializeAdapter *serialize_adapter, void *serialize_config) -> GVoxAdapterContext * {
    auto *ctx = new GVoxAdapterContext{
        .input = {
            .adapter = input_adapter,
            .user_ptr = {},
            .allocations = {},
            .errors = {},
        },
        .output = {
            .adapter = output_adapter,
            .user_ptr = {},
            .allocations = {},
            .errors = {},
        },
        .parse = {
            .adapter = parse_adapter,
            .user_ptr = {},
            .allocations = {},
            .errors = {},
        },
        .serialize = {
            .adapter = serialize_adapter,
            .user_ptr = {},
            .allocations = {},
            .errors = {},
        },
        .regions = {},
        .errors = {},
    };
    if (ctx->input.adapter) {
        ctx->input.adapter->info.begin(ctx, input_config);
    }
    if (ctx->output.adapter) {
        ctx->output.adapter->info.begin(ctx, output_config);
    }
    if (ctx->parse.adapter) {
        ctx->parse.adapter->info.begin(ctx, parse_config);
    }
    if (ctx->serialize.adapter) {
        ctx->serialize.adapter->info.begin(ctx, serialize_config);
    }
    return ctx;
}

void gvox_destroy_adapter_context(GVoxAdapterContext *ctx) {
    if (ctx->serialize.adapter) {
        ctx->serialize.adapter->info.end(ctx);
    }
    if (ctx->parse.adapter) {
        ctx->parse.adapter->info.end(ctx);
    }
    if (ctx->output.adapter) {
        ctx->output.adapter->info.end(ctx);
    }
    if (ctx->input.adapter) {
        ctx->input.adapter->info.end(ctx);
    }
    delete ctx;
}

void gvox_translate_region(GVoxAdapterContext *ctx, GVoxRegionRange const *range) {
    if (!ctx->serialize.adapter) {
        ctx->serialize.errors.emplace_back("[GVOX TRANSLATE ERROR]: The given serialize adapter was null", GVOX_ERROR_UNKNOWN);
        return;
    }
    ctx->serialize.adapter->info.serialize_region(ctx, range);
}

auto gvox_sample_data(GVoxAdapterContext *ctx, GVoxOffset3D const *offset) -> uint32_t {
    auto region_iter = std::find_if(ctx->regions.begin(), ctx->regions.end(), [offset](GVoxRegion const &region) {
        return offset->x >= region.range.offset.x &&
               offset->y >= region.range.offset.y &&
               offset->z >= region.range.offset.z &&
               offset->x < region.range.offset.x + static_cast<int32_t>(region.range.extent.x) &&
               offset->y < region.range.offset.y + static_cast<int32_t>(region.range.extent.y) &&
               offset->z < region.range.offset.z + static_cast<int32_t>(region.range.extent.z);
    });
    GVoxRegion *region = nullptr;
    if (region_iter == ctx->regions.end()) {
        ctx->parse.adapter->info.load_region(ctx, offset);
        region = &ctx->regions.back();
    } else {
        region = &*region_iter;
    }
    auto in_region_offset = GVoxOffset3D{
        offset->x - region->range.offset.x,
        offset->y - region->range.offset.y,
        offset->z - region->range.offset.z,
    };
    return ctx->parse.adapter->info.sample_data(ctx, region, &in_region_offset);
}

auto gvox_query_region_flags(GVoxAdapterContext *ctx, GVoxRegionRange const *range) -> uint32_t {
    return ctx->parse.adapter->info.query_region_flags(ctx, range);
}

void gvox_input_adapter_push_error(GVoxAdapterContext *ctx, GVoxResult result_code, char const *message) {
    ctx->input.errors.emplace_back("[GVOX INPUT ERROR]: " + std::string(message), result_code);
    assert(0 && message);
}
void gvox_output_adapter_push_error(GVoxAdapterContext *ctx, GVoxResult result_code, char const *message) {
    ctx->output.errors.emplace_back("[GVOX OUTPUT ERROR]: " + std::string(message), result_code);
    assert(0 && message);
}
void gvox_parse_adapter_push_error(GVoxAdapterContext *ctx, GVoxResult result_code, char const *message) {
    ctx->parse.errors.emplace_back("[GVOX PARSE ERROR]: " + std::string(message), result_code);
    assert(0 && message);
}
void gvox_serialize_adapter_push_error(GVoxAdapterContext *ctx, GVoxResult result_code, char const *message) {
    ctx->serialize.errors.emplace_back("[GVOX SERIALIZE ERROR]: " + std::string(message), result_code);
    assert(0 && message);
}

auto gvox_input_adapter_malloc(GVoxAdapterContext *ctx, size_t size) -> void * {
    ctx->input.allocations.emplace_back(malloc(size));
    return ctx->input.allocations.back();
}
auto gvox_output_adapter_malloc(GVoxAdapterContext *ctx, size_t size) -> void * {
    ctx->output.allocations.emplace_back(malloc(size));
    return ctx->output.allocations.back();
}
auto gvox_parse_adapter_malloc(GVoxAdapterContext *ctx, size_t size) -> void * {
    ctx->parse.allocations.emplace_back(malloc(size));
    return ctx->parse.allocations.back();
}
auto gvox_serialize_adapter_malloc(GVoxAdapterContext *ctx, size_t size) -> void * {
    ctx->serialize.allocations.emplace_back(malloc(size));
    return ctx->serialize.allocations.back();
}

void gvox_input_adapter_set_user_pointer(GVoxAdapterContext *ctx, void *ptr) {
    if (ctx->input.user_ptr == nullptr) {
        ctx->input.user_ptr = gvox_input_adapter_malloc(ctx, sizeof(ctx->input.user_ptr));
    }
    ctx->input.user_ptr = ptr;
}
void gvox_output_adapter_set_user_pointer(GVoxAdapterContext *ctx, void *ptr) {
    if (ctx->output.user_ptr == nullptr) {
        ctx->output.user_ptr = gvox_output_adapter_malloc(ctx, sizeof(ctx->output.user_ptr));
    }
    ctx->output.user_ptr = ptr;
}
void gvox_parse_adapter_set_user_pointer(GVoxAdapterContext *ctx, void *ptr) {
    if (ctx->parse.user_ptr == nullptr) {
        ctx->parse.user_ptr = gvox_parse_adapter_malloc(ctx, sizeof(ctx->parse.user_ptr));
    }
    ctx->parse.user_ptr = ptr;
}
void gvox_serialize_adapter_set_user_pointer(GVoxAdapterContext *ctx, void *ptr) {
    if (ctx->serialize.user_ptr == nullptr) {
        ctx->serialize.user_ptr = gvox_serialize_adapter_malloc(ctx, sizeof(ctx->serialize.user_ptr));
    }
    ctx->serialize.user_ptr = ptr;
}

auto gvox_input_adapter_get_user_pointer(GVoxAdapterContext *ctx) -> void * {
    if (ctx->input.user_ptr == nullptr) {
        ctx->input.user_ptr = gvox_input_adapter_malloc(ctx, sizeof(ctx->input.user_ptr));
    }
    return ctx->input.user_ptr;
}
auto gvox_output_adapter_get_user_pointer(GVoxAdapterContext *ctx) -> void * {
    if (ctx->output.user_ptr == nullptr) {
        ctx->output.user_ptr = gvox_output_adapter_malloc(ctx, sizeof(ctx->output.user_ptr));
    }
    return ctx->output.user_ptr;
}
auto gvox_parse_adapter_get_user_pointer(GVoxAdapterContext *ctx) -> void * {
    if (ctx->parse.user_ptr == nullptr) {
        ctx->parse.user_ptr = gvox_parse_adapter_malloc(ctx, sizeof(ctx->parse.user_ptr));
    }
    return ctx->parse.user_ptr;
}
auto gvox_serialize_adapter_get_user_pointer(GVoxAdapterContext *ctx) -> void * {
    if (ctx->serialize.user_ptr == nullptr) {
        ctx->serialize.user_ptr = gvox_serialize_adapter_malloc(ctx, sizeof(ctx->serialize.user_ptr));
    }
    return ctx->serialize.user_ptr;
}

// To be used by the parse adapter
void gvox_make_region_available(GVoxAdapterContext *ctx, GVoxRegion const *region) {
    ctx->regions.push_back(*region);
}

void gvox_input_read(GVoxAdapterContext *ctx, size_t position, size_t size, void *data) {
    ctx->input.adapter->info.read(ctx, position, size, data);
}

// To be used by the serialize adapter
void gvox_output_write(GVoxAdapterContext *ctx, size_t position, size_t size, void const *data) {
    ctx->output.adapter->info.write(ctx, position, size, data);
}

void gvox_output_reserve(GVoxAdapterContext *ctx, size_t size) {
    ctx->output.adapter->info.reserve(ctx, size);
}
