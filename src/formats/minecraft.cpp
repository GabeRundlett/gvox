#include <gvox/gvox.h>

#include <cstdlib>
#include <cstring>
#include <cassert>

#include <algorithm>
#include <array>
#include <variant>
#include <vector>
#include <string_view>
#include <utility>
#include <memory>
#include <bit>

// #include <iostream>
// #include <fstream>
// #include <iomanip>

#include <zlib.h>

#if GVOX_FORMAT_MINECRAFT_BUILT_STATIC
#define EXPORT
#else
#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport)
#endif
#endif

template <typename T>
static void write_data(uint8_t *&buffer_ptr, T const &data) {
    *reinterpret_cast<T *>(buffer_ptr) = data;
    buffer_ptr += sizeof(T);
}

template <typename T>
static auto read_data(uint8_t *&buffer_ptr) -> T {
    auto &result = *reinterpret_cast<T *>(buffer_ptr);
    buffer_ptr += sizeof(T);
    return result;
}

template <typename T>
static auto read_data_ref(uint8_t *&buffer_ptr) -> T const & {
    auto const &result = *reinterpret_cast<T const *>(buffer_ptr);
    buffer_ptr += sizeof(T);
    return result;
}

template <typename T>
constexpr auto as_le(T x) -> T {
    auto bytes = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(x);
    for (size_t i = 0; i < sizeof(T) / 2; ++i) {
        auto temp = bytes[i];
        bytes[i] = bytes[sizeof(T) - 1 - i];
        bytes[sizeof(T) - 1 - i] = temp;
    }
    return std::bit_cast<T>(bytes);
}

struct MinecraftContext {
    MinecraftContext();
    ~MinecraftContext() = default;

    static auto create_payload(GVoxScene scene) -> GVoxPayload;
    static void destroy_payload(GVoxPayload payload);
    static auto parse_payload(GVoxPayload payload) -> GVoxScene;
};

MinecraftContext::MinecraftContext() = default;

auto MinecraftContext::create_payload(GVoxScene /* scene */) -> GVoxPayload {
    GVoxPayload result = {};
    return result;
}

void MinecraftContext::destroy_payload(GVoxPayload /* payload */) {
    // delete[] payload.data;
}

using McrChunkTable = std::array<uint32_t, 32 * 32>;

struct McrHeader {
    McrChunkTable offsets;
    McrChunkTable timestamps;
};

enum class NbtTagId : uint8_t {
    End,
    Byte,
    Short,
    Int,
    Long,
    Float,
    Double,
    Byte_Array,
    String,
    List,
    Compound,
    Int_Array,
    Long_Array,
};

struct NbtTag {
    using Byte = std::uint8_t;
    using Short = std::int16_t;
    using Int = std::int32_t;
    using Long = std::int64_t;
    using Float = float;
    using Double = double;
    struct Byte_Array {
        std::int32_t size;
        Byte *data;
    };
    using String = std::string_view;
    struct List;
    struct Int_Array {
        std::int32_t size;
        Int *data;
    };
    struct Long_Array {
        std::int32_t size;
        Long *data;
    };

    using Payload = std::variant<
        std::monostate,
        Byte,
        Short,
        Int,
        Long,
        Float,
        Double,
        Byte_Array,
        String,
        std::shared_ptr<List>,
        Int_Array,
        Long_Array>;

    struct List {
        NbtTagId payloads_tag_id;
        std::int32_t payload_n;
        std::vector<Payload> payloads;
    };

    NbtTagId id;
    std::string_view name;
    Payload payload;

    void parse_nbt_payload(GVoxScene &scene, uint8_t *&buffer_ptr);
};

auto parse_nbt_tag(GVoxScene &scene, uint8_t *&buffer_ptr) -> NbtTag {
    NbtTag result;
    result.id = read_data<NbtTagId>(buffer_ptr);
    if (result.id != NbtTagId::End) {
        auto len = as_le(read_data<uint16_t>(buffer_ptr));
        result.name = std::string_view{
            reinterpret_cast<char *>(buffer_ptr),
            reinterpret_cast<char *>(buffer_ptr) + len,
        };
        buffer_ptr += len;
    }
    result.parse_nbt_payload(scene, buffer_ptr);
    return result;
}

