#pragma once

#include <gvox/gvox.h>
#include <bit>
#include <array>
#include <vector>
#include <string>
#include <variant>

#include <cstdlib>
#include <cstring>

namespace magicavoxel {
    static constexpr uint32_t CHUNK_ID_VOX_ = std::bit_cast<uint32_t>(std::array{'V', 'O', 'X', ' '});
    static constexpr uint32_t CHUNK_ID_MAIN = std::bit_cast<uint32_t>(std::array{'M', 'A', 'I', 'N'});
    static constexpr uint32_t CHUNK_ID_SIZE = std::bit_cast<uint32_t>(std::array{'S', 'I', 'Z', 'E'});
    static constexpr uint32_t CHUNK_ID_XYZI = std::bit_cast<uint32_t>(std::array{'X', 'Y', 'Z', 'I'});
    static constexpr uint32_t CHUNK_ID_RGBA = std::bit_cast<uint32_t>(std::array{'R', 'G', 'B', 'A'});
    static constexpr uint32_t CHUNK_ID_nTRN = std::bit_cast<uint32_t>(std::array{'n', 'T', 'R', 'N'});
    static constexpr uint32_t CHUNK_ID_nGRP = std::bit_cast<uint32_t>(std::array{'n', 'G', 'R', 'P'});
    static constexpr uint32_t CHUNK_ID_nSHP = std::bit_cast<uint32_t>(std::array{'n', 'S', 'H', 'P'});
    static constexpr uint32_t CHUNK_ID_IMAP = std::bit_cast<uint32_t>(std::array{'I', 'M', 'A', 'P'});
    static constexpr uint32_t CHUNK_ID_LAYR = std::bit_cast<uint32_t>(std::array{'L', 'A', 'Y', 'R'});
    static constexpr uint32_t CHUNK_ID_MATL = std::bit_cast<uint32_t>(std::array{'M', 'A', 'T', 'L'});
    static constexpr uint32_t CHUNK_ID_MATT = std::bit_cast<uint32_t>(std::array{'M', 'A', 'T', 'T'});

    static constexpr uint32_t MATERIAL_METAL_BIT = 1 << 0;
    static constexpr uint32_t MATERIAL_ROUGH_BIT = 1 << 1;
    static constexpr uint32_t MATERIAL_SPEC_BIT = 1 << 2;
    static constexpr uint32_t MATERIAL_IOR_BIT = 1 << 3;
    static constexpr uint32_t MATERIAL_ATT_BIT = 1 << 4;
    static constexpr uint32_t MATERIAL_FLUX_BIT = 1 << 5;
    static constexpr uint32_t MATERIAL_EMIT_BIT = 1 << 6;
    static constexpr uint32_t MATERIAL_LDR_BIT = 1 << 7;
    static constexpr uint32_t MATERIAL_TRANS_BIT = 1 << 8;
    static constexpr uint32_t MATERIAL_ALPHA_BIT = 1 << 9;
    static constexpr uint32_t MATERIAL_D_BIT = 1 << 10;
    static constexpr uint32_t MATERIAL_SP_BIT = 1 << 11;
    static constexpr uint32_t MATERIAL_G_BIT = 1 << 12;
    static constexpr uint32_t MATERIAL_MEDIA_BIT = 1 << 13;

    enum class MaterialType {
        DIFFUSE = 0,
        METAL = 1,
        GLASS = 2,
        EMIT = 3,
        BLEND = 4,
        MEDIA = 5,
    };

    struct Header {
        uint32_t file_header = 0;
        uint32_t file_version = 0;
    };

    struct ChunkHeader {
        uint32_t id = 0;
        uint32_t size = 0;
        uint32_t child_size = 0;
    };

    struct Material {
        uint32_t content_flags;
        MaterialType type;
        float metal;
        float rough;
        float spec;
        float ior;
        float att;
        float flux;
        float emit;
        float ldr;
        float trans;
        float alpha;
        float d;
        float sp;
        float g;
        float media;
    };

    struct Color {
        uint8_t r, g, b, a;
    };
    using Palette = std::array<Color, 256>;
    using MaterialList = std::array<Material, 256>;

    struct Layer {
        std::string name;
        Color color{};
        bool hidden{};
    };

