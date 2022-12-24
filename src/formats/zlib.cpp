#include <gvox/gvox.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <algorithm>
#include <iostream>

#include <zlib.h>

#if GVOX_FORMAT_ZLIB_BUILT_STATIC
#define EXPORT
#else
#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport)
#endif
#endif

#define OUTPUT_RAW 0

struct ZlibContext {
    ZlibContext();
    ~ZlibContext() = default;

    static auto create_payload(GVoxScene scene) -> GVoxPayload;
    static void destroy_payload(GVoxPayload payload);
    static auto parse_payload(GVoxPayload payload) -> GVoxScene;
};

ZlibContext::ZlibContext() = default;

auto ZlibContext::create_payload(GVoxScene scene) -> GVoxPayload {
    GVoxPayload result = {};
#if OUTPUT_RAW
    result.size += sizeof(size_t);

    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels == nullptr) {
            continue;
        }
        result.size += sizeof(size_t) * 3;
        result.size += scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z * sizeof(GVoxVoxel);
    }
    result.data = new uint8_t[result.size];
    auto *buffer_ptr = (uint8_t *)result.data;
    std::memcpy(buffer_ptr, &scene.node_n, sizeof(scene.node_n));
    buffer_ptr += sizeof(scene.node_n);
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels == nullptr) {
            continue;
        }
        std::memcpy(buffer_ptr, &scene.nodes[node_i].size_x, sizeof(scene.nodes[node_i].size_x));
        buffer_ptr += sizeof(scene.nodes[node_i].size_x);
        std::memcpy(buffer_ptr, &scene.nodes[node_i].size_y, sizeof(scene.nodes[node_i].size_y));
        buffer_ptr += sizeof(scene.nodes[node_i].size_y);
        std::memcpy(buffer_ptr, &scene.nodes[node_i].size_z, sizeof(scene.nodes[node_i].size_z));
        buffer_ptr += sizeof(scene.nodes[node_i].size_z);
        size_t const voxels_size = scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z * sizeof(GVoxVoxel);
        std::memcpy(buffer_ptr, scene.nodes[node_i].voxels, voxels_size);
        buffer_ptr += voxels_size;
    }
#else
    result.size += sizeof(size_t);
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels == nullptr) {
            continue;
        }
        result.size += sizeof(size_t) * 3;
        result.size += scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z * sizeof(uint32_t);
    }
    result.data = new uint8_t[result.size];
    auto *buffer_ptr = (uint8_t *)result.data;
    std::memcpy(buffer_ptr, &scene.node_n, sizeof(scene.node_n));
    buffer_ptr += sizeof(scene.node_n);
    for (size_t node_i = 0; node_i < scene.node_n; ++node_i) {
        if (scene.nodes[node_i].voxels == nullptr) {
            continue;
        }
        std::memcpy(buffer_ptr, &scene.nodes[node_i].size_x, sizeof(scene.nodes[node_i].size_x));
        buffer_ptr += sizeof(scene.nodes[node_i].size_x);
        std::memcpy(buffer_ptr, &scene.nodes[node_i].size_y, sizeof(scene.nodes[node_i].size_y));
        buffer_ptr += sizeof(scene.nodes[node_i].size_y);
        std::memcpy(buffer_ptr, &scene.nodes[node_i].size_z, sizeof(scene.nodes[node_i].size_z));
        buffer_ptr += sizeof(scene.nodes[node_i].size_z);
        size_t const voxels_n = scene.nodes[node_i].size_x * scene.nodes[node_i].size_y * scene.nodes[node_i].size_z;
        size_t const voxels_size = voxels_n * sizeof(uint32_t);
        for (size_t voxel_i = 0; voxel_i < voxels_n; ++voxel_i) {
            auto const &i_vox = scene.nodes[node_i].voxels[voxel_i];
            uint32_t u32_voxel = 0;
            uint32_t const r = static_cast<uint32_t>(std::max(std::min(i_vox.color.x, 1.0f), 0.0f) * 255.0f);
            uint32_t const g = static_cast<uint32_t>(std::max(std::min(i_vox.color.y, 1.0f), 0.0f) * 255.0f);
            uint32_t const b = static_cast<uint32_t>(std::max(std::min(i_vox.color.z, 1.0f), 0.0f) * 255.0f);
            uint32_t const i = i_vox.id;
            u32_voxel = u32_voxel | (r << 0x00);
            u32_voxel = u32_voxel | (g << 0x08);
            u32_voxel = u32_voxel | (b << 0x10);
            u32_voxel = u32_voxel | (i << 0x18);
            std::memcpy(buffer_ptr + voxel_i * sizeof(uint32_t), &u32_voxel, sizeof(uint32_t));
        }
        buffer_ptr += voxels_size;
    }
#endif

    auto uncompressed_size = result.size;

    auto *compressed = new uint8_t[result.size];

    z_stream defstream{};
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    defstream.avail_in = (uInt)result.size;   // size of input, string + terminator
    defstream.next_in = (Bytef *)result.data; // input char array
    defstream.avail_out = (uInt)result.size;  // size of output
    defstream.next_out = (Bytef *)compressed; // output char array
    deflateInit(&defstream, Z_BEST_COMPRESSION);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);

    assert(defstream.msg == nullptr);

    size_t const compressed_size = defstream.total_out;

    // std::cout << uncompressed_size << ", " << compressed_size << std::endl;

    delete[] result.data;
    result.data = new uint8_t[compressed_size + sizeof(size_t)];
    result.size = compressed_size + sizeof(size_t);

    *reinterpret_cast<size_t *>(result.data) = uncompressed_size;
    std::copy(compressed, compressed + compressed_size, result.data + sizeof(size_t));
    delete[] compressed;

    return result;
}

