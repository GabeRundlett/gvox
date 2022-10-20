#include <gvox/gvox.h>

#include <stdio.h>
#include <memory.h>
#include <assert.h>

#include <vector>

#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport) __stdcall
#endif

typedef struct {
    uint8_t b0, b1, b2, b3;
} ChunkID;

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
    printf("     - got string at %p with size %d\n", (void *)*buffer_ptr, string_size);
    *buffer_ptr += string_size;
}
static void parse_dict(uint8_t **buffer_ptr) {
    int entry_n = *(int *)(*buffer_ptr);
    *buffer_ptr += sizeof(entry_n);
    printf("   - parsing dictionary at %p with %d entries\n", (void *)*buffer_ptr, entry_n);
    for (int entry_i = 0; entry_i < entry_n; ++entry_i) {
        parse_string(buffer_ptr);
    }
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

unsigned int default_palette[256] = {
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
    printf("Fixing up... %llo, %d\n", nodes.size(), (int)got_colors);
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
    printf(" - dimensions %d, %d, %d\n", size_x, size_y, size_z);
}
void Parser::parse_chunk_XYZI() {
    int voxel_count = parse_int32(&buffer_ptr);
    printf(" - voxel count %d\n", voxel_count);
    size_t size_x = nodes.back().size_x;
    size_t size_y = nodes.back().size_y;
    size_t size_z = nodes.back().size_z;
    size_t size = size_x * size_y * size_z;
    nodes.back().voxels = new GVoxVoxel[size];
    struct MagicavoxelVoxel {
        uint8_t x, y, z, color_index;
    };
    for (int i = 0; i < voxel_count; ++i) {
        MagicavoxelVoxel voxel = *reinterpret_cast<MagicavoxelVoxel *>(buffer_ptr);
        size_t index = voxel.x + voxel.y * size_x + voxel.z * size_x * size_y;
        nodes.back().voxels[index] = GVoxVoxel{.id = static_cast<uint32_t>(voxel.color_index)};
    }
    // seek past voxel data
    buffer_ptr += sizeof(uint32_t) * (size_t)voxel_count;
}
void Parser::parse_chunk_RGBA() {
    buffer_ptr += sizeof(uint32_t) * 256;
    got_colors = true;
    printf(" - RGBA palette\n");
}
void Parser::parse_chunk_nTRN() {

    int node_id = parse_int32(&buffer_ptr);
    printf(" - nTRN node_id %d\n", node_id);
    parse_dict(&buffer_ptr);
    int child_node_id = parse_int32(&buffer_ptr);
    int reserved_id = parse_int32(&buffer_ptr);
    int layer_id = parse_int32(&buffer_ptr);
    int frame_n = parse_int32(&buffer_ptr);
    printf(" - child, reserved, layer, frame_n: %d, %d, %d, %d\n", child_node_id, reserved_id, layer_id, frame_n);
    for (int frame_i = 0; frame_i < frame_n; ++frame_i) {
        parse_dict(&buffer_ptr);
    }
}
void Parser::parse_chunk_nGRP() {
    int node_id = parse_int32(&buffer_ptr);
    printf(" - nGRP node_id %d\n", node_id);
    parse_dict(&buffer_ptr);
    int child_node_n = parse_int32(&buffer_ptr);
    for (int child_i = 0; child_i < child_node_n; ++child_i) {
        int child_id = parse_int32(&buffer_ptr);
        printf("   - child_id %d\n", child_id);
    }
}
void Parser::parse_chunk_nSHP() {
    int node_id = parse_int32(&buffer_ptr);
    printf(" - nSHP node_id %d\n", node_id);
    parse_dict(&buffer_ptr);
    int child_node_n = parse_int32(&buffer_ptr);
    for (int child_i = 0; child_i < child_node_n; ++child_i) {
        int model_id = parse_int32(&buffer_ptr);
        printf("   - model_id %d\n", model_id);
        parse_dict(&buffer_ptr);
    }
}
void Parser::parse_chunk_IMAP() {
    (void)buffer_ptr;
    assert(0 && "BRUH");
}
void Parser::parse_chunk_LAYR() {
    int node_id = parse_int32(&buffer_ptr);
    printf(" - LAYR node_id %d\n", node_id);
    parse_dict(&buffer_ptr);
    int reserved_id = parse_int32(&buffer_ptr);
    printf("   - reserved_id %d\n", reserved_id);
}
void Parser::parse_chunk_MATL() {
    int material_id = parse_int32(&buffer_ptr);
    printf(" - MATL %d\n", material_id);
    parse_dict(&buffer_ptr);
}
void Parser::parse_chunk_MATT() {
    (void)buffer_ptr;
    assert(0 && "MATT Deprecated");
}
void Parser::parse_chunk_rOBJ() {
    printf(" - rOBJ\n");
    parse_dict(&buffer_ptr);
}
void Parser::parse_chunk_rCAM() {
    int camera_id = parse_int32(&buffer_ptr);
    printf(" - rCAM %d\n", camera_id);
    parse_dict(&buffer_ptr);
}

int Parser::parse_chunk() {
    if (!buffer_ptr)
        return 1;
    ChunkID chunk_id = *(ChunkID *)(buffer_ptr);
    buffer_ptr += sizeof(chunk_id);
    int self_size = parse_int32(&buffer_ptr);
    int children_size = parse_int32(&buffer_ptr);
    auto next_buffer_ptr = buffer_ptr + self_size;
    printf("Parsing chunk %c%c%c%c\n", chunk_id.b0, chunk_id.b1, chunk_id.b2, chunk_id.b3);
    printf(" - self size     %d\n", self_size);
    printf(" - children size %d\n", children_size);
    if (memcmp(&chunk_id, &CHUNK_ID_MAIN, sizeof(ChunkID)) == 0) {
        buffer_ptr += self_size;
        printf("pointers were %p and %p\n", (void *)buffer_ptr, (void *)buffer_sentinel);
        buffer_sentinel = (buffer_ptr) + children_size;
        printf("pointers now  %p and %p\n", (void *)buffer_ptr, (void *)buffer_sentinel);
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
        printf("pointers were %p and %p\n", (void *)buffer_ptr, (void *)buffer_sentinel);
        fixup_last_node();
        return 1;
    }
    buffer_ptr = next_buffer_ptr;
    return 0;
}

void Parser::parse(GVoxPayload const &payload) {
    buffer_ptr = payload.data;
    buffer_sentinel = payload.data + payload.size;
    uint8_t start_bytes[4] = {'V', 'O', 'X', ' '};
    if (memcmp(&start_bytes, buffer_ptr, sizeof(start_bytes)) != 0)
        return;
    buffer_ptr += sizeof(start_bytes);
    int version = *(int *)(buffer_ptr);
    buffer_ptr += sizeof(version);
    printf("Found MagicaVoxel file has version %d\n", version);
    while (buffer_ptr < buffer_sentinel) {
        if (parse_chunk() != 0)
            break;
    }
    fixup_last_node();
}

extern "C" {
GVoxPayload EXPORT gvox_create_payload(GVoxScene scene) {
    // TODO: Implement this
    GVoxPayload result = {};
    printf("creating magicavoxel payload from the %zu nodes at %p\n", scene.node_n, (void *)scene.nodes);
    return result;
}

void EXPORT gvox_destroy_payload(GVoxPayload payload) {
    // TODO: Implement this
    printf("destroying magicavoxel payload at %p with size %zu\n", (void *)payload.data, payload.size);
}
GVoxScene EXPORT gvox_parse_payload(GVoxPayload payload) {
    // TODO: Implement this
    GVoxScene result = {};
    printf("parsing magicavoxel payload at %p with size %zu\n", (void *)payload.data, payload.size);
    Parser parser;
    parser.parse(payload);
    result.node_n = parser.nodes.size();
    result.nodes = new GVoxSceneNode[result.node_n];
    for (size_t i = 0; i < result.node_n; ++i) {
        result.nodes[i] = parser.nodes[i];
    }
    return result;
}
}
