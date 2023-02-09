#include <gvox/gvox.h>
#include <gvox/adapters/parse/magicavoxel.h>

#include <cstdlib>
#include <cstring>

#include <bit>
#include <array>
#include <vector>
#include <variant>
#include <algorithm>
#include <new>
#include <string>

#include <iostream>

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
    static constexpr uint32_t CHUNK_ID_rOBJ = std::bit_cast<uint32_t>(std::array{'r', 'O', 'B', 'J'});
    static constexpr uint32_t CHUNK_ID_rCAM = std::bit_cast<uint32_t>(std::array{'r', 'C', 'A', 'M'});

    struct Color {
        uint8_t r, g, b, a;
    };

    struct Transform {
        GvoxOffset3D offset;
        int8_t rotation;
    };

    struct TransformKeyframe {
        uint32_t frame_index;
        Transform transform;
    };

    struct Model {
        GvoxRegionRange range{};
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
        Color color;
        bool hidden;
    };

    static constexpr uint32_t MAX_DICT_SIZE = 4096;
    static constexpr uint32_t MAX_DICT_PAIRS = 256;
    struct Dictionary {
        char const *keys[MAX_DICT_PAIRS];
        char const *values[MAX_DICT_PAIRS];
        uint32_t num_key_value_pairs;
        char buffer[MAX_DICT_SIZE + 4]; // max 4096, +4 for safety
        uint32_t buffer_mem_used;
        template <typename T>
        auto get(char const *requested_key, T const &default_value) -> T const & {
            for (uint32_t key_i = 0u; key_i < num_key_value_pairs; ++key_i) {
                if (strcmp(keys[key_i], requested_key) == 0) {
                    if constexpr (std::is_same_v<T, char const *>) {
                        return values[key_i];
                    } else {
                        return *reinterpret_cast<T const *>(values[key_i]);
                    }
                }
            }
            return default_value;
        }
    };

    struct SceneTransformInfo {
        char name[65];
        Transform transform;
        uint32_t child_node_id;
        uint32_t layer_id;
        bool hidden;
        uint32_t num_keyframes;
        size_t keyframe_offset;
        bool loop;
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

    struct SceneGroup;
    struct SceneModel;
    using SceneNode = std::unique_ptr<std::variant<SceneGroup, SceneModel>>;

    struct SceneGroup {
        Transform transform;
        GvoxRegionRange range;
        std::vector<SceneNode> nodes;
    };

    struct SceneModel {
        Transform transform;
        uint32_t index;
    };

    struct Scene {
        std::vector<Model> models{};
        std::vector<uint32_t> group_children_ids{};
        std::vector<magicavoxel::SceneNodeInfo> node_infos{};
        SceneNode root_node{};
    };
} // namespace magicavoxel

struct MagicavoxelParseUserState {
    magicavoxel::Scene scene{};
    magicavoxel::Palette palette{};
    magicavoxel::MaterialList materials{};
    std::vector<magicavoxel::TransformKeyframe> transform_keyframes{};
    std::vector<magicavoxel::ModelKeyframe> shape_keyframes{};
    std::vector<magicavoxel::Layer> layers{};
    std::array<uint8_t, 256> index_map{};
    bool found_index_map_chunk{};
    size_t offset{};
};

