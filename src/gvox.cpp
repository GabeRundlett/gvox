#include <gvox/gvox.h>

#include <cassert>
#include <cstring>
#include <unordered_map>
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>
#include <iostream>
#include <array>
#include <algorithm>

#if __linux__
#include <unistd.h>
#include <dlfcn.h>
#elif _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

using GVoxFormatCreateContextFunc = void *(*)();
using GVoxFormatDestroyContextFunc = void (*)(void *);
using GVoxFormatCreatePayloadFunc = GVoxPayload (*)(void *, GVoxScene);
using GVoxFormatDestroyPayloadFunc = void (*)(void *, GVoxPayload);
using GVoxFormatParsePayloadFunc = GVoxScene (*)(void *, GVoxPayload);

#include <formats.hpp>

struct _GVoxContext {
    std::unordered_map<std::string, GVoxFormatLoader *> format_loader_table = {};
    std::vector<std::filesystem::path> root_paths = {};
    std::vector<std::pair<std::string, GVoxResult>> errors = {};
};

static auto gvox_context_find_loader(GVoxContext *ctx, std::string const &format_name) -> GVoxFormatLoader * {
    auto iter = ctx->format_loader_table.find(format_name);
    if (iter == ctx->format_loader_table.end()) {
        ctx->errors.emplace_back("Failed to find format [" + format_name + "]", GVOX_ERROR_INVALID_FORMAT);
        return nullptr;
    }
    return iter->second;
}
auto get_exe_path() -> std::filesystem::path {
    char *out_str = new char[512];
    for (size_t i = 0; i < 512; ++i) {
        out_str[i] = '\0';
    }
#if __linux__
    readlink("/proc/self/exe", out_str, 511);
#elif _WIN32
    GetModuleFileName(nullptr, out_str, 511);
#endif
    auto result = std::filesystem::path(out_str);
    if (!std::filesystem::is_directory(result)) {
        result = result.parent_path();
    }
    delete[] out_str;
    return result;
}
void impl_gvox_unregister_format(GVoxContext *ctx, GVoxFormatLoader const &self) {
    // basically the destructor for format_loader
    if (self.context != nullptr) {
        ctx->errors.emplace_back("Failed to destroy the context of format [" + std::string(self.name_str) + "]", GVOX_ERROR_INVALID_FORMAT);
        self.destroy_context(self.context);
    }
}

#if GVOX_ENABLE_FILE_IO
static inline void gvox_save(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *dst_format, uint8_t is_raw) {
    GVoxHeader file_header;
    GVoxPayload file_payload;
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, dst_format);
    if (format_loader == nullptr) {
        return;
    }
    file_payload = format_loader->create_payload(format_loader->context, scene);
    auto file = std::ofstream(filepath, std::ios::binary);
    if (!file.is_open()) {
        format_loader->destroy_payload(format_loader->context, file_payload);
    }
    file_header.payload_size = file_payload.size;
    file_header.format_name_size = std::strlen(dst_format);
    if (is_raw == 0u) {
        file.write(reinterpret_cast<char const *>(&file_header), sizeof(file_header));
        file.write(reinterpret_cast<char const *>(dst_format), static_cast<std::streamsize>(file_header.format_name_size));
    }
    file.write(reinterpret_cast<char const *>(file_payload.data), static_cast<std::streamsize>(file_payload.size));
    file.close();
    format_loader->destroy_payload(format_loader->context, file_payload);
}
#endif

auto gvox_create_context(void) -> GVoxContext * {
    auto *result = new GVoxContext;
    for (const auto *const name : format_names) {
        gvox_load_format(result, name);
    }
    return result;
}
void gvox_destroy_context(GVoxContext *ctx) {
    if (ctx == nullptr) {
        return;
    }
    for (auto &[format_key, format_loader] : ctx->format_loader_table) {
        impl_gvox_unregister_format(ctx, *format_loader);
        delete format_loader;
    }
    delete ctx;
}

#if GVOX_ENABLE_FILE_IO
void gvox_push_root_path(GVoxContext *ctx, char const *path) {
    ctx->root_paths.emplace_back(path);
}
void gvox_pop_root_path(GVoxContext *ctx) {
    ctx->root_paths.pop_back();
}

