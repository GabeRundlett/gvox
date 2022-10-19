#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <gvox/gvox.h>

#include <unordered_map>
#include <string>
#include <fstream>
#include <filesystem>

#if __linux__

#elif _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

struct _GVoxContext {
    std::unordered_map<std::string, GVoxFormatLoader *> format_loader_table = {};
};

static GVoxFormatLoader *gvox_context_find_loader(GVoxContext *ctx, std::string const &format_name) {
    return ctx->format_loader_table.at(format_name);
}

std::filesystem::path get_exe_path() {
#if __linux__
    return {};
#elif _WIN32
    char *out_str = new char[MAX_PATH];
    GetModuleFileName(NULL, out_str, MAX_PATH);
    return std::filesystem::path(out_str);
#endif
}

GVoxContext *gvox_create_context(void) {
    GVoxContext *result = new GVoxContext;
    gvox_load_format(result, "gvox_simple");
    gvox_load_format(result, "magicavoxel");
    return result;
}

void gvox_destroy_context(GVoxContext *ctx) {
    if (!ctx)
        return;
    delete ctx;
}

void gvox_register_format(GVoxContext *ctx, GVoxFormatLoader format_loader) {
    assert(format_loader.create_payload != nullptr);
    assert(format_loader.destroy_payload != nullptr);
    assert(format_loader.parse_payload != nullptr);
    ctx->format_loader_table[format_loader.format_name] = new GVoxFormatLoader{format_loader};
}

void gvox_load_format(GVoxContext *ctx, char const *format_loader_name) {
    using GVoxCreatePayloadFunc = GVoxPayload (*)(GVoxScene);
    using GVoxDestroyPayloadFunc = void (*)(GVoxPayload);
    using GVoxParsePayloadFunc = GVoxScene (*)(GVoxPayload);

#if __linux__
    void *so_handle = dlopen(format_loader_name, RTLD_LAZY);
    if (!so_handle) {
        auto path = get_exe_path() / format_loader_name;
        so_handle = dlopen(path.string().c_str(), RTLD_LAZY);
    }
    assert(so_handle != nullptr);
    GVoxFormatLoader format_loader = {
        .format_name = format_loader_name,
        .create_payload = (GVoxCreatePayloadFunc)dlsym(so_handle, "gvox_create_payload"),
        .destroy_payload = (GVoxDestroyPayloadFunc)dlsym(so_handle, "gvox_destroy_payload"),
        .parse_payload = (GVoxParsePayloadFunc)dlsym(so_handle, "gvox_parse_payload"),
    };
#elif _WIN32
    HINSTANCE dll_handle = LoadLibrary(format_loader_name);
    if (!dll_handle) {
        auto path = get_exe_path() / format_loader_name;
        dll_handle = LoadLibrary(path.string().c_str());
    }
    assert(dll_handle != nullptr);
    GVoxFormatLoader format_loader = {
        .format_name = format_loader_name,
        .create_payload = (GVoxCreatePayloadFunc)GetProcAddress(dll_handle, "gvox_create_payload"),
        .destroy_payload = (GVoxDestroyPayloadFunc)GetProcAddress(dll_handle, "gvox_destroy_payload"),
        .parse_payload = (GVoxParsePayloadFunc)GetProcAddress(dll_handle, "gvox_parse_payload"),
    };
#endif
    gvox_register_format(ctx, format_loader);
}

GVoxScene gvox_load(GVoxContext *ctx, char const *filepath) {
    GVoxScene result = {0};
    GVoxHeader file_header;
    GVoxPayload file_payload;
    struct {
        char *str;
        size_t str_size;
    } file_format_name;
    auto file = std::ifstream(filepath, std::ios::binary);
    assert(file.is_open());
    file.read((char *)&file_header, sizeof(file_header));
    file_format_name.str_size = file_header.format_name_size;
    file_format_name.str = new char[file_format_name.str_size + 1];
    file.read(file_format_name.str, file_format_name.str_size);
    file_format_name.str[file_format_name.str_size] = '\0';
    file_payload.size = file_header.payload_size;
    file_payload.data = new uint8_t[file_payload.size];
    file.read((char *)file_payload.data, file_payload.size);
    file.close();
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, file_format_name.str);
    if (format_loader)
        result = format_loader->parse_payload(file_payload);
    delete[] file_format_name.str;
    delete[] file_payload.data;
    return result;
}

GVoxScene gvox_load_raw(GVoxContext *ctx, char const *filepath, char const *format) {
    GVoxScene result = {0};
    GVoxPayload file_payload;
    auto file = std::ifstream(filepath, std::ios::binary | std::ios::ate);
    assert(file.is_open());
    file_payload.size = static_cast<size_t>(file.tellg());
    file_payload.data = new uint8_t[file_payload.size];
    file.read((char *)file_payload.data, file_payload.size);
    file.close();
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, format);
    if (format_loader)
        result = format_loader->parse_payload(file_payload);
    delete[] file_payload.data;
    return result;
}

static inline void _gvox_save(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *format, uint8_t is_raw) {
    GVoxHeader file_header;
    GVoxPayload file_payload;
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, format);
    if (!format_loader)
        return;
    // printf("creating payload (used for saving)\n");
    file_payload = format_loader->create_payload(scene);
    auto file = std::ofstream(filepath, std::ios::binary);
    if (!file.is_open())
        goto cleanup_payload;
    file_header.payload_size = file_payload.size;
    file_header.format_name_size = strlen(format);
    // printf("saving gvox file header with format_name_size %zu and payload_size %zu\n", file_header.format_name_size, file_header.payload_size);
    if (!is_raw) {
        file.write(reinterpret_cast<char const *>(&file_header), sizeof(file_header));
        file.write(reinterpret_cast<char const *>(format), file_header.format_name_size);
    }
    file.write(reinterpret_cast<char const *>(file_payload.data), file_payload.size);
    file.close();
cleanup_payload:
    // printf("destroying payload (used for saving)\n");
    format_loader->destroy_payload(file_payload);
}

void gvox_save(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *format) {
    _gvox_save(ctx, scene, filepath, format, false);
}

void gvox_save_raw(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *format) {
    _gvox_save(ctx, scene, filepath, format, true);
}

void gvox_destroy_scene(GVoxScene scene) {
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels)
            free(scene.nodes[node_i].voxels);
    }
    if (scene.nodes)
        free(scene.nodes);
}