void sample_scene(magicavoxel::Scene &scene, uint32_t node_index, uint32_t depth, GvoxOffset3D const &sample_pos, uint32_t &sampled_voxel) {
    auto const &node = scene.node_infos[node_index];
    if (std::holds_alternative<magicavoxel::SceneTransformInfo>(node)) {
        auto const &t_node = std::get<magicavoxel::SceneTransformInfo>(node);
        for (uint32_t i = 0; i < depth; ++i)
            std::cout << "  ";
        std::cout << " - Transform " << t_node.transform.offset.x << ", " << t_node.transform.offset.y << ", " << t_node.transform.offset.z << "\n";
        sample_scene(scene, t_node.child_node_id, depth + 1, sample_pos, sampled_voxel);
    } else if (std::holds_alternative<magicavoxel::SceneGroupInfo>(node)) {
        auto const &g_node = std::get<magicavoxel::SceneGroupInfo>(node);
        for (uint32_t i = 0; i < depth; ++i)
            std::cout << "  ";
        std::cout << " - Group\n";
        for (uint32_t child_i = 0; child_i < g_node.num_child_nodes; ++child_i) {
            sample_scene(scene, scene.group_children_ids[g_node.first_child_node_id_index + child_i], depth + 1, sample_pos, sampled_voxel);
            if (sampled_voxel != 255)
                return;
        }
    } else if (std::holds_alternative<magicavoxel::SceneShapeInfo>(node)) {
        auto const &s_node = std::get<magicavoxel::SceneShapeInfo>(node);
        for (uint32_t i = 0; i < depth; ++i)
            std::cout << "  ";
        std::cout << " - Shape\n";
        auto const &model = scene.models[s_node.model_id];
    }
}