auto gvox_load(GVoxContext *ctx, char const *filepath) -> GVoxScene {
    GVoxHeader file_header;
    GVoxPayload file_payload;
    struct {
        char *str;
        size_t str_size;
    } file_format_name{};
    auto file = std::ifstream(filepath, std::ios::binary);
    if (!file.is_open()) {
        for (auto const &root_path : ctx->root_paths) {
            file = std::ifstream(root_path / filepath, std::ios::binary);
            if (file.is_open()) {
                break;
            }
        }
    }
    if (!file.is_open()) {
        ctx->errors.emplace_back("Failed to load file [" + std::string(filepath) + "]", GVOX_ERROR_FAILED_TO_LOAD_FILE);
        return {};
    }
    file.read((char *)&file_header, sizeof(file_header));
    file_format_name.str_size = file_header.format_name_size;
    file_format_name.str = new char[file_format_name.str_size + 1];
    file.read(file_format_name.str, static_cast<std::streamsize>(file_format_name.str_size));
    file_format_name.str[file_format_name.str_size] = '\0';
    file_payload.size = file_header.payload_size;
    file_payload.data = new uint8_t[file_payload.size];
    file.read((char *)file_payload.data, static_cast<std::streamsize>(file_payload.size));
    file.close();
    auto result = gvox_parse(ctx, file_payload, file_format_name.str);
    delete[] file_format_name.str;
    delete[] file_payload.data;
    return result;
}
auto gvox_load_from_raw(GVoxContext *ctx, char const *filepath, char const *src_format) -> GVoxScene {
    GVoxPayload file_payload;
    auto file = std::ifstream(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        for (auto const &root_path : ctx->root_paths) {
            file = std::ifstream(root_path / filepath, std::ios::binary | std::ios::ate);
            if (file.is_open()) {
                break;
            }
        }
    }
    if (!file.is_open()) {
        ctx->errors.emplace_back("Failed to load file [" + std::string(filepath) + "]", GVOX_ERROR_FAILED_TO_LOAD_FILE);
        return {};
    }
    file_payload.size = static_cast<size_t>(file.tellg());
    file_payload.data = new uint8_t[file_payload.size];
    file.seekg(0, std::ios::beg);
    file.read((char *)file_payload.data, static_cast<std::streamsize>(file_payload.size));
    file.close();
    auto result = gvox_parse(ctx, file_payload, src_format);
    delete[] file_payload.data;
    return result;
}
void gvox_save(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *dst_format) {
    gvox_save(ctx, scene, filepath, dst_format, 0u);
}
void gvox_save_as_raw(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *dst_format) {
    gvox_save(ctx, scene, filepath, dst_format, 1u);
}
#endif

