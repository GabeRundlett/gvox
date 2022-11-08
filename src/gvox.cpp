#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <gvox/gvox.h>

#include <unordered_map>
#include <string>
#include <fstream>
#include <filesystem>
#include <vector>

#if __linux__
#include <unistd.h>
#include <dlfcn.h>
#elif _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

struct _GVoxContext {
    std::unordered_map<std::string, GVoxFormatLoader *> format_loader_table = {};
    std::vector<std::filesystem::path> root_paths = {};
};

static GVoxFormatLoader *gvox_context_find_loader(GVoxContext *ctx, std::string const &format_name) {
    return ctx->format_loader_table.at(format_name);
}

std::filesystem::path get_exe_path() {
    char *out_str = new char[512];
    for (size_t i = 0; i < 512; ++i)
        out_str[i] = '\0';
#if __linux__
    readlink("/proc/self/exe", out_str, 511);
#elif _WIN32
    GetModuleFileName(NULL, out_str, 511);
#endif
    auto result = std::filesystem::path(out_str);
    if (!std::filesystem::is_directory(result))
        result = result.parent_path();
    delete[] out_str;
    return result;
}

GVoxContext *gvox_create_context(void) {
    GVoxContext *result = new GVoxContext;
    gvox_load_format(result, "gvox_simple");
    gvox_load_format(result, "gvox_u32");
    gvox_load_format(result, "gvox_u32_palette");
    gvox_load_format(result, "magicavoxel");
    return result;
}

void impl_gvox_unregister_format(GVoxFormatLoader const &self) {
    // basically the destructor for format_loader
    if (self.context) {
        assert(self.destroy_context != nullptr);
        self.destroy_context(self.context);
    }
}

void gvox_destroy_context(GVoxContext *ctx) {
    if (!ctx)
        return;
    for (auto &[format_key, format_loader] : ctx->format_loader_table) {
        impl_gvox_unregister_format(*format_loader);
    }
    delete ctx;
}

void gvox_register_format(GVoxContext *ctx, GVoxFormatLoader format_loader) {
    assert(format_loader.create_context != nullptr);
    assert(format_loader.destroy_context != nullptr);
    assert(format_loader.create_payload != nullptr);
    assert(format_loader.destroy_payload != nullptr);
    assert(format_loader.parse_payload != nullptr);
    if (format_loader.context == nullptr) {
        format_loader.context = format_loader.create_context();
    }
    ctx->format_loader_table[format_loader.name_str] = new GVoxFormatLoader{format_loader};
}

void gvox_load_format(GVoxContext *ctx, char const *format_loader_name) {
    using GVoxFormatCreateContextFunc = void *(*)();
    using GVoxFormatDestroyContextFunc = void (*)(void *);
    using GVoxFormatCreatePayloadFunc = GVoxPayload (*)(void *, GVoxScene);
    using GVoxFormatDestroyPayloadFunc = void (*)(void *, GVoxPayload);
    using GVoxFormatParsePayloadFunc = GVoxScene (*)(void *, GVoxPayload);

    std::string filename = std::string("gvox_format_") + format_loader_name;

#if __linux__
    filename = "lib" + filename + ".so";
    void *so_handle = dlopen(filename.c_str(), RTLD_LAZY);
    if (!so_handle) {
        auto path = get_exe_path() / filename;
        so_handle = dlopen(path.string().c_str(), RTLD_LAZY);
    }
    if (!so_handle) {
        return;
    }
    GVoxFormatLoader format_loader = {
        .format_name = format_loader_name,
        .create_context = (GVoxFormatCreateContextFunc)dlsym(so_handle, "gvox_format_create_context"),
        .destroy_context = (GVoxFormatDestroyContextFunc)dlsym(so_handle, "gvox_format_destroy_context"),
        .create_payload = (GVoxFormatCreatePayloadFunc)dlsym(so_handle, "gvox_format_create_payload"),
        .destroy_payload = (GVoxFormatDestroyPayloadFunc)dlsym(so_handle, "gvox_format_destroy_payload"),
        .parse_payload = (GVoxFormatParsePayloadFunc)dlsym(so_handle, "gvox_format_parse_payload"),
    };
#elif _WIN32
    filename = filename + ".dll";
    HINSTANCE dll_handle = LoadLibrary(filename.c_str());
    if (!dll_handle) {
        auto path = get_exe_path() / filename;
        dll_handle = LoadLibrary(path.string().c_str());
    }
    if (!dll_handle) {
        return;
    }
    GVoxFormatLoader format_loader = {
        .name_str = format_loader_name,
        .create_context = (GVoxFormatCreateContextFunc)GetProcAddress(dll_handle, "gvox_format_create_context"),
        .destroy_context = (GVoxFormatDestroyContextFunc)GetProcAddress(dll_handle, "gvox_format_destroy_context"),
        .create_payload = (GVoxFormatCreatePayloadFunc)GetProcAddress(dll_handle, "gvox_format_create_payload"),
        .destroy_payload = (GVoxFormatDestroyPayloadFunc)GetProcAddress(dll_handle, "gvox_format_destroy_payload"),
        .parse_payload = (GVoxFormatParsePayloadFunc)GetProcAddress(dll_handle, "gvox_format_parse_payload"),
    };
#endif
    gvox_register_format(ctx, format_loader);
}