extern "C" void gvox_parse_adapter_magicavoxel_begin(GvoxAdapterContext *ctx, [[maybe_unused]] void *config) {
    auto *user_state_ptr = malloc(sizeof(MagicavoxelParseUserState));
    auto &user_state = *(new (user_state_ptr) MagicavoxelParseUserState());
    gvox_parse_adapter_set_user_pointer(ctx, user_state_ptr);
    auto read_var = [&](auto &var) {
        gvox_input_read(ctx, user_state.offset, sizeof(var), &var);
        user_state.offset += sizeof(var);
    };
    auto read_dict = [&](magicavoxel::Dictionary &temp_dict) {
        uint32_t num_pairs_to_read = 0;
        read_var(num_pairs_to_read);
        if (num_pairs_to_read > magicavoxel::MAX_DICT_PAIRS) {
            return false;
        }
        temp_dict.buffer_mem_used = 0;
        temp_dict.num_key_value_pairs = 0;
        for (uint32_t i = 0; i < num_pairs_to_read; i++) {
            uint32_t key_string_size = 0;
            read_var(key_string_size);
            if (temp_dict.buffer_mem_used + key_string_size > magicavoxel::MAX_DICT_SIZE) {
                return false;
            }
            char *key = &temp_dict.buffer[temp_dict.buffer_mem_used];
            temp_dict.buffer_mem_used += key_string_size + 1;
            gvox_input_read(ctx, user_state.offset, key_string_size, key);
            user_state.offset += key_string_size;
            key[key_string_size] = 0;
            uint32_t value_string_size = 0;
            read_var(value_string_size);
            if (temp_dict.buffer_mem_used + value_string_size > magicavoxel::MAX_DICT_SIZE) {
                return false;
            }
            char *value = &temp_dict.buffer[temp_dict.buffer_mem_used];
            temp_dict.buffer_mem_used += value_string_size + 1;
            gvox_input_read(ctx, user_state.offset, value_string_size, value);
            user_state.offset += value_string_size;
            value[value_string_size] = 0;
            temp_dict.keys[temp_dict.num_key_value_pairs] = key;
            temp_dict.values[temp_dict.num_key_value_pairs] = value;
            temp_dict.num_key_value_pairs++;
        }
        return true;
    };
    uint32_t file_header = 0;
    uint32_t file_version = 0;
    read_var(file_header);
    read_var(file_version);
    if (file_header != magicavoxel::CHUNK_ID_VOX_ || (file_version != 150 && file_version != 200)) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "either bad magicavoxel header, or unsupported magicavoxel version");
        return;
    }
    uint32_t main_chunk_id = 0;
    uint32_t main_chunk_size = 0;
    uint32_t main_chunk_child_size = 0;
    read_var(main_chunk_id);
    read_var(main_chunk_size);
    read_var(main_chunk_child_size);
    if (main_chunk_id != magicavoxel::CHUNK_ID_MAIN) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "the first magicavoxel chunk must be the main chunk");
        return;
    }
    auto last_byte_index = user_state.offset + main_chunk_child_size;
    auto temp_dict = magicavoxel::Dictionary{};
    user_state.scene.group_children_ids.push_back(std::numeric_limits<uint32_t>::max());
    while (user_state.offset <= last_byte_index - sizeof(uint32_t) * 3) {
        uint32_t chunk_id = 0;
        uint32_t chunk_size = 0;
        uint32_t chunk_child_size = 0;
        read_var(chunk_id);
        read_var(chunk_size);
        read_var(chunk_child_size);
        switch (chunk_id) {
        case magicavoxel::CHUNK_ID_SIZE: {
            if (chunk_size != 12 || chunk_child_size != 0) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected chunk size for SIZE chunk");
                return;
            }
            auto next_model = magicavoxel::Model{};
            read_var(next_model.range.extent.x);
            read_var(next_model.range.extent.y);
            read_var(next_model.range.extent.z);
            user_state.scene.models.push_back(next_model);
        } break;
        case magicavoxel::CHUNK_ID_XYZI: {
            if (user_state.scene.models.size() == 0 ||
                user_state.scene.models.back().range.extent.x == 0 ||
                user_state.scene.models.back().range.extent.y == 0 ||
                user_state.scene.models.back().range.extent.z == 0 ||
                user_state.scene.models.back().palette_ids.size() > 0) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "expected a SIZE chunk before XYZI chunk");
                return;
            }
            auto &model = user_state.scene.models.back();
            uint32_t num_voxels_in_chunk = 0;
            read_var(num_voxels_in_chunk);
            if (num_voxels_in_chunk == 0) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "why is there a model with size 0?");
                return;
            }
            auto packed_voxel_data = std::vector<uint8_t>(num_voxels_in_chunk * 4);
            gvox_input_read(ctx, user_state.offset, packed_voxel_data.size(), packed_voxel_data.data());
            user_state.offset += packed_voxel_data.size();
            uint8_t min_x = 255, min_y = 255, min_z = 255;
            uint8_t max_x = 0, max_y = 0, max_z = 0;
            for (uint32_t i = 0; i < num_voxels_in_chunk; i++) {
                uint8_t x = packed_voxel_data[i * 4 + 0];
                uint8_t y = packed_voxel_data[i * 4 + 1];
                uint8_t z = packed_voxel_data[i * 4 + 2];
                if (x >= model.range.extent.x && y >= model.range.extent.y && z >= model.range.extent.z) {
                    gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "invalid data in XYZI chunk");
                    return;
                }
                min_x = std::min(x, min_x), min_y = std::min(y, min_y), min_z = std::min(z, min_z);
                max_x = std::max(x, max_x), max_y = std::max(y, max_y), max_z = std::max(z, max_z);
            }
            model.range.extent.x = max_x - min_x + 1, model.range.extent.y = max_y - min_y + 1, model.range.extent.z = max_z - min_z + 1;
            model.range.offset.x = min_x, model.range.offset.y = min_y, model.range.offset.z = max_z;
            const uint32_t k_stride_x = 1;
            const uint32_t k_stride_y = model.range.extent.x;
            const uint32_t k_stride_z = model.range.extent.x * model.range.extent.y;
            model.palette_ids.resize(model.range.extent.x * model.range.extent.y * model.range.extent.z);
            std::fill(model.palette_ids.begin(), model.palette_ids.end(), uint8_t{255});
            for (uint32_t i = 0; i < num_voxels_in_chunk; i++) {
                uint8_t x = packed_voxel_data[i * 4 + 0] - min_x;
                uint8_t y = packed_voxel_data[i * 4 + 1] - min_y;
                uint8_t z = packed_voxel_data[i * 4 + 2] - min_z;
                uint8_t color_index = packed_voxel_data[i * 4 + 3];
                model.palette_ids[(x * k_stride_x) + (y * k_stride_y) + (z * k_stride_z)] = color_index;
            }
        } break;
        case magicavoxel::CHUNK_ID_RGBA: {
            if (chunk_size != sizeof(user_state.palette)) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected chunk size for RGBA chunk");
                return;
            }
            read_var(user_state.palette);
        } break;
        case magicavoxel::CHUNK_ID_nTRN: {
            uint32_t node_id = 0;
            read_var(node_id);
            if (!read_dict(temp_dict)) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read nTRN dictionary");
                return;
            }
            auto result_transform = magicavoxel::SceneTransformInfo{};
            auto name_str = temp_dict.get<char const *>("_name", "");
            std::copy(name_str, name_str + std::max<size_t>(strlen(name_str) + 1, 65), result_transform.name);
            result_transform.hidden = temp_dict.get<bool>("_hidden", false);
            result_transform.loop = temp_dict.get<bool>("_loop", false);
            uint32_t reserved_id = 0;
            read_var(result_transform.child_node_id);
            read_var(reserved_id);
            read_var(result_transform.layer_id);
            read_var(result_transform.num_keyframes);
            if (reserved_id != UINT32_MAX) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected values for reserved_id in nTRN chunk");
                return;
            }
            if (result_transform.num_keyframes == 0) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "must have at least 1 frame in nTRN chunk");
                return;
            }
            result_transform.keyframe_offset = user_state.transform_keyframes.size();
            user_state.transform_keyframes.resize(result_transform.keyframe_offset + result_transform.num_keyframes);
            for (uint32_t i = 0; i < result_transform.num_keyframes; i++) {
                if (!read_dict(temp_dict)) {
                    gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read nTRN keyframe dictionary");
                    return;
                }
                auto &trn = user_state.transform_keyframes[result_transform.keyframe_offset + i].transform;
                auto r_str = temp_dict.get<char const *>("_r", 0);
                if (r_str != nullptr)
                    trn.rotation = static_cast<int8_t>(atoi(r_str));
                auto t_str = temp_dict.get<char const *>("_t", 0);
                if (t_str != nullptr)
                    sscanf_s(t_str, "%i %i %i", &trn.offset.x, &trn.offset.y, &trn.offset.z);
                user_state.transform_keyframes[result_transform.keyframe_offset + i].frame_index = temp_dict.get<uint32_t>("_f", 0);
            }
            result_transform.transform = user_state.transform_keyframes[result_transform.keyframe_offset].transform;
            user_state.scene.node_infos.resize(std::max<size_t>(node_id + 1, user_state.scene.node_infos.size()));
            user_state.scene.node_infos[node_id] = result_transform;
        } break;
        case magicavoxel::CHUNK_ID_nGRP: {
            uint32_t node_id = 0;
            read_var(node_id);
            // has dictionary, we don't care
            read_dict(temp_dict);
            auto result_group = magicavoxel::SceneGroupInfo{};
            uint32_t num_child_nodes = 0;
            read_var(num_child_nodes);
            if (num_child_nodes) {
                size_t prior_size = user_state.scene.group_children_ids.size();
                user_state.scene.group_children_ids.resize(prior_size + num_child_nodes);
                gvox_input_read(ctx, user_state.offset, sizeof(uint32_t) * num_child_nodes, &user_state.scene.group_children_ids[prior_size]);
                user_state.offset += sizeof(uint32_t) * num_child_nodes;
                result_group.first_child_node_id_index = (uint32_t)prior_size;
                result_group.num_child_nodes = num_child_nodes;
            }
            user_state.scene.node_infos.resize(std::max<size_t>(node_id + 1, user_state.scene.node_infos.size()));
            user_state.scene.node_infos[node_id] = result_group;
        } break;
        case magicavoxel::CHUNK_ID_nSHP: {
            uint32_t node_id = 0;
            read_var(node_id);
            read_dict(temp_dict);
            auto result_shape = magicavoxel::SceneShapeInfo{};
            result_shape.loop = temp_dict.get<bool>("_loop", false);
            read_var(result_shape.num_keyframes);
            if (result_shape.num_keyframes == 0) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "must have at least 1 frame in nSHP chunk");
                return;
            }
            result_shape.keyframe_offset = user_state.shape_keyframes.size();
            user_state.shape_keyframes.resize(result_shape.keyframe_offset + result_shape.num_keyframes);
            for (uint32_t i = 0; i < result_shape.num_keyframes; i++) {
                read_var(user_state.shape_keyframes[result_shape.keyframe_offset + i].model_index);
                if (!read_dict(temp_dict)) {
                    gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read nSHP keyframe dictionary");
                    return;
                }
                user_state.shape_keyframes[result_shape.keyframe_offset + i].frame_index = temp_dict.get<uint32_t>("_f", 0);
            }
            user_state.scene.node_infos.resize(std::max<size_t>(node_id + 1, user_state.scene.node_infos.size()));
            user_state.scene.node_infos[node_id] = result_shape;
        } break;
        case magicavoxel::CHUNK_ID_IMAP: {
            if (chunk_size != 256) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected chunk size for IMAP chunk");
                return;
            }
            read_var(user_state.index_map);
            user_state.found_index_map_chunk = true;
        } break;
        case magicavoxel::CHUNK_ID_LAYR: {
            int32_t layer_id = 0;
            int32_t reserved_id = 0;
            read_var(layer_id);
            if (!read_dict(temp_dict)) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read dictionary in LAYR chunk");
                return;
            }
            read_var(reserved_id);
            if (reserved_id != -1) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected value for reserved_id in LAYR chunk");
                return;
            }
            auto result_layer = magicavoxel::Layer{
                .name = temp_dict.get<char const *>("_name", ""),
                .color = {255, 255, 255, 255},
                .hidden = temp_dict.get<bool>("_hidden", false),
            };
            char const *color_string = temp_dict.get<char const *>("_color", nullptr);
            if (color_string != nullptr) {
                uint32_t r, g, b;
                sscanf_s(color_string, "%u %u %u", &r, &g, &b);
                result_layer.color.r = static_cast<uint8_t>(r);
                result_layer.color.g = static_cast<uint8_t>(g);
                result_layer.color.b = static_cast<uint8_t>(b);
            }
            user_state.layers.resize(std::max<size_t>(layer_id + 1, user_state.layers.size()));
            user_state.layers[layer_id] = result_layer;
        } break;
        case magicavoxel::CHUNK_ID_MATL: {
            int32_t material_id = 0;
            read_var(material_id);
            material_id = material_id & 0xFF; // incoming material 256 is material 0
            if (!read_dict(temp_dict)) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read dictionary in MATL chunk");
                return;
            }
            char const *type_string = temp_dict.get<char const *>("_type", nullptr);
            if (type_string) {
                if (0 == strcmp(type_string, "_diffuse")) {
                    user_state.materials[material_id].type = magicavoxel::MaterialType::DIFFUSE;
                } else if (0 == strcmp(type_string, "_metal")) {
                    user_state.materials[material_id].type = magicavoxel::MaterialType::METAL;
                } else if (0 == strcmp(type_string, "_glass")) {
                    user_state.materials[material_id].type = magicavoxel::MaterialType::GLASS;
                } else if (0 == strcmp(type_string, "_emit")) {
                    user_state.materials[material_id].type = magicavoxel::MaterialType::EMIT;
                } else if (0 == strcmp(type_string, "_blend")) {
                    user_state.materials[material_id].type = magicavoxel::MaterialType::BLEND;
                } else if (0 == strcmp(type_string, "_media")) {
                    user_state.materials[material_id].type = magicavoxel::MaterialType::MEDIA;
                }
            }
            constexpr auto material_property_ids = std::array{
                std::pair<char const *, uint32_t>{"_metal", magicavoxel::MATERIAL_METAL_BIT},
                std::pair<char const *, uint32_t>{"_rough", magicavoxel::MATERIAL_ROUGH_BIT},
                std::pair<char const *, uint32_t>{"_spec", magicavoxel::MATERIAL_SPEC_BIT},
                std::pair<char const *, uint32_t>{"_ior", magicavoxel::MATERIAL_IOR_BIT},
                std::pair<char const *, uint32_t>{"_att", magicavoxel::MATERIAL_ATT_BIT},
                std::pair<char const *, uint32_t>{"_flux", magicavoxel::MATERIAL_FLUX_BIT},
                std::pair<char const *, uint32_t>{"_emit", magicavoxel::MATERIAL_EMIT_BIT},
                std::pair<char const *, uint32_t>{"_ldr", magicavoxel::MATERIAL_LDR_BIT},
                std::pair<char const *, uint32_t>{"_trans", magicavoxel::MATERIAL_TRANS_BIT},
                std::pair<char const *, uint32_t>{"_alpha", magicavoxel::MATERIAL_ALPHA_BIT},
                std::pair<char const *, uint32_t>{"_d", magicavoxel::MATERIAL_D_BIT},
                std::pair<char const *, uint32_t>{"_sp", magicavoxel::MATERIAL_SP_BIT},
                std::pair<char const *, uint32_t>{"_g", magicavoxel::MATERIAL_G_BIT},
                std::pair<char const *, uint32_t>{"_media", magicavoxel::MATERIAL_MEDIA_BIT},
            };
            size_t field_offset = 0;
            for (auto const &[mat_str, mat_bit] : material_property_ids) {
                char const *prop_str = temp_dict.get<char const *>(mat_str, NULL);
                if (prop_str) {
                    user_state.materials[material_id].content_flags |= mat_bit;
                    *(&user_state.materials[material_id].metal + field_offset) = static_cast<float>(atof(prop_str));
                }
                ++field_offset;
            }
        } break;
        case magicavoxel::CHUNK_ID_MATT: {
            if (chunk_size < 16u) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected chunk size for MATT chunk");
                return;
            }
            int32_t material_id = 0;
            read_var(material_id);
            material_id = material_id & 0xFF;
            int32_t material_type = 0;
            read_var(material_type);
            float material_weight = 0.0f;
            read_var(material_weight);
            uint32_t property_bits = 0u;
            read_var(property_bits);
            switch (material_type) {
            case 0:
                user_state.materials[material_id].type = magicavoxel::MaterialType::DIFFUSE;
                break;
            case 1:
                user_state.materials[material_id].type = magicavoxel::MaterialType::METAL;
                user_state.materials[material_id].content_flags |= magicavoxel::MATERIAL_METAL_BIT;
                user_state.materials[material_id].metal = material_weight;
                break;
            case 2:
                user_state.materials[material_id].type = magicavoxel::MaterialType::GLASS;
                user_state.materials[material_id].content_flags |= magicavoxel::MATERIAL_TRANS_BIT;
                user_state.materials[material_id].trans = material_weight;
                break;
            case 3:
                user_state.materials[material_id].type = magicavoxel::MaterialType::EMIT;
                user_state.materials[material_id].content_flags |= magicavoxel::MATERIAL_EMIT_BIT;
                user_state.materials[material_id].emit = material_weight;
                break;
            }
            user_state.offset += chunk_size - 16u;
        } break;
        default: {
            user_state.offset += chunk_size;
        } break;
        }
    }
    if (user_state.scene.node_infos.size() != 0) {
        uint32_t x = 255u;
        sample_scene(user_state.scene, 0, 0, {0, 0, 0}, x);
    }
}

