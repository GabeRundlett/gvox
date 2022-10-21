#include <gvox/gvox.h>

#include <stdio.h>
#include <memory.h>
#include <assert.h>

#include <array>
#include <vector>
#include <algorithm>

#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport) __stdcall
#endif

struct MagicavoxelVoxel {
    uint8_t x, y, z, color_index;
};
struct ChunkID {
    uint8_t b0, b1, b2, b3;
};

static const ChunkID CHUNK_ID_VOX_ = {'V', 'O', 'X', ' '};
static const ChunkID CHUNK_ID_MAIN = {'M', 'A', 'I', 'N'};
static const ChunkID CHUNK_ID_SIZE = {'S', 'I', 'Z', 'E'};
static const ChunkID CHUNK_ID_XYZI = {'X', 'Y', 'Z', 'I'};
static const ChunkID CHUNK_ID_RGBA = {'R', 'G', 'B', 'A'};
static const ChunkID CHUNK_ID_nTRN = {'n', 'T', 'R', 'N'};
static const ChunkID CHUNK_ID_nGRP = {'n', 'G', 'R', 'P'};
static const ChunkID CHUNK_ID_nSHP = {'n', 'S', 'H', 'P'};
static const ChunkID CHUNK_ID_IMAP = {'I', 'M', 'A', 'P'};
static const ChunkID CHUNK_ID_LAYR = {'L', 'A', 'Y', 'R'};
static const ChunkID CHUNK_ID_MATL = {'M', 'A', 'T', 'L'};
static const ChunkID CHUNK_ID_MATT = {'M', 'A', 'T', 'T'};
static const ChunkID CHUNK_ID_rOBJ = {'r', 'O', 'B', 'J'};
static const ChunkID CHUNK_ID_rCAM = {'r', 'C', 'A', 'M'};

static int parse_int32(uint8_t **buffer_ptr) {
    int result = *(int *)(*buffer_ptr);
    *buffer_ptr += sizeof(result);
    return result;
}
static void parse_string(uint8_t **buffer_ptr) {
    int string_size = *(int *)(*buffer_ptr);
    *buffer_ptr += sizeof(string_size);
    *buffer_ptr += string_size;
}
static void parse_dict(uint8_t **buffer_ptr) {
    int entry_n = *(int *)(*buffer_ptr);
    *buffer_ptr += sizeof(entry_n);
    for (int entry_i = 0; entry_i < entry_n; ++entry_i) {
        parse_string(buffer_ptr);
    }
}

template <typename T>
static void write_data(uint8_t **buffer_ptr, T const &data) {
    *reinterpret_cast<T *>(*buffer_ptr) = data;
    *buffer_ptr += sizeof(T);
}

struct Parser {
    uint8_t *buffer_ptr;
    uint8_t *buffer_sentinel;
    std::vector<GVoxSceneNode> nodes;
    bool got_colors = false;
    void parse_chunk_SIZE();
    void parse_chunk_XYZI();
    void parse_chunk_RGBA();
    void parse_chunk_nTRN();
    void parse_chunk_nGRP();
    void parse_chunk_nSHP();
    void parse_chunk_IMAP();
    void parse_chunk_LAYR();
    void parse_chunk_MATL();
    void parse_chunk_MATT();
    void parse_chunk_rOBJ();
    void parse_chunk_rCAM();
    int parse_chunk();
    void fixup_last_node();
    void parse(GVoxPayload const &payload);
};
using Palette = std::array<uint32_t, 256>;
Palette default_palette = {
    0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
    0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
    0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
    0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
    0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
    0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
    0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
    0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
    0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
    0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
    0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
    0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
    0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
    0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
    0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
    0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111};
