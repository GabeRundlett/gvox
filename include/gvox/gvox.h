#ifndef GVOX_H
#define GVOX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct {
    float x;
    float y;
    float z;
} GVoxf32vec3;

typedef struct {
    GVoxf32vec3 color;
    uint32_t id;
} GVoxVoxel;

typedef struct {
    // TODO: add a transform
    size_t size_x, size_y, size_z;
    GVoxVoxel *voxels;
} GVoxSceneNode;

typedef struct {
    size_t node_n;
    GVoxSceneNode *nodes;
} GVoxScene;

typedef struct {
    size_t format_name_size;
    size_t payload_size;
} GVoxHeader;

typedef struct {
    size_t size;
    void *data;
} GVoxPayload;

typedef struct {
    char const *format_name;
    GVoxPayload (*create_payload)(GVoxScene scene);
    void (*destroy_payload)(GVoxPayload payload);
    GVoxScene (*parse_payload)(GVoxPayload payload);
} GVoxFormatLoader;

typedef struct _GVoxContext GVoxContext;

GVoxContext *gvox_create_context(void);
void gvox_destroy_context(GVoxContext *ctx);
GVoxScene gvox_load(GVoxContext *ctx, char const *filepath);
GVoxScene gvox_load_raw(GVoxContext *ctx, char const *filepath, char const *format);
void gvox_save(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *format);
void gvox_save_raw(GVoxContext *ctx, GVoxScene scene, char const *filepath, char const *format);
void gvox_destroy_scene(GVoxScene scene);
void gvox_register_format(GVoxContext *ctx, GVoxFormatLoader format_loader);

#ifdef __cplusplus
}
#endif

#endif
