#include <gvox/gvox.h>

#include <cstdlib>
#include <cstring>
#include <cassert>

#include <algorithm>
#include <array>
#include <variant>
#include <vector>
#include <unordered_map>
#include <string_view>
#include <utility>
#include <memory>
#include <bit>

#include <filesystem>

#define PRINT_PARSE 0

// #include <iostream>

#if PRINT_PARSE
#include <iostream>
#include <iomanip>
#define PARSE_INDENT_ARG , std::string const &indent
#define PARSE_INDENT_ARG_DEF , std::string const &indent = ""
#define PARSE_INDENT_ARG_VAL , indent
#define PARSE_INDENT_ARG_VAL_ADD , indent + "    "
#else
#define PARSE_INDENT_ARG
#define PARSE_INDENT_ARG_DEF
#define PARSE_INDENT_ARG_VAL
#define PARSE_INDENT_ARG_VAL_ADD
#endif

#include <zlib.h>
#include <minizip/unzip.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <nlohmann/json.hpp>

#if GVOX_FORMAT_MINECRAFT_BUILT_STATIC
#define EXPORT
#else
#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport)
#endif
#endif

static constexpr auto ceil_log2(size_t x) -> size_t {
    constexpr auto const t = std::array<size_t, 6>{
        0xFFFFFFFF00000000u,
        0x00000000FFFF0000u,
        0x000000000000FF00u,
        0x00000000000000F0u,
        0x000000000000000Cu,
        0x0000000000000002u};

    size_t y = (((x & (x - 1)) == 0) ? 0 : 1);
    int j = 32;

    for (size_t const i : t) {
        int const k = (((x & i) == 0) ? 0 : j);
        y += static_cast<size_t>(k);
        x >>= k;
        j >>= 1;
    }

    return y;
}

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
    using Byte = std::int8_t;
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
    struct Compound {
        std::unordered_map<std::string_view, std::shared_ptr<NbtTag>> tags;
    };
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
        Compound,
        Int_Array,
        Long_Array>;

    struct List {
        NbtTagId payloads_tag_id;
        std::vector<Payload> payloads;
    };

    NbtTagId id;
    std::string_view name;
    Payload payload;

    void parse_nbt_payload(uint8_t *&buffer_ptr PARSE_INDENT_ARG_DEF);
};

auto parse_nbt_tag(uint8_t *&buffer_ptr PARSE_INDENT_ARG_DEF) -> NbtTag {
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
    result.parse_nbt_payload(buffer_ptr PARSE_INDENT_ARG_VAL);
    return result;
}