void Parser::fixup_last_node() {
    if (nodes.size() == 0 || got_colors)
        return;
    size_t size_x = nodes.back().size_x;
    size_t size_y = nodes.back().size_y;
    size_t size_z = nodes.back().size_z;
    size_t size = size_x * size_y * size_z;
    for (size_t i = 0; i < size; ++i) {
        auto &voxel = nodes.back().voxels[i];
        uint32_t palette_value = default_palette[voxel.id];
        voxel.color.x = static_cast<float>((palette_value >> 0x00) & 0xff) * 1.0f / 255.0f;
        voxel.color.y = static_cast<float>((palette_value >> 0x08) & 0xff) * 1.0f / 255.0f;
        voxel.color.z = static_cast<float>((palette_value >> 0x10) & 0xff) * 1.0f / 255.0f;
    }
}
void Parser::parse_chunk_SIZE() {
    // check if there was a previous chunk, and if it never had a palette applied
    fixup_last_node();
    // create the new chunk
    int size_x = parse_int32(&buffer_ptr);
    int size_y = parse_int32(&buffer_ptr);
    int size_z = parse_int32(&buffer_ptr);
    nodes.push_back({});
    got_colors = false;
    nodes.back().size_x = (size_t)size_x;
    nodes.back().size_y = (size_t)size_y;
    nodes.back().size_z = (size_t)size_z;
}
void Parser::parse_chunk_XYZI() {
    int voxel_count = parse_int32(&buffer_ptr);
    size_t size_x = nodes.back().size_x;
    size_t size_y = nodes.back().size_y;
    size_t size_z = nodes.back().size_z;
    size_t size = size_x * size_y * size_z;
    nodes.back().voxels = (GVoxVoxel *)malloc(sizeof(GVoxVoxel) * size);
    memset(nodes.back().voxels, 0, size * sizeof(GVoxVoxel));
    for (int i = 0; i < voxel_count; ++i) {
        MagicavoxelVoxel voxel = *(reinterpret_cast<MagicavoxelVoxel *>(buffer_ptr) + i);
        size_t index = voxel.x + voxel.y * size_x + voxel.z * size_x * size_y;
        nodes.back().voxels[index] = GVoxVoxel{.id = static_cast<uint32_t>(voxel.color_index)};
    }
    // seek past voxel data
    buffer_ptr += sizeof(uint32_t) * (size_t)voxel_count;
}
void Parser::parse_chunk_RGBA() {
    size_t size_x = nodes.back().size_x;
    size_t size_y = nodes.back().size_y;
    size_t size_z = nodes.back().size_z;
    size_t size = size_x * size_y * size_z;
    Palette &palette = *reinterpret_cast<Palette *>(buffer_ptr);
    for (size_t i = 0; i < size; ++i) {
        auto &voxel = nodes.back().voxels[i];
        uint32_t palette_value = palette[voxel.id];
        voxel.color.x = static_cast<float>((palette_value >> 0x00) & 0xff) * 1.0f / 255.0f;
        voxel.color.y = static_cast<float>((palette_value >> 0x08) & 0xff) * 1.0f / 255.0f;
        voxel.color.z = static_cast<float>((palette_value >> 0x10) & 0xff) * 1.0f / 255.0f;
    }
    buffer_ptr += sizeof(Palette);
    got_colors = true;
}
void Parser::parse_chunk_nTRN() {
    // int node_id = parse_int32(&buffer_ptr);
    // parse_dict(&buffer_ptr);
    // int child_node_id = parse_int32(&buffer_ptr);
    // int reserved_id = parse_int32(&buffer_ptr);
    // int layer_id = parse_int32(&buffer_ptr);
    // int frame_n = parse_int32(&buffer_ptr);
    // for (int frame_i = 0; frame_i < frame_n; ++frame_i) {
    //     parse_dict(&buffer_ptr);
    // }
}
void Parser::parse_chunk_nGRP() {
    // int node_id = parse_int32(&buffer_ptr);
    // parse_dict(&buffer_ptr);
    // int child_node_n = parse_int32(&buffer_ptr);
    // for (int child_i = 0; child_i < child_node_n; ++child_i) {
    //     int child_id = parse_int32(&buffer_ptr);
    // }
}
void Parser::parse_chunk_nSHP() {
    // int node_id = parse_int32(&buffer_ptr);
    // parse_dict(&buffer_ptr);
    // int child_node_n = parse_int32(&buffer_ptr);
    // for (int child_i = 0; child_i < child_node_n; ++child_i) {
    //     int model_id = parse_int32(&buffer_ptr);
    //     parse_dict(&buffer_ptr);
    // }
}
void Parser::parse_chunk_IMAP() {
    // assert(0 && "BRUH");
}
void Parser::parse_chunk_LAYR() {
    // int node_id = parse_int32(&buffer_ptr);
    // parse_dict(&buffer_ptr);
    // int reserved_id = parse_int32(&buffer_ptr);
}
void Parser::parse_chunk_MATL() {
    // int material_id = parse_int32(&buffer_ptr);
    // parse_dict(&buffer_ptr);
}
void Parser::parse_chunk_MATT() {
    // assert(0 && "MATT Deprecated");
}
void Parser::parse_chunk_rOBJ() {
    parse_dict(&buffer_ptr);
}
void Parser::parse_chunk_rCAM() {
    // int camera_id = parse_int32(&buffer_ptr);
    // parse_dict(&buffer_ptr);
}
int Parser::parse_chunk() {
    if (!buffer_ptr)
        return 1;
    ChunkID chunk_id = *(ChunkID *)(buffer_ptr);
    buffer_ptr += sizeof(chunk_id);
    int self_size = parse_int32(&buffer_ptr);
    int children_size = parse_int32(&buffer_ptr);
    auto next_buffer_ptr = buffer_ptr + self_size;
    if (memcmp(&chunk_id, &CHUNK_ID_MAIN, sizeof(ChunkID)) == 0) {
        buffer_ptr += self_size;
        buffer_sentinel = (buffer_ptr) + children_size;
    } else if (memcmp(&chunk_id, &CHUNK_ID_SIZE, sizeof(ChunkID)) == 0) {
        parse_chunk_SIZE();
    } else if (memcmp(&chunk_id, &CHUNK_ID_XYZI, sizeof(ChunkID)) == 0) {
        parse_chunk_XYZI();
    } else if (memcmp(&chunk_id, &CHUNK_ID_RGBA, sizeof(ChunkID)) == 0) {
        parse_chunk_RGBA();
    } else if (memcmp(&chunk_id, &CHUNK_ID_nTRN, sizeof(ChunkID)) == 0) {
        parse_chunk_nTRN();
    } else if (memcmp(&chunk_id, &CHUNK_ID_nGRP, sizeof(ChunkID)) == 0) {
        parse_chunk_nGRP();
    } else if (memcmp(&chunk_id, &CHUNK_ID_nSHP, sizeof(ChunkID)) == 0) {
        parse_chunk_nSHP();
    } else if (memcmp(&chunk_id, &CHUNK_ID_IMAP, sizeof(ChunkID)) == 0) {
        parse_chunk_IMAP();
    } else if (memcmp(&chunk_id, &CHUNK_ID_LAYR, sizeof(ChunkID)) == 0) {
        parse_chunk_LAYR();
    } else if (memcmp(&chunk_id, &CHUNK_ID_MATL, sizeof(ChunkID)) == 0) {
        parse_chunk_MATL();
    } else if (memcmp(&chunk_id, &CHUNK_ID_MATT, sizeof(ChunkID)) == 0) {
        parse_chunk_MATT();
    } else if (memcmp(&chunk_id, &CHUNK_ID_rOBJ, sizeof(ChunkID)) == 0) {
        parse_chunk_rOBJ();
    } else if (memcmp(&chunk_id, &CHUNK_ID_rCAM, sizeof(ChunkID)) == 0) {
        parse_chunk_rCAM();
    } else {
        fixup_last_node();
        return 1;
    }
    buffer_ptr = next_buffer_ptr;
    return 0;
}
void Parser::parse(GVoxPayload const &payload) {
    buffer_ptr = payload.data;
    buffer_sentinel = payload.data + payload.size;
    if (memcmp(&CHUNK_ID_VOX_, buffer_ptr, sizeof(CHUNK_ID_VOX_)) != 0)
        return;
    buffer_ptr += sizeof(CHUNK_ID_VOX_);
    int version = *(int *)(buffer_ptr);
    buffer_ptr += sizeof(version);
    while (buffer_ptr < buffer_sentinel) {
        if (parse_chunk() != 0)
            break;
    }
    fixup_last_node();
}

