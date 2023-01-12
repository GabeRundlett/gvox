#include <gvox/gvox.h>

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cmath>

#include <array>
#include <vector>
#include <algorithm>
#include <set>

#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.hpp"

#if GVOX_FORMAT_MAGICAVOXEL_BUILT_STATIC
#define EXPORT
#else
#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport)
#endif
#endif

struct MagicavoxelVoxel {
    uint8_t x, y, z, color_index;
};
struct MagicavoxelChunkID {
    uint8_t b0, b1, b2, b3;
};
using Palette = std::array<uint32_t, 256>;
// static constexpr Palette default_palette = {
//     0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
//     0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
//     0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
//     0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
//     0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
//     0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
//     0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
//     0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
//     0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
//     0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
//     0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
//     0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
//     0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
//     0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
//     0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
//     0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111};
// static const MagicavoxelChunkID CHUNK_VOX_ = {'V', 'O', 'X', ' '};
static const MagicavoxelChunkID CHUNK_MAIN = {'M', 'A', 'I', 'N'};
static const MagicavoxelChunkID CHUNK_SIZE = {'S', 'I', 'Z', 'E'};
static const MagicavoxelChunkID CHUNK_XYZI = {'X', 'Y', 'Z', 'I'};
static const MagicavoxelChunkID CHUNK_RGBA = {'R', 'G', 'B', 'A'};
// static const MagicavoxelChunkID CHUNK_nTRN = {'n', 'T', 'R', 'N'};
// static const MagicavoxelChunkID CHUNK_nGRP = {'n', 'G', 'R', 'P'};
// static const MagicavoxelChunkID CHUNK_nSHP = {'n', 'S', 'H', 'P'};
// static const MagicavoxelChunkID CHUNK_IMAP = {'I', 'M', 'A', 'P'};
// static const MagicavoxelChunkID CHUNK_LAYR = {'L', 'A', 'Y', 'R'};
// static const MagicavoxelChunkID CHUNK_MATL = {'M', 'A', 'T', 'L'};
// static const MagicavoxelChunkID CHUNK_MATT = {'M', 'A', 'T', 'T'};
// static const MagicavoxelChunkID CHUNK_rOBJ = {'r', 'O', 'B', 'J'};
// static const MagicavoxelChunkID CHUNK_rCAM = {'r', 'C', 'A', 'M'};
template <typename T>
static void write_data(uint8_t **buffer_ptr, T const &data) {
    *reinterpret_cast<T *>(*buffer_ptr) = data;
    *buffer_ptr += sizeof(T);
}

struct MagicavoxelContext {
    MagicavoxelContext();
    ~MagicavoxelContext() = default;

    static auto create_payload(GVoxScene scene) -> GVoxPayload;
    static void destroy_payload(GVoxPayload payload);
    static auto parse_payload(GVoxPayload payload) -> GVoxScene;
};

MagicavoxelContext::MagicavoxelContext() = default;

GVoxVoxel rgb2hsv(GVoxVoxel c) {
    // return c;
    float kx = 0.0f;
    float ky = -1.0f / 3.0f;
    float kz = 2.0f / 3.0f;
    float kw = -1.0f;

    auto step = [](float y, float x) -> float {
        return static_cast<float>(x >= y);
    };

    float px = std::lerp(c.color.z, c.color.y, step(c.color.z, c.color.y));
    float py = std::lerp(c.color.y, c.color.z, step(c.color.z, c.color.y));
    float pz = std::lerp(kw, kx, step(c.color.z, c.color.y));
    float pw = std::lerp(kz, ky, step(c.color.z, c.color.y));

    float qx = std::lerp(px, c.color.x, step(px, c.color.x));
    float qy = std::lerp(py, py, step(px, c.color.x));
    float qz = std::lerp(pw, pz, step(px, c.color.x));
    float qw = std::lerp(c.color.x, px, step(px, c.color.x));

    float d = qx - std::min(qw, qy);
    float e = 1.0e-10f;
    return GVoxVoxel{{std::fabsf(qz + (qw - qy) / (6.0f * d + e)), d / (qx + e), qx}, c.id};
}