extern "C" void gvox_parse_adapter_magicavoxel_end([[maybe_unused]] GvoxAdapterContext *ctx) {
    auto &user_state = *reinterpret_cast<MagicavoxelParseUserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    user_state.~MagicavoxelParseUserState();
    free(&user_state);
}

extern "C" auto gvox_parse_adapter_magicavoxel_query_region_flags([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxRegionRange const *range, [[maybe_unused]] uint32_t channel_id) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_magicavoxel_load_region(GvoxAdapterContext *ctx, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxRegion {
    auto &user_state = *reinterpret_cast<MagicavoxelParseUserState *>(gvox_parse_adapter_get_user_pointer(ctx));
    uint32_t voxel_data = 0;
    auto palette_id = 255u;
    {
        auto const &mdl = user_state.scene.models[0];
        auto const index = offset->x + offset->y * mdl.range.extent.x + offset->z * mdl.range.extent.x * mdl.range.extent.y;
        palette_id = mdl.palette_ids[index];
    }
    switch (channel_id) {
    case GVOX_CHANNEL_ID_COLOR:
        if (palette_id < 255) {
            auto const palette_val = user_state.palette[palette_id];
            voxel_data = std::bit_cast<uint32_t>(palette_val);
        } else {
            voxel_data = 0;
        }
        break;
    case GVOX_CHANNEL_ID_MATERIAL_ID:
        voxel_data = palette_id;
        break;
    case GVOX_CHANNEL_ID_ROUGHNESS:
        if (palette_id < 255 && user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_ROUGH_BIT) {
            voxel_data = std::bit_cast<uint32_t>(user_state.materials[palette_id].rough);
        } else {
            voxel_data = 0;
        }
        break;
    case GVOX_CHANNEL_ID_METALNESS:
        if (palette_id < 255 && user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_METAL_BIT) {
            voxel_data = std::bit_cast<uint32_t>(user_state.materials[palette_id].metal);
        } else {
            voxel_data = 0;
        }
        break;
    case GVOX_CHANNEL_ID_TRANSPARENCY:
        if (palette_id < 255 && user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_ALPHA_BIT) {
            voxel_data = std::bit_cast<uint32_t>(user_state.materials[palette_id].alpha);
        } else {
            voxel_data = 0;
        }
        break;
    case GVOX_CHANNEL_ID_IOR:
        if (palette_id < 255 && user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_IOR_BIT) {
            voxel_data = std::bit_cast<uint32_t>(user_state.materials[palette_id].ior);
        } else {
            voxel_data = 0;
        }
        break;
    case GVOX_CHANNEL_ID_EMISSIVE_COLOR:
        if (palette_id < 255) {
            auto const palette_val = user_state.palette[palette_id];
            auto is_emissive = (user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_EMIT_BIT) != 0;
            voxel_data = std::bit_cast<uint32_t>(palette_val) * static_cast<uint32_t>(is_emissive);
        } else {
            voxel_data = 0;
        }
        break;
    case GVOX_CHANNEL_ID_EMISSIVE_POWER:
        if (palette_id < 255 && user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_EMIT_BIT) {
            voxel_data = std::bit_cast<uint32_t>(user_state.materials[palette_id].emit);
        } else {
            voxel_data = 0;
        }
        break;
    default:
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Requested unsupported channel from magicavoxel file");
        break;
    }
    GvoxRegion const region = {
        .range = {
            .offset = *offset,
            .extent = {1, 1, 1},
        },
        .channels = channel_id,
        .flags = GVOX_REGION_FLAG_UNIFORM,
        .data = reinterpret_cast<void *>(static_cast<size_t>(voxel_data)),
    };
    return region;
}

extern "C" void gvox_parse_adapter_magicavoxel_unload_region([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] GvoxRegion *region) {
}

extern "C" auto gvox_parse_adapter_magicavoxel_sample_region([[maybe_unused]] GvoxAdapterContext *ctx, GvoxRegion const *region, [[maybe_unused]] GvoxOffset3D const *offset, uint32_t /*channel_id*/) -> uint32_t {
    return static_cast<uint32_t>(reinterpret_cast<size_t>(region->data));
}

#define OGT_VOX_IMPLEMENTATION
#include "../shared/ogt_vox.hpp"