void NbtTag::parse_nbt_payload(GVoxScene &scene, uint8_t *&buffer_ptr) {
    switch (id) {
    case NbtTagId::End:
        // std::cout << "NbtTagId::End:        " << std::endl;
        payload = std::monostate{};
        break;
    case NbtTagId::Byte:
        // std::cout << "NbtTagId::Byte:       " << name << std::endl;
        payload = read_data<NbtTag::Byte>(buffer_ptr);
        break;
    case NbtTagId::Short:
        // std::cout << "NbtTagId::Short:      " << name << std::endl;
        payload = read_data<NbtTag::Short>(buffer_ptr);
        break;
    case NbtTagId::Int:
        // std::cout << "NbtTagId::Int:        " << name << std::endl;
        payload = read_data<NbtTag::Int>(buffer_ptr);
        break;
    case NbtTagId::Long:
        // std::cout << "NbtTagId::Long:       " << name << std::endl;
        payload = read_data<NbtTag::Long>(buffer_ptr);
        break;
    case NbtTagId::Float:
        // std::cout << "NbtTagId::Float:      " << name << std::endl;
        payload = read_data<NbtTag::Float>(buffer_ptr);
        break;
    case NbtTagId::Double:
        // std::cout << "NbtTagId::Double:     " << name << std::endl;
        payload = read_data<NbtTag::Double>(buffer_ptr);
        break;
    case NbtTagId::Byte_Array:
        // std::cout << "NbtTagId::Byte_Array: " << name << std::endl;
        payload = NbtTag::Byte_Array{
            .size = as_le(read_data<NbtTag::Int>(buffer_ptr)),
            .data = reinterpret_cast<NbtTag::Byte *>(buffer_ptr),
        };
        buffer_ptr += static_cast<size_t>(std::get<NbtTag::Byte_Array>(payload).size) * sizeof(NbtTag::Byte);
        break;
    case NbtTagId::String: {
        // std::cout << "NbtTagId::String:     " << name << std::endl;
        auto size = as_le(read_data<std::uint16_t>(buffer_ptr));
        payload = NbtTag::String{
            reinterpret_cast<char *>(buffer_ptr),
            reinterpret_cast<char *>(buffer_ptr) + size,
        };
        buffer_ptr += size;
    } break;
    case NbtTagId::List: {
        // std::cout << "NbtTagId::List:       " << name << std::endl;
        auto result_list = std::make_shared<NbtTag::List>();
        result_list->payloads_tag_id = read_data<NbtTagId>(buffer_ptr);
        result_list->payload_n = as_le(read_data<std::int32_t>(buffer_ptr));
        result_list->payloads.reserve(static_cast<size_t>(result_list->payload_n));
        auto temp_tag = NbtTag{
            .id = result_list->payloads_tag_id,
            .name = "",
        };
        for (std::int32_t list_i = 0; list_i < result_list->payload_n; ++list_i) {
            temp_tag.parse_nbt_payload(scene, buffer_ptr);
            result_list->payloads.push_back(std::move(temp_tag.payload));
        }
        payload = std::move(result_list);
    } break;
    case NbtTagId::Compound: {
        // std::cout << "NbtTagId::Compound:   " << name << std::endl;
        payload = std::monostate{};
        auto tag = NbtTag{};
        do {
            tag = parse_nbt_tag(scene, buffer_ptr);
        } while (tag.id != NbtTagId::End);
    } break;
    case NbtTagId::Int_Array:
        // std::cout << "NbtTagId::Int_Array:  " << name << std::endl;
        payload = NbtTag::Int_Array{
            .size = as_le(read_data<NbtTag::Int>(buffer_ptr)),
            .data = reinterpret_cast<NbtTag::Int *>(buffer_ptr),
        };
        buffer_ptr += static_cast<size_t>(std::get<NbtTag::Byte_Array>(payload).size) * sizeof(NbtTag::Int);
        break;
    case NbtTagId::Long_Array:
        // std::cout << "NbtTagId::Long_Array: " << name << std::endl;
        payload = NbtTag::Long_Array{
            .size = as_le(read_data<NbtTag::Int>(buffer_ptr)),
            .data = reinterpret_cast<NbtTag::Long *>(buffer_ptr),
        };
        buffer_ptr += static_cast<size_t>(std::get<NbtTag::Long_Array>(payload).size) * sizeof(NbtTag::Long);
        break;
    }
}