GVoxVoxel hsv2rgb(GVoxVoxel c) {
    // return c;
    float kx = 1.0f;
    float ky = 2.0f / 3.0f;
    float kz = 1.0f / 3.0f;
    float kw = 3.0f;
    float px = std::fabsf(std::fmodf(c.color.x + kx, 1.0f) * 6.0f - kw);
    float py = std::fabsf(std::fmodf(c.color.x + ky, 1.0f) * 6.0f - kw);
    float pz = std::fabsf(std::fmodf(c.color.x + kz, 1.0f) * 6.0f - kw);
    return GVoxVoxel{
        {
            c.color.z * std::lerp(kx, std::clamp(px - kx, 0.0f, 1.0f), c.color.y),
            c.color.z * std::lerp(kx, std::clamp(py - kx, 0.0f, 1.0f), c.color.y),
            c.color.z * std::lerp(kx, std::clamp(pz - kx, 0.0f, 1.0f), c.color.y),
        },
        c.id,
    };
}

auto MagicavoxelContext::create_payload(GVoxScene scene) -> GVoxPayload {
    GVoxPayload result = {};
    struct ChunkHeader {
        MagicavoxelChunkID id;
        int self_size;
        int children_size;
    };
    int const version = 150;
    auto node_nx = (scene.nodes[0].size_x + 255) / 256;
    auto node_ny = (scene.nodes[0].size_y + 255) / 256;
    auto node_nz = (scene.nodes[0].size_z + 255) / 256;
    auto node_n = node_nx * node_ny * node_nz;
    result.size += sizeof(CHUNK_ID_VOX_);
    result.size += sizeof(version);
    // Main chunk
    result.size += sizeof(ChunkHeader);
    auto size_before_main_children = result.size;
    // SIZE chunks
    result.size += sizeof(ChunkHeader) * node_n;
    result.size += sizeof(int) * 3 * node_n;
    // XYZI chunks
    result.size += sizeof(ChunkHeader) * node_n;
    result.size += sizeof(int) * node_n;
    std::set<uint32_t> out_palette;
    auto voxel_to_u32 = [](GVoxVoxel const &v) -> uint32_t {
        uint32_t u32_voxel = 0;
        uint32_t const r = static_cast<uint32_t>(std::clamp(v.color.x * 255.0f, 0.0f, 255.0f));
        uint32_t const g = static_cast<uint32_t>(std::clamp(v.color.y * 255.0f, 0.0f, 255.0f));
        uint32_t const b = static_cast<uint32_t>(std::clamp(v.color.z * 255.0f, 0.0f, 255.0f));
        uint32_t const a = 0xff;
        u32_voxel = u32_voxel | (r << 0x00);
        u32_voxel = u32_voxel | (g << 0x08);
        u32_voxel = u32_voxel | (b << 0x10);
        u32_voxel = u32_voxel | (a << 0x18);
        return u32_voxel;
    };
    auto u32_to_voxel = [](uint32_t u32_voxel) -> GVoxVoxel {
        float r = static_cast<float>((u32_voxel >> 0x00) & 0xff) / 255.0f;
        float g = static_cast<float>((u32_voxel >> 0x08) & 0xff) / 255.0f;
        float b = static_cast<float>((u32_voxel >> 0x10) & 0xff) / 255.0f;
        return GVoxVoxel{{r, g, b}, 1u};
    };
    int voxel_n = 0;
    bool palette_overflown = false;
    GVoxVoxel min_voxel = GVoxVoxel{{std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()}, 1u};
    GVoxVoxel max_voxel = GVoxVoxel{{std::numeric_limits<float>::min(), std::numeric_limits<float>::min(), std::numeric_limits<float>::min()}, 1u};
    for (size_t zi = 0; zi < scene.nodes[0].size_z; ++zi) {
        for (size_t yi = 0; yi < scene.nodes[0].size_y; ++yi) {
            for (size_t xi = 0; xi < scene.nodes[0].size_x; ++xi) {
                size_t const i = xi + yi * scene.nodes[0].size_x + zi * scene.nodes[0].size_x * scene.nodes[0].size_y;
                GVoxVoxel const &v = scene.nodes[0].voxels[i];
                if (v.id != 0) {
                    auto hsv_v = rgb2hsv(v);
                    if (v.color.x != 0.0f)
                        min_voxel.color.x = std::min(min_voxel.color.x, hsv_v.color.x);
                    if (v.color.y != 0.0f)
                        min_voxel.color.y = std::min(min_voxel.color.y, hsv_v.color.y);
                    if (v.color.z != 0.0f)
                        min_voxel.color.z = std::min(min_voxel.color.z, hsv_v.color.z);
                    if (v.color.x != 1.0f)
                        max_voxel.color.x = std::max(max_voxel.color.x, hsv_v.color.x);
                    if (v.color.y != 1.0f)
                        max_voxel.color.y = std::max(max_voxel.color.y, hsv_v.color.y);
                    if (v.color.z != 1.0f)
                        max_voxel.color.z = std::max(max_voxel.color.z, hsv_v.color.z);
                    if (out_palette.size() < 255) {
                        auto u32_voxel = voxel_to_u32(v);
                        auto palette_iter = out_palette.find(u32_voxel);
                        if (palette_iter == out_palette.end()) {
                            out_palette.insert(u32_voxel);
                            if (out_palette.size() == 255) {
                                palette_overflown = true;
                            }
                        }
                    }
                    // if (xi < 256 && yi < 256 && zi < 256)
                    ++voxel_n;
                }
            }
        }
    }
    min_voxel.color.x = std::max(min_voxel.color.x, 1.0f / 255.0f);
    min_voxel.color.y = std::max(min_voxel.color.y, 25.0f / 255.0f);
    min_voxel.color.z = std::max(min_voxel.color.z, 25.0f / 255.0f);
    max_voxel.color.x = std::min(max_voxel.color.x, 254.0f / 255.0f);
    max_voxel.color.y = std::min(max_voxel.color.y, 254.0f / 255.0f);
    max_voxel.color.z = std::min(max_voxel.color.z, 254.0f / 255.0f);
    // std::cout << "min: " << min_voxel.color.x << ", " << min_voxel.color.y << ", " << min_voxel.color.z << std::endl;
    // std::cout << "max: " << max_voxel.color.x << ", " << max_voxel.color.y << ", " << max_voxel.color.z << std::endl;
    Palette o_palette{};
    if (palette_overflown) {
        for (uint32_t i = 0; i < 8; ++i) {
            o_palette[i + 0] = voxel_to_u32(hsv2rgb(GVoxVoxel{{0.5f, 0.1f, static_cast<float>(i) / 7.0f}, 1u}));
            o_palette[i + 7] = voxel_to_u32(hsv2rgb(GVoxVoxel{{0.9f, 0.1f, static_cast<float>(i) / 7.0f}, 1u}));
        }
        float del_r = max_voxel.color.x - min_voxel.color.x;
        float del_g = max_voxel.color.y - min_voxel.color.y;
        float del_b = max_voxel.color.z - min_voxel.color.z;
        float off_r = min_voxel.color.x;
        float off_g = min_voxel.color.y;
        float off_b = min_voxel.color.z;
        uint32_t rn = 8;
        uint32_t gn = 5;
        uint32_t bn = 6;
        for (uint32_t bi = 0; bi < bn; ++bi) {
            for (uint32_t gi = 0; gi < gn; ++gi) {
                for (uint32_t ri = 0; ri < rn; ++ri) {
                    uint32_t i = ri + gi * rn + bi * rn * gn + 15;
                    float r = del_r * static_cast<float>(ri) / static_cast<float>(rn) + off_r;
                    float g = del_g * static_cast<float>(gi) / static_cast<float>(gn - 1) + off_g;
                    float b = del_b * static_cast<float>(bi) / static_cast<float>(bn - 1) + off_b;
                    o_palette[i] = voxel_to_u32(hsv2rgb(GVoxVoxel{{r, g, b}, 1u}));
                }
            }
        }
    } else {
        size_t i = 0;
        for (auto u32_voxel : out_palette) {
            o_palette[i] = u32_voxel;
            ++i;
        }
    }
    result.size += sizeof(MagicavoxelVoxel) * static_cast<size_t>(voxel_n);
    // RGBA chunk
    result.size += sizeof(ChunkHeader);
    result.size += sizeof(Palette);
    result.data = new uint8_t[result.size];
    memset(result.data, 0, result.size);
    uint8_t *buffer_ptr = result.data;
    write_data(&buffer_ptr, CHUNK_ID_VOX_);
    write_data(&buffer_ptr, version);
    write_data(&buffer_ptr, ChunkHeader{.id = CHUNK_MAIN, .self_size = 0, .children_size = static_cast<int>(result.size - size_before_main_children)});
    auto min_dist = std::numeric_limits<std::ptrdiff_t>::max();
    auto max_dist = std::numeric_limits<std::ptrdiff_t>::min();
    for (size_t node_zi = 0; node_zi < node_nz; ++node_zi) {
        for (size_t node_yi = 0; node_yi < node_ny; ++node_yi) {
            for (size_t node_xi = 0; node_xi < node_nx; ++node_xi) {
                auto node_offset_x = node_xi * 256;
                auto node_offset_y = node_yi * 256;
                auto node_offset_z = node_zi * 256;
                auto out_size_x = std::min<size_t>(scene.nodes[0].size_x - node_offset_x, 256);
                auto out_size_y = std::min<size_t>(scene.nodes[0].size_y - node_offset_y, 256);
                auto out_size_z = std::min<size_t>(scene.nodes[0].size_z - node_offset_z, 256);
                write_data(&buffer_ptr, ChunkHeader{.id = CHUNK_SIZE, .self_size = sizeof(int) * 3, .children_size = 0});
                write_data(&buffer_ptr, static_cast<int>(out_size_x));
                write_data(&buffer_ptr, static_cast<int>(out_size_y));
                write_data(&buffer_ptr, static_cast<int>(out_size_z));
                write_data(&buffer_ptr, ChunkHeader{.id = CHUNK_XYZI, .self_size = static_cast<int>(sizeof(int) + sizeof(MagicavoxelVoxel) * static_cast<size_t>(voxel_n)), .children_size = 0});
                write_data(&buffer_ptr, static_cast<int>(out_size_x * out_size_y * out_size_z));
                for (size_t zi = 0; zi < out_size_z; ++zi) {
                    for (size_t yi = 0; yi < out_size_y; ++yi) {
                        for (size_t xi = 0; xi < out_size_x; ++xi) {
                            size_t const voxel_i = (xi + node_offset_x) + (yi + node_offset_y) * scene.nodes[0].size_x + (zi + node_offset_z) * scene.nodes[0].size_x * scene.nodes[0].size_y;
                            auto const &i_voxel = scene.nodes[0].voxels[voxel_i];
                            if (i_voxel.id != 0) {
                                MagicavoxelVoxel o_voxel{};
                                o_voxel.x = static_cast<uint8_t>(xi);
                                o_voxel.y = static_cast<uint8_t>(yi);
                                o_voxel.z = static_cast<uint8_t>(zi);
                                if (palette_overflown) {
                                    auto hsv_v = /* rgb2hsv */ (i_voxel);
                                    auto min_iter = std::min_element(o_palette.begin(), o_palette.end(), [&](uint32_t a_, uint32_t b_) {
                                        auto const a_v = /* rgb2hsv */ (u32_to_voxel(a_));
                                        auto const b_v = /* rgb2hsv */ (u32_to_voxel(b_));
                                        auto const a = std::array<float, 3>{
                                            a_v.color.x - hsv_v.color.x,
                                            a_v.color.y - hsv_v.color.y,
                                            a_v.color.z - hsv_v.color.z,
                                        };
                                        auto const b = std::array<float, 3>{
                                            b_v.color.x - hsv_v.color.x,
                                            b_v.color.y - hsv_v.color.y,
                                            b_v.color.z - hsv_v.color.z,
                                        };
                                        float const a_dist = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
                                        float const b_dist = b[0] * b[0] + b[1] * b[1] + b[2] * b[2];
                                        return a_dist < b_dist;
                                    });
                                    auto dist = std::distance(o_palette.begin(), min_iter);
                                    o_voxel.color_index = static_cast<uint8_t>(dist + 1);
                                } else {
                                    auto u32_voxel = voxel_to_u32(i_voxel);
                                    auto palette_iter = std::find(o_palette.begin(), o_palette.end(), u32_voxel);
                                    o_voxel.color_index = static_cast<uint8_t>(std::distance(o_palette.begin(), palette_iter) + 1);
                                }
                                write_data(&buffer_ptr, o_voxel);
                            }
                        }
                    }
                }
            }
        }
    }
    write_data(&buffer_ptr, ChunkHeader{.id = CHUNK_RGBA, .self_size = static_cast<int>(sizeof(Palette)), .children_size = 0});
    write_data(&buffer_ptr, o_palette);
    return result;
}

