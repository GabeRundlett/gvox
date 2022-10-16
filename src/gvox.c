#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gvox/gvox.h>
#include <glib.h>

#include "formats/gvox_simple.h"
#include "formats/magicavoxel.h"

typedef struct {
    char *str;
    size_t str_size;
} GVoxFormatName;

struct _GVoxContext {
    GHashTable *format_loader_table;
};

static GVoxFormatLoader *gvox_context_find_loader(GVoxContext *ctx, char const *format_name) {
    GVoxFormatLoader *result = g_hash_table_lookup(ctx->format_loader_table, format_name);
    return result;
}

static void gvox_destroy_loader_value(GVoxFormatLoader *value) {
    // printf("destroying gvox format loader '%s'\n", value->format_name);
    free(value);
}

GVoxContext *gvox_create_context(void) {
    GVoxContext *result = malloc(sizeof(GVoxContext));
    result->format_loader_table = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, (GDestroyNotify)gvox_destroy_loader_value);
    {
        GVoxFormatLoader format_loader = {
            .format_name = "gvox_simple",
            .create_payload = gvox_simple_create_payload,
            .destroy_payload = gvox_simple_destroy_payload,
            .parse_payload = gvox_simple_parse_payload,
        };
        gvox_register_format(result, format_loader);
    }
    {
        GVoxFormatLoader format_loader = {
            .format_name = "magicavoxel",
            .create_payload = magicavoxel_create_payload,
            .destroy_payload = magicavoxel_destroy_payload,
            .parse_payload = magicavoxel_parse_payload,
        };
        gvox_register_format(result, format_loader);
    }

    return result;
}

void gvox_destroy_context(GVoxContext *ctx) {
    if (!ctx)
        return;
    g_hash_table_destroy(ctx->format_loader_table);
    free(ctx);
}

void gvox_register_format(GVoxContext *ctx, GVoxFormatLoader format_loader) {
    GVoxFormatLoader *value = malloc(sizeof(GVoxFormatLoader));
    memcpy(value, &format_loader, sizeof(GVoxFormatLoader));
    // printf("creating gvox format loader '%s'\n", value->format_name);
    g_hash_table_insert(ctx->format_loader_table, (void *)format_loader.format_name, value);
}

GVoxScene gvox_load(GVoxContext *ctx, char const *filepath) {
    GVoxScene result = {0};
    GVoxHeader file_header;
    GVoxPayload file_payload;
    GVoxFormatName file_format_name;
    FILE *fp;
    errno_t file_open_result = fopen_s(&fp, filepath, "rb+");
    if (fp == NULL || file_open_result != 0)
        return result;
    fread(&file_header, sizeof(file_header), 1, fp);
    file_format_name.str_size = file_header.format_name_size;
    file_format_name.str = malloc(file_format_name.str_size + 1);
    fread(file_format_name.str, file_format_name.str_size, 1, fp);
    file_format_name.str[file_format_name.str_size] = '\0';
    file_payload.size = file_header.payload_size;
    file_payload.data = malloc(file_payload.size);
    fread(file_payload.data, file_payload.size, 1, fp);
    fclose(fp);
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, file_format_name.str);
    if (format_loader)
        result = format_loader->parse_payload(file_payload);
    free(file_format_name.str);
    free(file_payload.data);
    return result;
}

GVoxScene gvox_load_raw(GVoxContext *ctx, char const *filepath, char const *format) {
    GVoxScene result = {0};
    FILE *fp;
    errno_t file_open_result = fopen_s(&fp, filepath, "rb+");
    if (fp == NULL || file_open_result != 0)
        return result;
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    GVoxPayload file_payload;
    file_payload.size = (size_t)file_size;
    file_payload.data = malloc(file_payload.size);
    fread(file_payload.data, file_payload.size, 1, fp);
    fclose(fp);
    GVoxFormatLoader *format_loader = gvox_context_find_loader(ctx, format);
    if (format_loader)
        result = format_loader->parse_payload(file_payload);
    free(file_payload.data);
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
    FILE *fp;
    errno_t file_open_result = fopen_s(&fp, filepath, "wb+");
    if (fp == NULL || file_open_result != 0)
        goto cleanup_payload;
    file_header.payload_size = file_payload.size;
    file_header.format_name_size = strlen(format);
    // printf("saving gvox file header with format_name_size %lld and payload_size %lld\n", file_header.format_name_size, file_header.payload_size);
    if (!is_raw) {
        fwrite(&file_header, sizeof(file_header), 1, fp);
        fwrite(format, file_header.format_name_size, 1, fp);
    }
    fwrite(file_payload.data, file_payload.size, 1, fp);
    fclose(fp);
cleanup_payload:
    // printf("destroying payload (used for saving)\n");
    format_loader->destroy_payload(file_payload);
}

void gvox_save(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *format) {
    _gvox_save(ctx, scene, filepath, format, 0);
}

void gvox_save_raw(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *format) {
    _gvox_save(ctx, scene, filepath, format, 1);
}

void gvox_destroy_scene(GVoxScene scene) {
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels)
            free(scene.nodes[node_i].voxels);
    }
    if (scene.nodes)
        free(scene.nodes);
}