void gvox_register_format(GVoxContext *ctx, GVoxFormatLoader format_loader) {
    if (format_loader.create_context == nullptr ||
        format_loader.destroy_context == nullptr ||
        format_loader.create_payload == nullptr ||
        format_loader.destroy_payload == nullptr ||
        format_loader.parse_payload == nullptr) {
        ctx->errors.emplace_back("Failed to register format [" + std::string(format_loader.name_str) + "]", GVOX_ERROR_INVALID_FORMAT);
        return;
    }
    if (format_loader.context == nullptr) {
        format_loader.context = format_loader.create_context();
    }
    ctx->format_loader_table[format_loader.name_str] = new GVoxFormatLoader{format_loader};
}
void gvox_load_format(GVoxContext *ctx, char const *format_loader_name) {
    std::string filename = std::string("gvox_format_") + format_loader_name;

    auto static_format_iter = std::find_if(static_format_infos.begin(), static_format_infos.end(), [&](auto const &static_format_info) {
        return static_format_info.name == filename;
    });

    if (static_format_iter != static_format_infos.end()) {
        GVoxFormatLoader const format_loader = {
            .name_str = format_loader_name,
            .context = nullptr,
            .create_context = static_format_iter->create_context,
            .destroy_context = static_format_iter->destroy_context,
            .create_payload = static_format_iter->create_payload,
            .destroy_payload = static_format_iter->destroy_payload,
            .parse_payload = static_format_iter->parse_payload,
        };
        gvox_register_format(ctx, format_loader);
        return;
    }
#if GVOX_ENABLE_FILE_IO
    else {
        auto create_context_str = std::string("gvox_format_") + format_loader_name + "_create_context";
        auto destroy_context_str = std::string("gvox_format_") + format_loader_name + "_destroy_context";
        auto create_payload_str = std::string("gvox_format_") + format_loader_name + "_create_payload";
        auto destroy_payload_str = std::string("gvox_format_") + format_loader_name + "_destroy_payload";
        auto parse_payload_str = std::string("gvox_format_") + format_loader_name + "_parse_payload";
#if __linux__
        filename = "lib" + filename + ".so";
        void *so_handle = dlopen(filename.c_str(), RTLD_LAZY);
        if (!so_handle) {
            auto path = get_exe_path() / filename;
            so_handle = dlopen(path.string().c_str(), RTLD_LAZY);
        }
        if (!so_handle) {
            ctx->errors.push_back({"Failed to load Format .so at [" + filename + "]", GVOX_ERROR_FAILED_TO_LOAD_FORMAT});
            return;
        }
        GVoxFormatLoader format_loader = {
            .name_str = format_loader_name,
            .context = nullptr,
            .create_context = (GVoxFormatCreateContextFunc)dlsym(so_handle, create_context_str.c_str()),
            .destroy_context = (GVoxFormatDestroyContextFunc)dlsym(so_handle, destroy_context_str.c_str()),
            .create_payload = (GVoxFormatCreatePayloadFunc)dlsym(so_handle, create_payload_str.c_str()),
            .destroy_payload = (GVoxFormatDestroyPayloadFunc)dlsym(so_handle, destroy_payload_str.c_str()),
            .parse_payload = (GVoxFormatParsePayloadFunc)dlsym(so_handle, parse_payload_str.c_str()),
        };
#elif _WIN32
        filename = filename + ".dll";
        HINSTANCE dll_handle = LoadLibrary(filename.c_str());
        if (dll_handle == nullptr) {
            auto path = get_exe_path() / filename;
            dll_handle = LoadLibrary(path.string().c_str());
        }
        if (dll_handle == nullptr) {
            ctx->errors.emplace_back("Failed to load Format DLL at [" + filename + "]", GVOX_ERROR_FAILED_TO_LOAD_FORMAT);
            return;
        }
        GVoxFormatLoader const format_loader = {
            .name_str = format_loader_name,
            .context = nullptr,
            .create_context = (GVoxFormatCreateContextFunc)GetProcAddress(dll_handle, create_context_str.c_str()),
            .destroy_context = (GVoxFormatDestroyContextFunc)GetProcAddress(dll_handle, destroy_context_str.c_str()),
            .create_payload = (GVoxFormatCreatePayloadFunc)GetProcAddress(dll_handle, create_payload_str.c_str()),
            .destroy_payload = (GVoxFormatDestroyPayloadFunc)GetProcAddress(dll_handle, destroy_payload_str.c_str()),
            .parse_payload = (GVoxFormatParsePayloadFunc)GetProcAddress(dll_handle, parse_payload_str.c_str()),
        };
#endif
        gvox_register_format(ctx, format_loader);
        return;
    }
#endif

    ctx->errors.push_back({"Failed to load Format at [" + filename + "]", GVOX_ERROR_FAILED_TO_LOAD_FORMAT});
}

auto gvox_get_result(GVoxContext *ctx) -> GVoxResult {
    if (ctx->errors.empty()) {
        return GVOX_SUCCESS;
    }
    auto [msg, id] = ctx->errors.back();
    return id;
}
void gvox_get_result_message(GVoxContext *ctx, char *const str_buffer, size_t *str_size) {
    if (str_buffer != nullptr) {
        assert(str_size);
        if (ctx->errors.empty()) {
            for (size_t i = 0; i < *str_size; ++i) {
                str_buffer[i] = '\0';
            }
            return;
        }
        auto [msg, id] = ctx->errors.back();
        assert(msg.size() <= *str_size);
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
void gvox_pop_result(GVoxContext *ctx) {
    ctx->errors.pop_back();
}

auto gvox_parse(GVoxContext *ctx, GVoxPayload payload, char const *src_format) -> GVoxScene {
    GVoxScene result = {};
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, src_format);
    if (format_loader != nullptr) {
        result = format_loader->parse_payload(format_loader->context, payload);
    }
    return result;
}
auto gvox_serialize(GVoxContext *ctx, GVoxScene scene, char const *dst_format) -> GVoxPayload {
    GVoxPayload result = {};
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, dst_format);
    if (format_loader != nullptr) {
        result = format_loader->create_payload(format_loader->context, scene);
    }
    return result;
}

void gvox_destroy_payload(GVoxContext *ctx, GVoxPayload payload, char const *format) {
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, format);
    if (format_loader != nullptr) {
        format_loader->destroy_payload(format_loader->context, payload);
    }
}
void gvox_destroy_scene(GVoxScene scene) {
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels != nullptr) {
            std::free(scene.nodes[node_i].voxels);
        }
    }
    if (scene.nodes != nullptr) {
        std::free(scene.nodes);
    }
}