void MagicavoxelContext::destroy_payload(GVoxPayload payload) {
    delete[] payload.data;
}

constexpr auto transform_mul(const ogt_vox_transform &a, const vec3 &b) -> vec3 {
    vec3 r;
    r.x = (a.m00 * b.x) + (a.m10 * b.y) + (a.m20 * b.z) + (a.m30 * 1.0f);
    r.y = (a.m01 * b.x) + (a.m11 * b.y) + (a.m21 * b.z) + (a.m31 * 1.0f);
    r.z = (a.m02 * b.x) + (a.m12 * b.y) + (a.m22 * b.z) + (a.m32 * 1.0f);
    return r;
}

auto MagicavoxelContext::parse_payload(GVoxPayload payload) -> GVoxScene {
    GVoxScene result = {};
    ogt_vox_scene const *scene = ogt_vox_read_scene(payload.data, static_cast<uint32_t>(payload.size));
    result.node_n = 1;
    result.nodes = new GVoxSceneNode[result.node_n];
    auto inst_min = [scene](ogt_vox_instance const &inst) {
        auto const &modl = *scene->models[inst.model_index];
        vec3 p0 = transform_mul(inst.transform, vec3{static_cast<float>(modl.offset_x + 0), static_cast<float>(modl.offset_y + 0), static_cast<float>(modl.offset_z + 0)});
        vec3 p1 = transform_mul(inst.transform, vec3{static_cast<float>(modl.offset_x + modl.size_x), static_cast<float>(modl.offset_y + modl.size_y), static_cast<float>(modl.offset_z + modl.size_z)});
        return vec3{std::min(p0.x, p1.x), std::min(p0.y, p1.y), std::min(p0.z, p1.z)};
    };
    auto inst_max = [scene](ogt_vox_instance const &inst) {
        auto const &modl = *scene->models[inst.model_index];
        vec3 p0 = transform_mul(inst.transform, vec3{static_cast<float>(modl.offset_x + 0), static_cast<float>(modl.offset_y + 0), static_cast<float>(modl.offset_z + 0)});
        vec3 p1 = transform_mul(inst.transform, vec3{static_cast<float>(modl.offset_x + modl.size_x), static_cast<float>(modl.offset_y + modl.size_y), static_cast<float>(modl.offset_z + modl.size_z)});
        return vec3{std::max(p0.x, p1.x), std::max(p0.y, p1.y), std::max(p0.z, p1.z)};
    };
#define INSTANCES_MIN(axis)                                                                  \
    static_cast<size_t>(                                                                     \
        inst_min(                                                                            \
            *std::min_element(                                                               \
                scene->instances,                                                            \
                scene->instances + scene->num_instances,                                     \
                [inst_min](ogt_vox_instance const &inst_a, ogt_vox_instance const &inst_b) { \
                    return inst_min(inst_a).axis < inst_min(inst_b).axis;                    \
                }))                                                                          \
            .axis)
#define INSTANCES_MAX(axis)                                                                  \
    static_cast<size_t>(                                                                     \
        inst_max(                                                                            \
            *std::max_element(                                                               \
                scene->instances,                                                            \
                scene->instances + scene->num_instances,                                     \
                [inst_max](ogt_vox_instance const &inst_a, ogt_vox_instance const &inst_b) { \
                    return inst_max(inst_a).axis < inst_max(inst_b).axis;                    \
                }))                                                                          \
            .axis)
    auto min_x = INSTANCES_MIN(x), min_y = INSTANCES_MIN(y), min_z = INSTANCES_MIN(z);
    auto max_x = INSTANCES_MAX(x), max_y = INSTANCES_MAX(y), max_z = INSTANCES_MAX(z);
    auto size_x = max_x - min_x, size_y = max_y - min_y, size_z = max_z - min_z;
    result.nodes[0].size_x = size_x;
    result.nodes[0].size_y = size_y;
    result.nodes[0].size_z = size_z;
    auto size = result.nodes[0].size_x * result.nodes[0].size_y * result.nodes[0].size_z;
    result.nodes[0].voxels = (GVoxVoxel *)std::malloc(sizeof(GVoxVoxel) * size);
    std::memset(result.nodes[0].voxels, 0, sizeof(GVoxVoxel) * size);
    for (uint32_t instance_i = 0; instance_i < scene->num_instances; ++instance_i) {
        auto const &inst = scene->instances[instance_i];
        auto const &modl = *scene->models[inst.model_index];
        for (size_t zi = 0; zi < modl.size_z; ++zi) {
            for (size_t yi = 0; yi < modl.size_y; ++yi) {
                for (size_t xi = 0; xi < modl.size_x; ++xi) {
                    size_t const modl_voxel_i = xi + yi * modl.size_x + zi * modl.size_x * modl.size_y;
                    vec3 p = transform_mul(inst.transform, vec3{static_cast<float>(modl.offset_x + xi), static_cast<float>(modl.offset_y + yi), static_cast<float>(modl.offset_z + zi)});
                    size_t const final_voxel_i = (static_cast<size_t>(p.x) - min_x) + (static_cast<size_t>(p.y) - min_y) * result.nodes[0].size_x + (static_cast<size_t>(p.z) - min_z) * result.nodes[0].size_x * result.nodes[0].size_y;
                    auto const &voxel = modl.voxel_data[modl_voxel_i];
                    if (voxel != 0) {
                        auto const &color = scene->palette.color[voxel];
                        result.nodes[0].voxels[final_voxel_i] = GVoxVoxel{
                            .color = {
                                static_cast<float>(color.r) * 1.0f / 255.0f,
                                static_cast<float>(color.g) * 1.0f / 255.0f,
                                static_cast<float>(color.b) * 1.0f / 255.0f,
                            },
                            .id = 1,
                        };
                    }
                }
            }
        }
    }
    ogt_vox_destroy_scene(scene);
    return result;
}

extern "C" EXPORT auto gvox_format_magicavoxel_create_context() -> void * {
    auto *result = new MagicavoxelContext{};
    return result;
}

extern "C" EXPORT void gvox_format_magicavoxel_destroy_context(void *context_ptr) {
    auto *self = reinterpret_cast<MagicavoxelContext *>(context_ptr);
    delete self;
}

extern "C" EXPORT auto gvox_format_magicavoxel_create_payload(void *context_ptr, GVoxScene const *scene) -> GVoxPayload {
    auto *self = reinterpret_cast<MagicavoxelContext *>(context_ptr);
    return self->create_payload(*scene);
}

extern "C" EXPORT void gvox_format_magicavoxel_destroy_payload(void *context_ptr, GVoxPayload const *payload) {
    auto *self = reinterpret_cast<MagicavoxelContext *>(context_ptr);
    self->destroy_payload(*payload);
}

extern "C" EXPORT auto gvox_format_magicavoxel_parse_payload(void *context_ptr, GVoxPayload const *payload) -> GVoxScene {
    auto *self = reinterpret_cast<MagicavoxelContext *>(context_ptr);
    return self->parse_payload(*payload);
}