    struct Transform {
        GvoxOffset3D offset{0, 0, 0};
        int8_t rotation{1 << 2};
    };

    struct TransformKeyframe {
        uint32_t frame_index{};
        Transform transform;
    };

    struct SceneTransformInfo {
        std::array<char, 65> name{};
        Transform transform;
        uint32_t child_node_id{};
        uint32_t layer_id{};
        bool hidden{};
        uint32_t num_keyframes{};
        size_t keyframe_offset{};
        bool loop{};
    };

    struct SceneGroupInfo {
        uint32_t first_child_node_id_index;
        uint32_t num_child_nodes;
    };

    struct SceneShapeInfo {
        uint32_t model_id;
        uint32_t num_keyframes;
        size_t keyframe_offset;
        bool loop;
    };

    using SceneNodeInfo = std::variant<SceneTransformInfo, SceneGroupInfo, SceneShapeInfo>;

    struct SceneInfo {
        std::vector<uint32_t> group_children_ids{};
        std::vector<magicavoxel::SceneNodeInfo> node_infos{};
    };

    struct Model {
        std::array<uint32_t, 3> extent{};
        uint32_t voxel_count{};
        int64_t input_offset{};

        [[nodiscard]] auto valid() const -> bool {
            return input_offset != 0;
        }
    };

    struct ModelKeyframe {
        uint32_t frame_index;
        uint32_t model_index;
    };

    static constexpr uint32_t MAX_DICT_SIZE = 4096;
    static constexpr uint32_t MAX_DICT_PAIRS = 256;
    struct Dictionary {
        std::array<char const *, MAX_DICT_PAIRS> keys;
        std::array<char const *, MAX_DICT_PAIRS> values;
        uint32_t num_key_value_pairs;
        std::array<char, MAX_DICT_SIZE + 4> buffer; // max 4096, +4 for safety
        uint32_t buffer_mem_used;
        template <typename T>
        auto get(char const *requested_key, T const &default_value) -> T const & {
            for (uint32_t key_i = 0u; key_i < num_key_value_pairs; ++key_i) {
                if (strcmp(keys.at(key_i), requested_key) == 0) {
                    if constexpr (std::is_same_v<T, char const *>) {
                        return values.at(key_i);
                    } else {
                        return *reinterpret_cast<T const *>(values.at(key_i));
                    }
                }
            }
            return default_value;
        }
    };

    inline auto read_dict(GvoxInputStream input_stream, Dictionary &temp_dict) -> bool {
        uint32_t num_pairs_to_read = 0;
        gvox_input_read(input_stream, &num_pairs_to_read, sizeof(num_pairs_to_read));
        if (num_pairs_to_read > MAX_DICT_PAIRS) {
            return false;
        }
        temp_dict.buffer_mem_used = 0;
        temp_dict.num_key_value_pairs = 0;
        for (uint32_t i = 0; i < num_pairs_to_read; i++) {
            uint32_t key_string_size = 0;
            gvox_input_read(input_stream, &key_string_size, sizeof(key_string_size));
            if (temp_dict.buffer_mem_used + key_string_size > MAX_DICT_SIZE) {
                return false;
            }
            char *key = &temp_dict.buffer[temp_dict.buffer_mem_used];
            temp_dict.buffer_mem_used += key_string_size + 1;
            gvox_input_read(input_stream, key, key_string_size);
            key[key_string_size] = 0;
            uint32_t value_string_size = 0;
            gvox_input_read(input_stream, &value_string_size, sizeof(value_string_size));
            if (temp_dict.buffer_mem_used + value_string_size > MAX_DICT_SIZE) {
                return false;
            }
            char *value = &temp_dict.buffer[temp_dict.buffer_mem_used];
            temp_dict.buffer_mem_used += value_string_size + 1;
            gvox_input_read(input_stream, value, value_string_size);
            value[value_string_size] = 0;
            temp_dict.keys[temp_dict.num_key_value_pairs] = key;
            temp_dict.values[temp_dict.num_key_value_pairs] = value;
            temp_dict.num_key_value_pairs++;
        }
        return true;
    };

    inline void string_r_dict_entry(char const *str, int8_t &rotation) {
        rotation = static_cast<int8_t>(atoi(str));
    }