void gvox_push_root_path(GVoxContext *ctx, char const *path) {
    ctx->root_paths.push_back(path);
}

void gvox_pop_root_path(GVoxContext *ctx) {
    ctx->root_paths.pop_back();
}

GVoxScene gvox_load(GVoxContext *ctx, char const *filepath) {
    GVoxScene result = {};
    GVoxHeader file_header;
    GVoxPayload file_payload;
    struct {
        char *str;
        size_t str_size;
    } file_format_name;
    auto file = std::ifstream(filepath, std::ios::binary);
    if (!file.is_open()) {
        for (auto const &root_path : ctx->root_paths) {
            file = std::ifstream(root_path / filepath, std::ios::binary);
            if (file.is_open())
                break;
        }
    }
    assert(file.is_open());
    file.read((char *)&file_header, sizeof(file_header));
    file_format_name.str_size = file_header.format_name_size;
    file_format_name.str = new char[file_format_name.str_size + 1];
    file.read(file_format_name.str, static_cast<std::streamsize>(file_format_name.str_size));
    file_format_name.str[file_format_name.str_size] = '\0';
    file_payload.size = file_header.payload_size;
    file_payload.data = new uint8_t[file_payload.size];
    file.read((char *)file_payload.data, static_cast<std::streamsize>(file_payload.size));
    file.close();
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, file_format_name.str);
    if (format_loader)
        result = format_loader->parse_payload(format_loader->context, file_payload);
    delete[] file_format_name.str;
    delete[] file_payload.data;
    return result;
}

GVoxScene gvox_load_raw(GVoxContext *ctx, char const *filepath, char const *format) {
    GVoxScene result = {};
    GVoxPayload file_payload;
    auto file = std::ifstream(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        for (auto const &root_path : ctx->root_paths) {
            file = std::ifstream(root_path / filepath, std::ios::binary | std::ios::ate);
            if (file.is_open())
                break;
        }
    }
    if (!file.is_open()) {
        // TODO: Error handling
        return result;
    }
    file_payload.size = static_cast<size_t>(file.tellg());
    file_payload.data = new uint8_t[file_payload.size];
    file.seekg(0, std::ios::beg);
    file.read((char *)file_payload.data, static_cast<std::streamsize>(file_payload.size));
    file.close();
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, format);
    if (format_loader)
        result = format_loader->parse_payload(format_loader->context, file_payload);
    delete[] file_payload.data;
    return result;
}

static inline void _gvox_save(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *format, uint8_t is_raw) {
    GVoxHeader file_header;
    GVoxPayload file_payload;
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, format);
    if (!format_loader)
        return;
    file_payload = format_loader->create_payload(format_loader->context, scene);
    auto file = std::ofstream(filepath, std::ios::binary);
    if (!file.is_open())
        goto cleanup_payload;
    file_header.payload_size = file_payload.size;
    file_header.format_name_size = strlen(format);
    if (!is_raw) {
        file.write(reinterpret_cast<char const *>(&file_header), sizeof(file_header));
        file.write(reinterpret_cast<char const *>(format), static_cast<std::streamsize>(file_header.format_name_size));
    }
    file.write(reinterpret_cast<char const *>(file_payload.data), static_cast<std::streamsize>(file_payload.size));
    file.close();
cleanup_payload:
    format_loader->destroy_payload(format_loader->context, file_payload);
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