void NbtTag::parse_nbt_payload(uint8_t *&buffer_ptr PARSE_INDENT_ARG) {
    switch (id) {
    case NbtTagId::End:
        payload = std::monostate{};
        break;
    case NbtTagId::Byte:
        payload = read_data<NbtTag::Byte>(buffer_ptr);
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mByte\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = \033[38;2;181;206;168m" << static_cast<int32_t>(std::get<NbtTag::Byte>(payload)) << "\033[0m" << std::endl;
#endif
        break;
    case NbtTagId::Short:
        payload = read_data<NbtTag::Short>(buffer_ptr);
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mShort\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = \033[38;2;181;206;168m" << as_le(std::get<NbtTag::Short>(payload)) << "\033[0m" << std::endl;
#endif
        break;
    case NbtTagId::Int:
        payload = read_data<NbtTag::Int>(buffer_ptr);
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mInt\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = \033[38;2;181;206;168m" << as_le(std::get<NbtTag::Int>(payload)) << "\033[0m" << std::endl;
#endif
        break;
    case NbtTagId::Long:
        payload = read_data<NbtTag::Long>(buffer_ptr);
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mLong\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = \033[38;2;181;206;168m" << as_le(std::get<NbtTag::Long>(payload)) << "\033[0m" << std::endl;
#endif
        break;
    case NbtTagId::Float:
        payload = read_data<NbtTag::Float>(buffer_ptr);
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mFloat\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = \033[38;2;181;206;168m" << as_le(std::get<NbtTag::Float>(payload)) << "\033[0m" << std::endl;
#endif
        break;
    case NbtTagId::Double:
        payload = read_data<NbtTag::Double>(buffer_ptr);
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mDouble\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = \033[38;2;181;206;168m" << as_le(std::get<NbtTag::Double>(payload)) << "\033[0m" << std::endl;
#endif
        break;
    case NbtTagId::Byte_Array:
        payload = NbtTag::Byte_Array{
            .size = as_le(read_data<NbtTag::Int>(buffer_ptr)),
            .data = reinterpret_cast<NbtTag::Byte *>(buffer_ptr),
        };
        buffer_ptr += static_cast<size_t>(std::get<NbtTag::Byte_Array>(payload).size) * sizeof(NbtTag::Byte);
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mByte_Array\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = { .size = \033[38;2;181;206;168m" << std::get<NbtTag::Byte_Array>(payload).size << "\033[0m }" << std::endl;
#endif
        break;
    case NbtTagId::String: {
        auto size = as_le(read_data<std::uint16_t>(buffer_ptr));
        payload = NbtTag::String{
            reinterpret_cast<char *>(buffer_ptr),
            reinterpret_cast<char *>(buffer_ptr) + size,
        };
        buffer_ptr += size;
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mString\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = \033[38;2;206;145;120m\"" << std::get<NbtTag::String>(payload) << "\"\033[0m" << std::endl;
#endif
    } break;
    case NbtTagId::List: {
        auto result_list = std::make_shared<NbtTag::List>();
        result_list->payloads_tag_id = read_data<NbtTagId>(buffer_ptr);
        auto payload_n = as_le(read_data<std::int32_t>(buffer_ptr));
        result_list->payloads.reserve(static_cast<size_t>(payload_n));
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;206;145;120m\"" << name << "\"\033[0m[\033[38;2;181;206;168m" << payload_n << "\033[0m] [" << std::endl;
#endif
        auto temp_tag = NbtTag{
            .id = result_list->payloads_tag_id,
            .name = "",
        };
        for (std::int32_t list_i = 0; list_i < payload_n; ++list_i) {
            temp_tag.parse_nbt_payload(buffer_ptr PARSE_INDENT_ARG_VAL_ADD);
            result_list->payloads.push_back(std::move(temp_tag.payload));
        }
#if PRINT_PARSE
        std::cout << indent << "]" << std::endl;
#endif
        payload = std::move(result_list);
    } break;
    case NbtTagId::Compound: {
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;206;145;120m\"" << name << "\"\033[0m {" << std::endl;
#endif
        payload = NbtTag::Compound{};
        auto tag = NbtTag{};
        while (true) {
            tag = parse_nbt_tag(buffer_ptr PARSE_INDENT_ARG_VAL_ADD);
            if (tag.id == NbtTagId::End)
                break;
            std::get<NbtTag::Compound>(payload).tags[tag.name] = std::make_shared<NbtTag>(tag);
        }
#if PRINT_PARSE
        std::cout << indent << "}" << std::endl;
#endif
    } break;
    case NbtTagId::Int_Array:
        payload = NbtTag::Int_Array{
            .size = as_le(read_data<NbtTag::Int>(buffer_ptr)),
            .data = reinterpret_cast<NbtTag::Int *>(buffer_ptr),
        };
        buffer_ptr += static_cast<size_t>(std::get<NbtTag::Int_Array>(payload).size) * sizeof(NbtTag::Int);
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mInt_Array\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = { .size = \033[38;2;181;206;168m" << std::get<NbtTag::Int_Array>(payload).size << "\033[0m }" << std::endl;
#endif
        break;
    case NbtTagId::Long_Array:
        payload = NbtTag::Long_Array{
            .size = as_le(read_data<NbtTag::Int>(buffer_ptr)),
            .data = reinterpret_cast<NbtTag::Long *>(buffer_ptr),
        };
        buffer_ptr += static_cast<size_t>(std::get<NbtTag::Long_Array>(payload).size) * sizeof(NbtTag::Long);
#if PRINT_PARSE
        std::cout << indent << "\033[38;2;078;201;176mLong_Array\033[0m: \033[38;2;206;145;120m\"" << name << "\"\033[0m = { .size = \033[38;2;181;206;168m" << std::get<NbtTag::Long_Array>(payload).size << "\033[0m }" << std::endl;
#endif
        break;
    }
}

constexpr auto get_mask(uint64_t bits_per_element) -> uint64_t {
    return (~0ull) >> (64 - bits_per_element);
}

struct Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

void print_pixels(Pixel *pixels, size_t size_x, size_t size_y) {
    for (size_t yi = 0; yi < size_y; yi += 1) {
        for (size_t xi = 0; xi < size_x; xi += 1) {
            size_t i = xi + yi * size_x;
            auto &pixel = pixels[i];
            printf("\033[48;2;%03d;%03d;%03dm", (uint32_t)(pixel.r), (uint32_t)(pixel.g), (uint32_t)(pixel.b));
            char c = ' ';
            fputc(c, stdout);
            fputc(c, stdout);
        }
        printf("\033[0m\n");
    }
}

auto MinecraftContext::parse_payload(GVoxPayload payload) -> GVoxScene {
    GVoxScene result = {};
    result.node_n = 1;
    result.nodes = (GVoxSceneNode *)std::malloc(sizeof(GVoxSceneNode) * result.node_n);
    auto &node = result.nodes[0];
    node.size_x = 16;
    node.size_y = 16;
    node.size_z = 384;
    size_t const voxels_size = node.size_x * node.size_y * node.size_z * sizeof(GVoxVoxel);
    node.voxels = (GVoxVoxel *)std::malloc(voxels_size);
    std::memset(node.voxels, 0, voxels_size);
    assert(payload.size > sizeof(McrHeader));
    auto *buffer_ptr = payload.data;
    auto const &header = read_data_ref<McrHeader>(buffer_ptr);
    auto jar_filepath = std::filesystem::path{"C:/Users/gabe/AppData/Roaming/.minecraft/versions/1.19.3/1.19.3.jar"};
    auto jar_zip = unzOpen(jar_filepath.string().c_str());
    assert(jar_zip != NULL);
    auto load_from_jar = [&](char const *path) {
        int err;
        err = unzLocateFile(jar_zip, path, 1);
        assert(err == UNZ_OK);
        auto file_info = unz_file_info{};
        err = unzGetCurrentFileInfo(jar_zip, &file_info, nullptr, 0, nullptr, 0, nullptr, 0);
        assert(err == UNZ_OK);
        auto file_data = std::vector<uint8_t>{};
        file_data.resize(file_info.uncompressed_size);
        err = unzOpenCurrentFile(jar_zip);
        assert(err == UNZ_OK);
        err = unzReadCurrentFile(jar_zip, file_data.data(), file_data.size());
        assert(err == file_data.size());
        return file_data;
    };
    struct BlockInfo {
        size_t model_variant_n{};
        Pixel avg_color{};
    };
    auto load_json_from_jar = [&](std::string const &path) -> nlohmann::json {
        auto json_data = load_from_jar(path.c_str());
        auto json_string = std::string_view{reinterpret_cast<char *>(json_data.data()), json_data.size()};
        return nlohmann::json::parse(json_string);
    };
    auto make_blockstate_json_path = [](std::string const block_name_str) -> std::string {
        return "assets/minecraft/blockstates/" + block_name_str + ".json";
    };
    auto block_infos = std::unordered_map<std::string, BlockInfo>{};
    auto lookup_block_info = [&](std::string_view block_name) -> BlockInfo const & {
        auto block_name_str = std::string{block_name};
        auto result_iter = block_infos.find(block_name_str);
        if (result_iter == block_infos.end()) {
            auto &block_info = block_infos[block_name_str] = {};
            auto blockstate_json = load_json_from_jar(make_blockstate_json_path(block_name_str));
            if (blockstate_json.contains("variants")) {
                auto const &variants = blockstate_json.at("variants");
                for (auto const &item : variants.items()) {
                    ++block_info.model_variant_n;
                }
                if (block_info.model_variant_n == 1) {
                    // auto model_name = variants.at("")[0].at("model").get<std::string>();
                    // auto block_model_json = load_json_from_jar("assets/minecraft/models/" + model_name.substr(10, model_name.size() - 10) + ".json");
                    // auto parent = variants[0].at("parent").get<std::string>();
                    // std::cout << parent << " -> ";
                }
            }
            // std::cout << block_name << ": " << block_info.model_variant_n << std::endl;
            return block_info;
        } else {
            return result_iter->second;
        }
    };
    // auto const &block_tex_info = lookup_block_tex_info("diamond_ore");

    // auto png_data = load_from_jar("assets/minecraft/textures/block/diamond_ore.png");
    // int size_x{}, size_y{}, channel_n{};
    // auto *pixels = reinterpret_cast<Pixel *>(stbi_load_from_memory(png_data.data(), png_data.size(), &size_x, &size_y, &channel_n, 4));
    // print_pixels(pixels, size_x, size_y);

    size_t start_chunk_xi = 0;
    size_t chunk_xn = 1;
    size_t start_chunk_zi = 0;
    size_t chunk_zn = 1;
    for (size_t chunk_zi = 0; chunk_zi < chunk_zn; ++chunk_zi) {
        for (size_t chunk_xi = 0; chunk_xi < chunk_xn; ++chunk_xi) {
            auto const chunk_i = (chunk_xi + start_chunk_xi) + (chunk_zi + start_chunk_zi) * 32;
            auto data = header.offsets[chunk_i];
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
                infstream.avail_out = static_cast<uInt>(new_size - infstream.total_out);
                infstream.next_out = (Bytef *)(uncompressed_data + infstream.total_out);
                uncompressed_size = new_size;
            }
            inflateEnd(&infstream);
            assert(inflate_err >= 0);
            auto chunk_buffer_ptr = uncompressed_data;
            auto root_tag = parse_nbt_tag(chunk_buffer_ptr);
            assert(root_tag.id == NbtTagId::Compound);
            auto &root = std::get<NbtTag::Compound>(root_tag.payload);
            // auto x_pos = as_le(std::get<NbtTag::Int>(root.tags["xPos"]->payload));
            auto y_pos = as_le(std::get<NbtTag::Int>(root.tags["yPos"]->payload));
            // auto z_pos = as_le(std::get<NbtTag::Int>(root.tags["zPos"]->payload));
            auto &sections = *std::get<std::shared_ptr<NbtTag::List>>(root.tags["sections"]->payload);
            // std::cout << x_pos << ", " << y_pos << ", " << z_pos << std::endl;
            for (auto &section_payload : sections.payloads) {
                auto &section = std::get<NbtTag::Compound>(section_payload);
                auto section_y = static_cast<int32_t>(std::get<NbtTag::Byte>(section.tags["Y"]->payload));
                if (section_y >= y_pos) {
                    auto x_off = chunk_xi * 16;
                    auto y_off = (section_y - y_pos) * 16;
                    auto z_off = chunk_zi * 16;
                    // std::cout << "y_off = " << y_off << std::endl;
                    auto &block_states = std::get<NbtTag::Compound>(section.tags["block_states"]->payload);
                    auto &palette = *std::get<std::shared_ptr<NbtTag::List>>(block_states.tags["palette"]->payload);
                    size_t const PALETTED_REGION_SIZE = 16;
                    if (palette.payloads.size() == 1) {
                        auto &block_data = std::get<NbtTag::Compound>(palette.payloads[0]);
                        auto &block_name_id = std::get<NbtTag::String>(block_data.tags["Name"]->payload);
                        // only one block-type in the chunk
                        // std::cout << block_name << std::endl;
                        if (block_name_id != "minecraft:air") {

                            auto block_name = block_name_id.substr(10, block_name_id.size() - 10);
                            auto const &block_info = lookup_block_info(block_name);

                            for (size_t element_zi = 0; element_zi < PALETTED_REGION_SIZE; ++element_zi) {
                                for (size_t element_yi = 0; element_yi < PALETTED_REGION_SIZE; ++element_yi) {
                                    for (size_t element_xi = 0; element_xi < PALETTED_REGION_SIZE; ++element_xi) {
                                        auto const o_index = (element_xi + x_off) + (element_yi + y_off) * node.size_x + (element_zi + z_off) * node.size_x * node.size_y;
                                        node.voxels[o_index] = GVoxVoxel{
                                            .color = {0.5f, 0.5f, 0.5f},
                                            .id = static_cast<uint32_t>(1),
                                        };
                                    }
                                }
                            }
                        }
                    } else {
                        auto const variant_n = palette.payloads.size();
                        auto const bits_per_element = std::max<size_t>(4, ceil_log2(variant_n));
                        auto const elem_per_u64 = 64 / bits_per_element;
                        auto elements = std::get<NbtTag::Long_Array>(block_states.tags["data"]->payload);
                        for (size_t element_zi = 0; element_zi < PALETTED_REGION_SIZE; ++element_zi) {
                            for (size_t element_yi = 0; element_yi < PALETTED_REGION_SIZE; ++element_yi) {
                                for (size_t element_xi = 0; element_xi < PALETTED_REGION_SIZE; ++element_xi) {
                                    auto const element_index = element_xi + element_yi * PALETTED_REGION_SIZE + element_zi * PALETTED_REGION_SIZE * PALETTED_REGION_SIZE;
                                    auto const u64_index = element_index / elem_per_u64;
                                    auto const in_64_index = element_index - u64_index * elem_per_u64;
                                    auto const mask = get_mask(bits_per_element);
                                    auto const o_index = (node.size_x - 1 - (element_xi + x_off)) + (element_yi + z_off) * node.size_x + (element_zi + y_off) * node.size_x * node.size_y;
                                    auto palette_index = (as_le(elements.data[u64_index]) >> (in_64_index * bits_per_element)) & mask;
                                    assert(palette_index < variant_n);
                                    auto &block_data = std::get<NbtTag::Compound>(palette.payloads[palette_index]);
                                    auto &block_name_id = std::get<NbtTag::String>(block_data.tags["Name"]->payload);
                                    if (block_name_id != "minecraft:air") {

                                        auto block_name = block_name_id.substr(10, block_name_id.size() - 10);
                                        auto const &block_info = lookup_block_info(block_name);

                                        node.voxels[o_index] = GVoxVoxel{
                                            .color = {0.5f, 0.5f, 0.5f},
                                            .id = static_cast<uint32_t>(palette_index),
                                        };
                                        // std::cout << palette_index << " ";
                                    }
                                }
                                // std::cout << " ";
                            }
                            // std::cout << std::endl;
                        }
                    }
                }
            }
            free(uncompressed_data);
        }
    }

    unzClose(jar_zip);

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