    inline void string_t_dict_entry(char const *str, GvoxOffset3D &offset) {
        auto s = std::string{str};
        for (size_t count = 0; count < 3; ++count) {
            auto pos = s.find(' ');
            auto token = s.substr(0, pos);
            switch (count) {
            case 0: offset.data[0] = atoi(token.c_str()); break;
            case 1: offset.data[1] = atoi(token.c_str()); break;
            case 2: offset.data[2] = atoi(token.c_str()); break;
            }
            s.erase(0, pos + 1);
        }
    }

    inline void string_color_dict_entry(char const *str, uint32_t &r, uint32_t &g, uint32_t &b) {
        auto s = std::string{str};
        for (size_t count = 0; count < 3; ++count) {
            auto pos = s.find(' ');
            auto token = s.substr(0, pos);
            switch (count) {
            case 0: r = static_cast<uint32_t>(atoi(token.c_str())); break;
            case 1: g = static_cast<uint32_t>(atoi(token.c_str())); break;
            case 2: b = static_cast<uint32_t>(atoi(token.c_str())); break;
            }
            s.erase(0, pos + 1);
        }
    }

    constexpr auto rotate(int8_t packed_rotation_bits, GvoxOffset3D offset) {
        auto result = std::array<int64_t, 3>{};
        constexpr auto row2_index = std::array<uint32_t, 5>{2, UINT32_MAX, 1, 0, UINT32_MAX};
        uint32_t const row0_vec_index = (packed_rotation_bits >> 0) & 3;
        uint32_t const row1_vec_index = (packed_rotation_bits >> 2) & 3;
        uint32_t const row2_vec_index = row2_index[((1 << row0_vec_index) | (1 << row1_vec_index)) - 3];
        result[row0_vec_index] = offset.data[0];
        result[row1_vec_index] = offset.data[1];
        result[row2_vec_index] = offset.data[2];
        if ((packed_rotation_bits & (1 << 4)) != 0) {
            result[0] *= -1;
        }
        if ((packed_rotation_bits & (1 << 5)) != 0) {
            result[1] *= -1;
        }
        if ((packed_rotation_bits & (1 << 6)) != 0) {
            result[2] *= -1;
        }
        return std::bit_cast<GvoxOffset3D>(result);
    }
    constexpr auto rotate(int8_t packed_rotation_bits, GvoxExtent3D extent) {
        auto result = std::array<uint64_t, 3>{};
        constexpr auto row2_index = std::array<uint32_t, 5>{2, UINT32_MAX, 1, 0, UINT32_MAX};
        uint32_t const row0_vec_index = (packed_rotation_bits >> 0) & 3;
        uint32_t const row1_vec_index = (packed_rotation_bits >> 2) & 3;
        uint32_t const row2_vec_index = row2_index[((1 << row0_vec_index) | (1 << row1_vec_index)) - 3];
        result[row0_vec_index] = extent.data[0];
        result[row1_vec_index] = extent.data[1];
        result[row2_vec_index] = extent.data[2];
        return std::bit_cast<GvoxExtent3D>(result);
    }
    constexpr auto rotate(int8_t packed_rotation_bits, GvoxExtent3D p, GvoxExtent3D extent) {
        auto result = std::array<uint64_t, 3>{};
        constexpr auto row2_index = std::array<uint32_t, 5>{2, UINT32_MAX, 1, 0, UINT32_MAX};
        uint32_t const row0_vec_index = (packed_rotation_bits >> 0) & 3;
        uint32_t const row1_vec_index = (packed_rotation_bits >> 2) & 3;
        uint32_t const row2_vec_index = row2_index[((1 << row0_vec_index) | (1 << row1_vec_index)) - 3];
        result[row0_vec_index] = p.data[0];
        result[row1_vec_index] = p.data[1];
        result[row2_vec_index] = p.data[2];
        auto extent_arr = std::bit_cast<std::array<uint64_t, 3>>(extent);
        if ((packed_rotation_bits & (1 << 4)) != 0) {
            result[row0_vec_index] = extent_arr[row0_vec_index] - 1 - result[row0_vec_index];
        }
        if ((packed_rotation_bits & (1 << 5)) != 0) {
            result[row1_vec_index] = extent_arr[row1_vec_index] - 1 - result[row1_vec_index];
        }
        if ((packed_rotation_bits & (1 << 6)) != 0) {
            result[row2_vec_index] = extent_arr[row2_vec_index] - 1 - result[row2_vec_index];
        }
        return std::bit_cast<GvoxExtent3D>(result);
    }
    constexpr auto rotate(int8_t rot_b, int8_t rot_a) {
        constexpr auto row2_index = std::array<int8_t, 5>{2, -1, 1, 0, -1};
        auto ai = std::array<int8_t, 3>{static_cast<int8_t>(rot_a & 3), static_cast<int8_t>((rot_a >> 2) & 3), 0};
        ai[2] = row2_index[static_cast<size_t>((1 << ai[0]) | (1 << ai[1])) - 3];
        auto bi = std::array<int8_t, 3>{static_cast<int8_t>(rot_b & 3), static_cast<int8_t>((rot_b >> 2) & 3), 0};
        bi[2] = row2_index[static_cast<size_t>((1 << bi[0]) | (1 << bi[1])) - 3];
        int8_t const c0_i = ai[static_cast<size_t>(bi[0])];
        int8_t const c1_i = ai[static_cast<size_t>(bi[1])];
        auto result = static_cast<int8_t>((c0_i << 0) | (c1_i << 2));
        if ((((rot_a >> (4 + bi[0])) & 1) ^ ((rot_b >> 4) & 1)) == 1) {
            result |= 1 << 4;
        }
        if ((((rot_a >> (4 + bi[1])) & 1) ^ ((rot_b >> 5) & 1)) == 1) {
            result |= 1 << 5;
        }
        if ((((rot_a >> (4 + bi[2])) & 1) ^ ((rot_b >> 6) & 1)) == 1) {
            result |= 1 << 6;
        }
        return result;
    }
    constexpr auto inverse(int8_t rot_a) {
        constexpr auto row2_index = std::array<int8_t, 5>{2, -1, 1, 0, -1};
        auto ai = std::array<int8_t, 3>{static_cast<int8_t>(rot_a & 3), static_cast<int8_t>((rot_a >> 2) & 3), 0};
        ai[2] = row2_index[static_cast<size_t>((1 << ai[0]) | (1 << ai[1])) - 3];
        auto bi = ai;
        bi[static_cast<size_t>(ai[0])] = 0;
        bi[static_cast<size_t>(ai[1])] = 1;
        bi[static_cast<size_t>(ai[2])] = 2;
        auto result = static_cast<int8_t>((bi[0] << 0) | (bi[1] << 2));
        if (((rot_a >> 4) & 1) == 1) {
            result |= 1 << (4 + ai[0]);
        }
        if (((rot_a >> 5) & 1) == 1) {
            result |= 1 << (4 + ai[1]);
        }
        if (((rot_a >> 6) & 1) == 1) {
            result |= 1 << (4 + ai[2]);
        }
        return result;
    }

