#include <gvox/gvox.h>

#include <cstring>
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
struct _GVoxFormatAdapter {
    void *context;
    GVoxFormatAdapterInfo info;
};
struct _GVoxContext {
    std::unordered_map<std::string, _GVoxInputAdapter *> input_adapter_table = {};
    std::unordered_map<std::string, _GVoxOutputAdapter *> output_adapter_table = {};
    std::unordered_map<std::string, _GVoxFormatAdapter *> format_adapter_table = {};
    std::vector<std::pair<std::string, GVoxResult>> errors = {};
};

#include <formats.hpp>

static void impl_gvox_unregister_input_adapter(GVoxContext *ctx, _GVoxInputAdapter const &self) {
}
static void impl_gvox_unregister_output_adapter(GVoxContext *ctx, _GVoxOutputAdapter const &self) {
}
static void impl_gvox_unregister_format_adapter(GVoxContext *ctx, _GVoxFormatAdapter const &self) {
    if (self.context != nullptr) {
        self.info.destroy_context(self.context);
    }
}
static uint32_t gvox_region_index(GVoxRegion const *region, GVoxExtent3D const *position) {
    if (region->metadata.flags & GVOX_REGION_IS_UNIFORM_BIT) {
        return 0;
    } else {
        return region->data[position->x + position->y * region->metadata.extent.x + position->z * region->metadata.extent.x * region->metadata.extent.y];
    }
}

auto gvox_create_context(void) -> GVoxContext * {
    auto *ctx = new GVoxContext;
    for (const auto *const format_loader_name : format_names) {
        std::string filename = std::string("gvox_format_") + format_loader_name;

        auto format_iter = std::find_if(format_infos.begin(), format_infos.end(), [&](auto const &format_info) {
            return format_info.name == filename;
        });

        if (format_iter != format_infos.end()) {
            GVoxFormatAdapterInfo const format_adapter_info = {
                .name_str = format_loader_name,
                // .create_context = format_iter->create_context,
                // .destroy_context = format_iter->destroy_context,
                // .serialize_begin = format_iter->serialize_begin,
                // .serialize_end = format_iter->serialize_end,
                // .serialize_region = format_iter->serialize_region,
            };
            gvox_register_format_adapter(ctx, &format_adapter_info);
        }
    }
    return ctx;
}
void gvox_destroy_context(GVoxContext *ctx) {
    if (ctx == nullptr) {
        return;
    }
    for (auto &[key, adapter] : ctx->input_adapter_table) {
        impl_gvox_unregister_input_adapter(ctx, *adapter);
        delete adapter;
    }
    for (auto &[key, adapter] : ctx->output_adapter_table) {
        impl_gvox_unregister_output_adapter(ctx, *adapter);
        delete adapter;
    }
    for (auto &[key, adapter] : ctx->format_adapter_table) {
        impl_gvox_unregister_format_adapter(ctx, *adapter);
        delete adapter;
    }
    delete ctx;
}

GVoxInputAdapter *gvox_register_input_adapter(GVoxContext *ctx, GVoxInputAdapterInfo const *adapter_info) {
    void *adapter_context = nullptr;
    _GVoxInputAdapter *result = new _GVoxInputAdapter{
        *adapter_info,
    };
    ctx->input_adapter_table[adapter_info->name_str] = result;
    return result;
}
GVoxInputAdapter *gvox_get_input_adapter(GVoxContext *ctx, char const *adapter_name) {
    return ctx->input_adapter_table.at(adapter_name);
}

GVoxOutputAdapter *gvox_register_output_adapter(GVoxContext *ctx, GVoxOutputAdapterInfo const *adapter_info) {
    void *adapter_context = nullptr;
    _GVoxOutputAdapter *result = new _GVoxOutputAdapter{
        *adapter_info,
    };
    ctx->output_adapter_table[adapter_info->name_str] = result;
    return result;
}
GVoxOutputAdapter *gvox_get_output_adapter(GVoxContext *ctx, char const *adapter_name) {
    return ctx->output_adapter_table.at(adapter_name);
}

GVoxFormatAdapter *gvox_register_format_adapter(GVoxContext *ctx, GVoxFormatAdapterInfo const *adapter_info) {
    void *adapter_context = nullptr;
    if (adapter_info->create_context != nullptr) {
        adapter_context = adapter_info->create_context();
    }
    _GVoxFormatAdapter *result = new _GVoxFormatAdapter{
        adapter_context,
        *adapter_info,
    };
    ctx->format_adapter_table[adapter_info->name_str] = result;
    return result;
    // ctx->errors.emplace_back("Failed to register format [" + std::string(adapter_info->name_str) + "]", GVOX_ERROR_INVALID_FORMAT);
}
GVoxFormatAdapter *gvox_get_format_adapter(GVoxContext *ctx, char const *adapter_name) {
    return ctx->format_adapter_table.at(adapter_name);
}

GVoxVoxel gvox_region_sample_voxel(GVoxRegion const *region, GVoxExtent3D const *position) {
    return region->data[gvox_region_index(region, position)];
}

GVoxRegionList gvox_parse(
    GVoxInputAdapter *input_adapter,
    void const *input_config,
    GVoxFormatAdapter *format_adapter,
    void const *format_config) {

    GVoxParseState state;
    state.region_list = {};

    state.input_adapter = input_adapter;
    state.input_config = input_config;
    state.format_adapter = format_adapter;
    state.format_config = format_config;

    if (input_adapter->info.begin) {
        input_adapter->info.begin(&state);
    }

    format_adapter->info.parse(&state);

    if (input_adapter->info.end) {
        input_adapter->info.end(&state);
    }

    return state.region_list;
}

void gvox_serialize(
    GVoxOutputAdapter *output_adapter,
    void const *output_config,
    GVoxFormatAdapter *format_adapter,
    void const *format_config,
    GVoxRegion const *region) {

    GVoxSerializeState state;
    state.full_region_metadata = &region->metadata;

    state.output_adapter = output_adapter;
    state.output_config = output_config;
    state.format_adapter = format_adapter;
    state.format_config = format_config;

    if (output_adapter->info.begin) {
        output_adapter->info.begin(&state);
    }

    if (format_adapter->info.serialize_begin) {
        format_adapter->info.serialize_begin(&state);
    }

    format_adapter->info.serialize_region(&state, region);

    if (format_adapter->info.serialize_end) {
        format_adapter->info.serialize_end(&state);
    }

    if (output_adapter->info.end) {
        output_adapter->info.end(&state);
    }
}

void gvox_serialize_write(GVoxSerializeState *state, size_t position, size_t size, void const *data) {
    state->output_adapter->info.write(state, position, size, data);
}

void gvox_serialize_reserve(GVoxSerializeState *state, size_t size) {
    state->output_adapter->info.reserve(state, size);
}