extern "C" {
GVoxPayload EXPORT gvox_create_payload(GVoxScene scene) {
    GVoxPayload result = {};
    struct ChunkHeader {
        ChunkID id;
        int self_size;
        int children_size;
    };
    int version = 150;
    result.size += sizeof(CHUNK_ID_VOX_);
    result.size += sizeof(version);
    // Main chunk
    result.size += sizeof(ChunkHeader);
    auto size_before_main_children = result.size;
    // SIZE chunk
    result.size += sizeof(ChunkHeader);
    result.size += sizeof(int) * 3;
    // XYZI chunk
    result.size += sizeof(ChunkHeader);
    int voxel_n = 0;
    for (size_t i = 0; i < scene.nodes[0].size_x * scene.nodes[0].size_y * scene.nodes[0].size_z; ++i) {
        if (scene.nodes[0].voxels[i].id != 0) {
            ++voxel_n;
        }
    }
    result.size += sizeof(voxel_n);
    result.size += sizeof(MagicavoxelVoxel) * voxel_n;
    // RGBA chunk
    result.size += sizeof(ChunkHeader);
    result.size += sizeof(Palette);
    result.data = new uint8_t[result.size];
    memset(result.data, 0, result.size);
    uint8_t *buffer_ptr = result.data;
    write_data(&buffer_ptr, CHUNK_ID_VOX_);
    write_data(&buffer_ptr, version);
    write_data(&buffer_ptr, ChunkHeader{.id = CHUNK_ID_MAIN, .self_size = 0, .children_size = static_cast<int>(result.size - size_before_main_children)});
    write_data(&buffer_ptr, ChunkHeader{.id = CHUNK_ID_SIZE, .self_size = sizeof(int) * 3, .children_size = 0});
    write_data(&buffer_ptr, static_cast<int>(scene.nodes[0].size_x));
    write_data(&buffer_ptr, static_cast<int>(scene.nodes[0].size_y));
    write_data(&buffer_ptr, static_cast<int>(scene.nodes[0].size_z));
    write_data(&buffer_ptr, ChunkHeader{.id = CHUNK_ID_XYZI, .self_size = static_cast<int>(sizeof(int) + sizeof(MagicavoxelVoxel) * voxel_n), .children_size = 0});
    write_data(&buffer_ptr, static_cast<int>(scene.nodes[0].size_x * scene.nodes[0].size_y * scene.nodes[0].size_z));
    for (size_t zi = 0; zi < scene.nodes[0].size_z; ++zi) {
        for (size_t yi = 0; yi < scene.nodes[0].size_y; ++yi) {
            for (size_t xi = 0; xi < scene.nodes[0].size_x; ++xi) {
                size_t i = xi + yi * scene.nodes[0].size_x + zi * scene.nodes[0].size_x * scene.nodes[0].size_y;
                auto const &i_voxel = scene.nodes[0].voxels[i];
                if (i_voxel.id != 0) {
                    MagicavoxelVoxel o_voxel;
                    o_voxel.x = static_cast<uint8_t>(xi);
                    o_voxel.y = static_cast<uint8_t>(yi);
                    o_voxel.z = static_cast<uint8_t>(zi);
                    auto min_iter = std::min_element(default_palette.begin(), default_palette.end(), [&](uint32_t a_, uint32_t b_) {
                        float a[3] = {
                            static_cast<float>((a_ >> 0x00) & 0xff) * 1.0f / 255.0f - i_voxel.color.x,
                            static_cast<float>((a_ >> 0x08) & 0xff) * 1.0f / 255.0f - i_voxel.color.y,
                            static_cast<float>((a_ >> 0x10) & 0xff) * 1.0f / 255.0f - i_voxel.color.z,
                        };
                        float b[3] = {
                            static_cast<float>((b_ >> 0x00) & 0xff) * 1.0f / 255.0f - i_voxel.color.x,
                            static_cast<float>((b_ >> 0x08) & 0xff) * 1.0f / 255.0f - i_voxel.color.y,
                            static_cast<float>((b_ >> 0x10) & 0xff) * 1.0f / 255.0f - i_voxel.color.z,
                        };
                        float a_dist = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
                        float b_dist = b[0] * b[0] + b[1] * b[1] + b[2] * b[2];
                        return a_dist < b_dist;
                    });
                    o_voxel.color_index = static_cast<uint8_t>(min_iter - default_palette.begin());
                    write_data(&buffer_ptr, o_voxel);
                }
            }
        }
    }
    write_data(&buffer_ptr, ChunkHeader{.id = CHUNK_ID_RGBA, .self_size = static_cast<int>(sizeof(Palette)), .children_size = 0});
    write_data(&buffer_ptr, default_palette);
    return result;
}

void EXPORT gvox_destroy_payload(GVoxPayload payload) {
    delete[] payload.data;
}

GVoxScene EXPORT gvox_parse_payload(GVoxPayload payload) {
    GVoxScene result = {};
    Parser parser;
    parser.parse(payload);
    result.node_n = parser.nodes.size();
    result.nodes = new GVoxSceneNode[result.node_n];
    for (size_t i = 0; i < result.node_n; ++i)
        result.nodes[i] = parser.nodes[i];
    return result;
}
}
