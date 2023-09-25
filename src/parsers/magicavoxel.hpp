#pragma once

#include <gvox/gvox.h>
#include <bit>
#include <array>
#include <vector>
#include <string>
#include <variant>

#define MAGICAVOXEL_USE_WASM_FIX 1

#if MAGICAVOXEL_USE_WASM_FIX
#include <cstdlib>
#include <cstring>
#else
#include <sstream>
#endif

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

    struct Color {
        uint8_t r, g, b, a;
    };

    struct Transform {
        GvoxOffset3D offset{0, 0, 0};
        int8_t rotation{1 << 2};
    };

    constexpr auto rotate(int8_t packed_rotation_bits, GvoxOffset3D offset) {
        auto result = std::array<int64_t, 3>{};
        uint32_t constexpr row2_index[] = {2, UINT32_MAX, 1, 0, UINT32_MAX};
        uint32_t const row0_vec_index = (packed_rotation_bits >> 0) & 3;
        uint32_t const row1_vec_index = (packed_rotation_bits >> 2) & 3;
        uint32_t const row2_vec_index = row2_index[((1 << row0_vec_index) | (1 << row1_vec_index)) - 3];
        result[row0_vec_index] = offset.x;
        result[row1_vec_index] = offset.y;
        result[row2_vec_index] = offset.z;
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
        uint32_t constexpr row2_index[] = {2, UINT32_MAX, 1, 0, UINT32_MAX};
        uint32_t const row0_vec_index = (packed_rotation_bits >> 0) & 3;
        uint32_t const row1_vec_index = (packed_rotation_bits >> 2) & 3;
        uint32_t const row2_vec_index = row2_index[((1 << row0_vec_index) | (1 << row1_vec_index)) - 3];
        result[row0_vec_index] = extent.x;
        result[row1_vec_index] = extent.y;
        result[row2_vec_index] = extent.z;
        return std::bit_cast<GvoxExtent3D>(result);
    }
    constexpr auto rotate(int8_t packed_rotation_bits, GvoxExtent3D p, GvoxExtent3D extent) {
        auto result = std::array<uint64_t, 3>{};
        uint32_t constexpr row2_index[] = {2, UINT32_MAX, 1, 0, UINT32_MAX};
        uint32_t const row0_vec_index = (packed_rotation_bits >> 0) & 3;
        uint32_t const row1_vec_index = (packed_rotation_bits >> 2) & 3;
        uint32_t const row2_vec_index = row2_index[((1 << row0_vec_index) | (1 << row1_vec_index)) - 3];
        result[row0_vec_index] = p.x;
        result[row1_vec_index] = p.y;
        result[row2_vec_index] = p.z;
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
        int8_t constexpr row2_index[] = {2, -1, 1, 0, -1};
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
        int8_t constexpr row2_index[] = {2, -1, 1, 0, -1};
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

    struct TransformKeyframe {
        uint32_t frame_index{};
        Transform transform;
    };

    struct Model {
        GvoxExtent3D extent{};
        std::vector<uint8_t> palette_ids{};
    };

    struct ModelKeyframe {
        uint32_t frame_index;
        uint32_t model_index;
    };

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

    using Palette = std::array<Color, 256>;
    using MaterialList = std::array<Material, 256>;

    struct Layer {
        std::string name;
        Color color{};
        bool hidden{};
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

    struct ModelInstance {
        int8_t rotation{1 << 2};

        GvoxOffset3D aabb_min;
        GvoxOffset3D aabb_max;

        uint32_t index;
    };

    struct BvhNode {
        struct Children {
            uint32_t offset;
        };
        struct Range {
            uint32_t first;
            uint32_t count;
        };
        GvoxOffset3D aabb_min;
        GvoxOffset3D aabb_max;
        std::variant<Children, Range> data;
        [[nodiscard]] auto is_leaf() const -> bool {
            return std::holds_alternative<Range>(data);
        }
        [[nodiscard]] auto needs_subdivide() const -> bool {
            if (std::holds_alternative<Children>(data)) {
                return true;
            }
            auto const &range = std::get<Range>(data);
            return range.count > 2;
        }
    };

    struct Scene {
        std::vector<Model> models{};
        std::vector<ModelInstance> model_instances{};

        Transform transform;

        std::vector<BvhNode> bvh_nodes;
    };

    void string_r_dict_entry(char const *str, int8_t &rotation) {
#if MAGICAVOXEL_USE_WASM_FIX
        rotation = static_cast<int8_t>(atoi(str));
#else
        auto ss = std::stringstream{};
        ss.str(str);
        int32_t temp_int = 0;
        ss >> temp_int;
        rotation = static_cast<int8_t>(temp_int);
#endif
    }

    void string_t_dict_entry(char const *str, GvoxOffset3D &offset) {
#if MAGICAVOXEL_USE_WASM_FIX
        auto s = std::string{str};
        for (size_t count = 0; count < 3; ++count) {
            auto pos = s.find(' ');
            auto token = s.substr(0, pos);
            switch (count) {
            case 0: offset.x = atoi(token.c_str()); break;
            case 1: offset.y = atoi(token.c_str()); break;
            case 2: offset.z = atoi(token.c_str()); break;
            }
            s.erase(0, pos + 1);
        }
#else
        auto ss = std::stringstream{};
        ss.str(str);
        ss >> offset.x >> offset.y >> offset.z;
#endif
    }

    void string_color_dict_entry(char const *str, uint32_t &r, uint32_t &g, uint32_t &b) {
#if MAGICAVOXEL_USE_WASM_FIX
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
#else
        auto ss = std::stringstream{};
        ss.str(str);
        ss >> r >> g >> b;
#endif
    }
} // namespace magicavoxel