auto MinecraftContext::parse_payload(GVoxPayload payload) -> GVoxScene {
    assert(payload.size > sizeof(McrHeader));
    auto *buffer_ptr = payload.data;
    auto const &header = read_data_ref<McrHeader>(buffer_ptr);
    size_t xi = 31; // in-region chunk indices
    size_t zi = 31;
    auto const i = xi + zi * 32;
    auto data = header.offsets[i];
    auto offset = as_le(data);
    // auto size = offset & 0xff;
    offset = offset >> 0x08;
    buffer_ptr = payload.data + offset * 4096;
    auto compressed_size = as_le(read_data<uint32_t>(buffer_ptr));
    auto compression_type = read_data<uint8_t>(buffer_ptr);
    assert(compression_type == 2);
    z_stream infstream;
    size_t uncompressed_size = compressed_size;
    auto *uncompressed_data = static_cast<uint8_t *>(realloc(NULL, uncompressed_size));
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = (uInt)compressed_size;
    infstream.next_in = (Bytef *)buffer_ptr;
    infstream.avail_out = (uInt)uncompressed_size;
    infstream.next_out = (Bytef *)uncompressed_data;
    inflateInit(&infstream);
    int inflate_err = Z_OK;
    while (infstream.avail_in > 0) {
        inflate_err = inflate(&infstream, Z_NO_FLUSH);
        if (inflate_err == Z_STREAM_END) {
            break;
        } else if (inflate_err < 0) {
            break;
        }
        auto add_size = static_cast<size_t>(uncompressed_size);
        auto new_size = uncompressed_size + add_size;
        uncompressed_data = static_cast<uint8_t *>(realloc(uncompressed_data, new_size));
        infstream.avail_out = static_cast<uLong>(new_size - infstream.total_out);
        infstream.next_out = (Bytef *)(uncompressed_data + infstream.total_out);
        uncompressed_size = new_size;
    }
    inflateEnd(&infstream);
    assert(inflate_err >= 0);
    auto chunk_buffer_ptr = uncompressed_data;
    GVoxScene result = {};
    result.node_n = 1;
    result.nodes = (GVoxSceneNode *)std::malloc(sizeof(GVoxSceneNode) * result.node_n);
    result.nodes[0].size_x = 16;
    result.nodes[0].size_y = 16;
    result.nodes[0].size_z = 320;
    size_t const voxels_size = result.nodes[0].size_x * result.nodes[0].size_y * result.nodes[0].size_z * sizeof(GVoxVoxel);
    result.nodes[0].voxels = (GVoxVoxel *)std::malloc(voxels_size);
    parse_nbt_tag(result, chunk_buffer_ptr);
    // auto file = std::ofstream("test.nbt", std::ios::binary);
    // if (!file.is_open()) {
    //     break;
    // }
    // file.write(reinterpret_cast<char const *>(uncompressed_data), static_cast<std::streamsize>(infstream.total_out));
    // file.close();
    free(uncompressed_data);
    return result;
}

extern "C" EXPORT auto gvox_format_minecraft_create_context() -> void * {
    auto *result = new MinecraftContext{};
    return result;
}

extern "C" EXPORT void gvox_format_minecraft_destroy_context(void *context_ptr) {
    auto *self = reinterpret_cast<MinecraftContext *>(context_ptr);
    delete self;
}

extern "C" EXPORT auto gvox_format_minecraft_create_payload(void *context_ptr, GVoxScene const *scene) -> GVoxPayload {
    auto *self = reinterpret_cast<MinecraftContext *>(context_ptr);
    return self->create_payload(*scene);
}

extern "C" EXPORT void gvox_format_minecraft_destroy_payload(void *context_ptr, GVoxPayload const *payload) {
    auto *self = reinterpret_cast<MinecraftContext *>(context_ptr);
    self->destroy_payload(*payload);
}

extern "C" EXPORT auto gvox_format_minecraft_parse_payload(void *context_ptr, GVoxPayload const *payload) -> GVoxScene {
    auto *self = reinterpret_cast<MinecraftContext *>(context_ptr);
    return self->parse_payload(*payload);
}