    namespace xraw {
        static constexpr uint32_t MAGIC = std::bit_cast<uint32_t>(std::array{'X', 'R', 'A', 'W'});

        enum class ColorChannelDataType : uint8_t {
            U32,
            I32,
            F32,
        };

        struct Header {
            uint32_t magic;
            ColorChannelDataType color_channel_data_type;
            uint8_t color_channel_n;
            uint8_t bits_per_channel;
            uint8_t bits_per_index; // if 0, no palette
            uint32_t volume_size_x;
            uint32_t volume_size_y;
            uint32_t volume_size_z;
            uint32_t palette_size;

            [[nodiscard]] auto valid() const -> bool {
                if (magic != MAGIC) {
                    return false;
                }
                auto c = static_cast<uint8_t>(color_channel_data_type);
                if (c > 2) {
                    return false;
                }
                if (color_channel_n > 4) {
                    return false;
                }
                if (bits_per_channel != 8 && bits_per_channel != 16 && bits_per_channel != 32) {
                    return false;
                }
                if (bits_per_index != 8 && bits_per_index != 16 && bits_per_index != 0) {
                    return false;
                }
                if (palette_size != 256 && palette_size != 32768) {
                    return false;
                }
                return true;
            }
        };
    } // namespace xraw
} // namespace magicavoxel