void ZlibContext::destroy_payload(GVoxPayload payload) {
    delete[] payload.data;
}

auto ZlibContext::parse_payload(GVoxPayload payload) -> GVoxScene {
    GVoxScene result = {};
    auto *buffer_ptr = (uint8_t *)payload.data;
    auto *buffer_sentinel = buffer_ptr + payload.size;
    uint8_t *uncompressed_data = nullptr;
    {
        auto uncompressed_size = *reinterpret_cast<size_t *>(buffer_ptr);
        auto compressed_size = payload.size - sizeof(size_t);
        // std::cout << uncompressed_size << ", " << compressed_size << std::endl;
        buffer_ptr += sizeof(size_t);
        uncompressed_data = new uint8_t[uncompressed_size];

        z_stream infstream;
        infstream.zalloc = Z_NULL;
        infstream.zfree = Z_NULL;
        infstream.opaque = Z_NULL;
        infstream.avail_in = (uInt)compressed_size;      // size of input
        infstream.next_in = (Bytef *)buffer_ptr;         // input char array
        infstream.avail_out = (uInt)uncompressed_size;   // size of output
        infstream.next_out = (Bytef *)uncompressed_data; // output char array
        inflateInit(&infstream);
        inflate(&infstream, Z_NO_FLUSH);
        inflateEnd(&infstream);

        assert(infstream.msg == nullptr);
        assert(infstream.total_out == uncompressed_size);

        buffer_ptr = uncompressed_data;
        buffer_sentinel = buffer_ptr + uncompressed_size;
    }
#if OUTPUT_RAW
    result.node_n = *(size_t *)buffer_ptr;
    buffer_ptr += sizeof(result.node_n);
    result.nodes = (GVoxSceneNode *)std::malloc(sizeof(GVoxSceneNode) * result.node_n);
    size_t node_i = 0;
    while (buffer_ptr < buffer_sentinel) {
        result.nodes[node_i].size_x = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_x);
        result.nodes[node_i].size_y = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_y);
        result.nodes[node_i].size_z = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_z);
        size_t const voxels_size = result.nodes[node_i].size_x * result.nodes[node_i].size_y * result.nodes[node_i].size_z * sizeof(GVoxVoxel);
        result.nodes[node_i].voxels = (GVoxVoxel *)std::malloc(voxels_size);
        std::memcpy(result.nodes[node_i].voxels, buffer_ptr, voxels_size);
        buffer_ptr += voxels_size;
        ++node_i;
    }
#else
    result.node_n = *(size_t *)buffer_ptr;
    buffer_ptr += sizeof(result.node_n);
    result.nodes = (GVoxSceneNode *)std::malloc(sizeof(GVoxSceneNode) * result.node_n);
    size_t node_i = 0;
    while (buffer_ptr < buffer_sentinel) {
        result.nodes[node_i].size_x = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_x);
        result.nodes[node_i].size_y = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_y);
        result.nodes[node_i].size_z = *(size_t *)buffer_ptr;
        buffer_ptr += sizeof(result.nodes[node_i].size_z);
        size_t const voxels_n = result.nodes[node_i].size_x * result.nodes[node_i].size_y * result.nodes[node_i].size_z;
        size_t const voxels_size = voxels_n * sizeof(GVoxVoxel);
        result.nodes[node_i].voxels = (GVoxVoxel *)std::malloc(voxels_size);
        for (size_t voxel_i = 0; voxel_i < voxels_n; ++voxel_i) {
            uint32_t u32_voxel = 0;
            std::memcpy(&u32_voxel, buffer_ptr + voxel_i * sizeof(uint32_t), sizeof(uint32_t));
            float r = static_cast<float>((u32_voxel >> 0x00) & 0xff) / 255.0f;
            float g = static_cast<float>((u32_voxel >> 0x08) & 0xff) / 255.0f;
            float b = static_cast<float>((u32_voxel >> 0x10) & 0xff) / 255.0f;
            uint32_t const i = (u32_voxel >> 0x18) & 0xff;
            result.nodes[node_i].voxels[voxel_i] = GVoxVoxel{
                .color = {r, g, b},
                .id = i,
            };
        }
        buffer_ptr += voxels_size;
        ++node_i;
    }
#endif
    delete[] uncompressed_data;
    return result;
}

extern "C" EXPORT auto gvox_format_zlib_create_context() -> void * {
    auto *result = new ZlibContext{};
    return result;
}

extern "C" EXPORT void gvox_format_zlib_destroy_context(void *context_ptr) {
    auto *self = reinterpret_cast<ZlibContext *>(context_ptr);
    delete self;
}

extern "C" EXPORT auto gvox_format_zlib_create_payload(void *context_ptr, GVoxScene scene) -> GVoxPayload {
    auto *self = reinterpret_cast<ZlibContext *>(context_ptr);
    return self->create_payload(scene);
}

extern "C" EXPORT void gvox_format_zlib_destroy_payload(void *context_ptr, GVoxPayload payload) {
    auto *self = reinterpret_cast<ZlibContext *>(context_ptr);
    self->destroy_payload(payload);
}

extern "C" EXPORT auto gvox_format_zlib_parse_payload(void *context_ptr, GVoxPayload payload) -> GVoxScene {
    auto *self = reinterpret_cast<ZlibContext *>(context_ptr);
    return self->parse_payload(payload);
}
