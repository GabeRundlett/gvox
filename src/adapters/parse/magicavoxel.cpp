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
#include <memory>
#include <sstream>
#include <limits>

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
        auto result = std::array<int32_t, 3>{};
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
        auto result = std::array<uint32_t, 3>{};
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
        auto result = std::array<uint32_t, 3>{};
        uint32_t constexpr row2_index[] = {2, UINT32_MAX, 1, 0, UINT32_MAX};
        uint32_t const row0_vec_index = (packed_rotation_bits >> 0) & 3;
        uint32_t const row1_vec_index = (packed_rotation_bits >> 2) & 3;
        uint32_t const row2_vec_index = row2_index[((1 << row0_vec_index) | (1 << row1_vec_index)) - 3];
        result[row0_vec_index] = p.x;
        result[row1_vec_index] = p.y;
        result[row2_vec_index] = p.z;
        if ((packed_rotation_bits & (1 << 4)) != 0) {
            result[0] = extent.x - 1 - result[0];
        }
        if ((packed_rotation_bits & (1 << 5)) != 0) {
            result[1] = extent.y - 1 - result[1];
        }
        if ((packed_rotation_bits & (1 << 6)) != 0) {
            result[2] = extent.z - 1 - result[2];
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
        char name[65]{};
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

    struct SceneGroup;
    struct SceneModel;
    using SceneNode = std::variant<SceneGroup, SceneModel>;

    struct SceneGroup {
        Transform transform;
        GvoxRegionRange range{};
        std::vector<std::unique_ptr<SceneNode>> nodes;
    };

    struct SceneModel {
        int8_t rotation{1 << 2};
        GvoxRegionRange range;
        uint32_t index;
    };

    struct Scene {
        std::vector<Model> models{};
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

void construct_scene(magicavoxel::Scene &scene, magicavoxel::SceneInfo &scene_info, magicavoxel::SceneNode &current_node, uint32_t node_index, uint32_t depth, magicavoxel::Transform trn) {
    auto const &node_info = scene_info.node_infos[node_index];
    if (std::holds_alternative<magicavoxel::SceneTransformInfo>(node_info)) {
        auto const &t_node_info = std::get<magicavoxel::SceneTransformInfo>(node_info);
        auto new_trn = trn;
        auto rotated_offset = magicavoxel::rotate(trn.rotation, t_node_info.transform.offset);
        new_trn.offset.x += rotated_offset.x;
        new_trn.offset.y += rotated_offset.y;
        new_trn.offset.z += rotated_offset.z;
        new_trn.rotation = magicavoxel::rotate(new_trn.rotation, t_node_info.transform.rotation);
        construct_scene(scene, scene_info, current_node, t_node_info.child_node_id, depth + 1, new_trn);
    } else if (std::holds_alternative<magicavoxel::SceneGroupInfo>(node_info)) {
        auto const &g_node_info = std::get<magicavoxel::SceneGroupInfo>(node_info);
        auto &g_current_node = std::get<magicavoxel::SceneGroup>(current_node = magicavoxel::SceneGroup{});
        g_current_node.transform = trn;
        g_current_node.nodes.resize(g_node_info.num_child_nodes);
        for (auto &node : g_current_node.nodes) {
            node = std::make_unique<magicavoxel::SceneNode>();
        }
        for (uint32_t child_i = 0; child_i < g_node_info.num_child_nodes; ++child_i) {
            construct_scene(scene, scene_info, *g_current_node.nodes[child_i], scene_info.group_children_ids[g_node_info.first_child_node_id_index + child_i], depth + 1, trn);
        }
    } else if (std::holds_alternative<magicavoxel::SceneShapeInfo>(node_info)) {
        auto const &s_node_info = std::get<magicavoxel::SceneShapeInfo>(node_info);
        auto &s_current_node = std::get<magicavoxel::SceneModel>(current_node = magicavoxel::SceneModel{});
        s_current_node.rotation = trn.rotation;
        s_current_node.index = s_node_info.model_id;
        auto &model = scene.models[s_current_node.index];
        s_current_node.range.extent = magicavoxel::rotate(trn.rotation, model.extent);
        s_current_node.range.offset.x = trn.offset.x - static_cast<int32_t>(s_current_node.range.extent.x + ((trn.rotation >> 4) & 1)) / 2;
        s_current_node.range.offset.y = trn.offset.y - static_cast<int32_t>(s_current_node.range.extent.y + ((trn.rotation >> 5) & 1)) / 2;
        s_current_node.range.offset.z = trn.offset.z - static_cast<int32_t>(s_current_node.range.extent.z + ((trn.rotation >> 6) & 1)) / 2;
    }
}

void sample_scene(magicavoxel::Scene &scene, magicavoxel::SceneNode &current_node, GvoxOffset3D const &sample_pos, uint32_t &sampled_voxel) {
    if (std::holds_alternative<magicavoxel::SceneGroup>(current_node)) {
        auto &g_current_node = std::get<magicavoxel::SceneGroup>(current_node);
        for (auto &node : g_current_node.nodes) {
            sample_scene(scene, *node, sample_pos, sampled_voxel);
            if (sampled_voxel != 255) {
                break;
            }
        }
    } else if (std::holds_alternative<magicavoxel::SceneModel>(current_node)) {
        auto &s_current_node = std::get<magicavoxel::SceneModel>(current_node);
        auto &model = scene.models[s_current_node.index];
        auto offset = s_current_node.range.offset;
        auto extent = s_current_node.range.extent;
        if (sample_pos.x < offset.x ||
            sample_pos.y < offset.y ||
            sample_pos.z < offset.z ||
            sample_pos.x >= offset.x + static_cast<int32_t>(extent.x) ||
            sample_pos.y >= offset.y + static_cast<int32_t>(extent.y) ||
            sample_pos.z >= offset.z + static_cast<int32_t>(extent.z)) {
            return;
        }
        auto rel_p = GvoxExtent3D{
            static_cast<uint32_t>(sample_pos.x - offset.x),
            static_cast<uint32_t>(sample_pos.y - offset.y),
            static_cast<uint32_t>(sample_pos.z - offset.z),
        };
        rel_p = magicavoxel::rotate(magicavoxel::inverse(s_current_node.rotation), rel_p, model.extent);
        auto const index = rel_p.x + rel_p.y * model.extent.x + rel_p.z * model.extent.x * model.extent.y;
        sampled_voxel = model.palette_ids[index];
    }
}

extern "C" void gvox_parse_adapter_magicavoxel_create(GvoxAdapterContext *ctx, void * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(MagicavoxelParseUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) MagicavoxelParseUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_magicavoxel_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<MagicavoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~MagicavoxelParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_magicavoxel_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<MagicavoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto read_var = [&](auto &var) {
        gvox_input_read(blit_ctx, user_state.offset, sizeof(var), &var);
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
            gvox_input_read(blit_ctx, user_state.offset, key_string_size, key);
            user_state.offset += key_string_size;
            key[key_string_size] = 0;
            uint32_t value_string_size = 0;
            read_var(value_string_size);
            if (temp_dict.buffer_mem_used + value_string_size > magicavoxel::MAX_DICT_SIZE) {
                return false;
            }
            char *value = &temp_dict.buffer[temp_dict.buffer_mem_used];
            temp_dict.buffer_mem_used += value_string_size + 1;
            gvox_input_read(blit_ctx, user_state.offset, value_string_size, value);
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
    auto temp_scene_info = magicavoxel::SceneInfo{};
    temp_scene_info.group_children_ids.push_back(std::numeric_limits<uint32_t>::max());
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
            read_var(next_model.extent.x);
            read_var(next_model.extent.y);
            read_var(next_model.extent.z);
            user_state.scene.models.push_back(next_model);
        } break;
        case magicavoxel::CHUNK_ID_XYZI: {
            if (user_state.scene.models.empty() ||
                user_state.scene.models.back().extent.x == 0 ||
                user_state.scene.models.back().extent.y == 0 ||
                user_state.scene.models.back().extent.z == 0 ||
                !user_state.scene.models.back().palette_ids.empty()) {
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
            gvox_input_read(blit_ctx, user_state.offset, packed_voxel_data.size(), packed_voxel_data.data());
            user_state.offset += packed_voxel_data.size();
            const uint32_t k_stride_x = 1;
            const uint32_t k_stride_y = model.extent.x;
            const uint32_t k_stride_z = model.extent.x * model.extent.y;
            model.palette_ids.resize(model.extent.x * model.extent.y * model.extent.z);
            std::fill(model.palette_ids.begin(), model.palette_ids.end(), uint8_t{255});
            for (uint32_t i = 0; i < num_voxels_in_chunk; i++) {
                uint8_t const x = packed_voxel_data[i * 4 + 0];
                uint8_t const y = packed_voxel_data[i * 4 + 1];
                uint8_t const z = packed_voxel_data[i * 4 + 2];
                if (x >= model.extent.x && y >= model.extent.y && z >= model.extent.z) {
                    gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "invalid data in XYZI chunk");
                    return;
                }
                uint8_t const color_index = packed_voxel_data[i * 4 + 3];
                model.palette_ids[(x * k_stride_x) + (y * k_stride_y) + (z * k_stride_z)] = color_index - 1;
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
            const auto *name_str = temp_dict.get<char const *>("_name", "");
            std::copy(name_str, name_str + std::min<size_t>(strlen(name_str) + 1, 65), result_transform.name);
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
                const auto *r_str = temp_dict.get<char const *>("_r", nullptr);
                if (r_str != nullptr) {
                    auto ss = std::stringstream{};
                    ss.str(r_str);
                    int32_t temp_int = 0;
                    ss >> temp_int;
                    trn.rotation = static_cast<int8_t>(temp_int);
                }
                const auto *t_str = temp_dict.get<char const *>("_t", nullptr);
                if (t_str != nullptr) {
                    auto ss = std::stringstream{};
                    ss.str(t_str);
                    ss >> trn.offset.x >> trn.offset.y >> trn.offset.z;
                }
                user_state.transform_keyframes[result_transform.keyframe_offset + i].frame_index = temp_dict.get<uint32_t>("_f", 0);
            }
            result_transform.transform = user_state.transform_keyframes[result_transform.keyframe_offset].transform;
            temp_scene_info.node_infos.resize(std::max<size_t>(node_id + 1, temp_scene_info.node_infos.size()));
            temp_scene_info.node_infos[node_id] = result_transform;
        } break;
        case magicavoxel::CHUNK_ID_nGRP: {
            uint32_t node_id = 0;
            read_var(node_id);
            // has dictionary, we don't care
            read_dict(temp_dict);
            auto result_group = magicavoxel::SceneGroupInfo{};
            uint32_t num_child_nodes = 0;
            read_var(num_child_nodes);
            if (num_child_nodes != 0u) {
                size_t const prior_size = temp_scene_info.group_children_ids.size();
                temp_scene_info.group_children_ids.resize(prior_size + num_child_nodes);
                gvox_input_read(blit_ctx, user_state.offset, sizeof(uint32_t) * num_child_nodes, &temp_scene_info.group_children_ids[prior_size]);
                user_state.offset += sizeof(uint32_t) * num_child_nodes;
                result_group.first_child_node_id_index = (uint32_t)prior_size;
                result_group.num_child_nodes = num_child_nodes;
            }
            temp_scene_info.node_infos.resize(std::max<size_t>(node_id + 1, temp_scene_info.node_infos.size()));
            temp_scene_info.node_infos[node_id] = result_group;
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
            result_shape.model_id = user_state.shape_keyframes[result_shape.keyframe_offset].model_index;
            temp_scene_info.node_infos.resize(std::max<size_t>(node_id + 1, temp_scene_info.node_infos.size()));
            temp_scene_info.node_infos[node_id] = result_shape;
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
                uint32_t r = 0;
                uint32_t g = 0;
                uint32_t b = 0;
                auto ss = std::stringstream{};
                ss.str(color_string);
                ss >> r >> g >> b;
                result_layer.color.r = static_cast<uint8_t>(r);
                result_layer.color.g = static_cast<uint8_t>(g);
                result_layer.color.b = static_cast<uint8_t>(b);
            }
            user_state.layers.resize(std::max<size_t>(static_cast<size_t>(layer_id + 1), user_state.layers.size()));
            user_state.layers[static_cast<size_t>(layer_id)] = result_layer;
        } break;
        case magicavoxel::CHUNK_ID_MATL: {
            int32_t material_id = 0;
            read_var(material_id);
            material_id = (material_id - 1) & 0xFF; // incoming material 256 is material 0
            if (!read_dict(temp_dict)) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read dictionary in MATL chunk");
                return;
            }
            char const *type_string = temp_dict.get<char const *>("_type", nullptr);
            if (type_string != nullptr) {
                if (0 == strcmp(type_string, "_diffuse")) {
                    user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::DIFFUSE;
                } else if (0 == strcmp(type_string, "_metal")) {
                    user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::METAL;
                } else if (0 == strcmp(type_string, "_glass")) {
                    user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::GLASS;
                } else if (0 == strcmp(type_string, "_emit")) {
                    user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::EMIT;
                } else if (0 == strcmp(type_string, "_blend")) {
                    user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::BLEND;
                } else if (0 == strcmp(type_string, "_media")) {
                    user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::MEDIA;
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
                if (prop_str != nullptr) {
                    user_state.materials[static_cast<size_t>(material_id)].content_flags |= mat_bit;
                    *(&user_state.materials[static_cast<size_t>(material_id)].metal + field_offset) = static_cast<float>(atof(prop_str));
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
                user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::DIFFUSE;
                break;
            case 1:
                user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::METAL;
                user_state.materials[static_cast<size_t>(material_id)].content_flags |= magicavoxel::MATERIAL_METAL_BIT;
                user_state.materials[static_cast<size_t>(material_id)].metal = material_weight;
                break;
            case 2:
                user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::GLASS;
                user_state.materials[static_cast<size_t>(material_id)].content_flags |= magicavoxel::MATERIAL_TRANS_BIT;
                user_state.materials[static_cast<size_t>(material_id)].trans = material_weight;
                break;
            case 3:
                user_state.materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::EMIT;
                user_state.materials[static_cast<size_t>(material_id)].content_flags |= magicavoxel::MATERIAL_EMIT_BIT;
                user_state.materials[static_cast<size_t>(material_id)].emit = material_weight;
                break;
            }
            user_state.offset += chunk_size - 16u;
        } break;
        default: {
            user_state.offset += chunk_size;
        } break;
        }
    }
    if (!temp_scene_info.node_infos.empty()) {
        construct_scene(user_state.scene, temp_scene_info, user_state.scene.root_node, 0, 0, {});
    }
}

extern "C" void gvox_parse_adapter_magicavoxel_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

extern "C" auto gvox_parse_adapter_magicavoxel_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_magicavoxel_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    auto const available_channels =
        uint32_t{GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_MATERIAL_ID | GVOX_CHANNEL_BIT_ROUGHNESS |
                 GVOX_CHANNEL_BIT_METALNESS | GVOX_CHANNEL_BIT_TRANSPARENCY | GVOX_CHANNEL_BIT_IOR |
                 GVOX_CHANNEL_BIT_EMISSIVE_COLOR | GVOX_CHANNEL_BIT_EMISSIVE_POWER};
    if ((channel_flags & ~available_channels) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    GvoxRegion const region = {
        .range = *range,
        .channels = channel_flags & available_channels,
        .flags = 0u,
        .data = nullptr,
    };
    return region;
}

extern "C" void gvox_parse_adapter_magicavoxel_unload_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegion * /*unused*/) {
}

extern "C" auto gvox_parse_adapter_magicavoxel_sample_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegion const * /*unused*/, GvoxOffset3D const *offset, uint32_t channel_id) -> uint32_t {
    auto &user_state = *static_cast<MagicavoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    uint32_t voxel_data = 0;
    auto palette_id = 255u;
    sample_scene(user_state.scene, user_state.scene.root_node, *offset, palette_id);
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
        if (palette_id < 255 && ((user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_ROUGH_BIT) != 0u)) {
            voxel_data = std::bit_cast<uint32_t>(user_state.materials[palette_id].rough);
        } else {
            voxel_data = 0;
        }
        break;
    case GVOX_CHANNEL_ID_METALNESS:
        if (palette_id < 255 && ((user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_METAL_BIT) != 0u)) {
            voxel_data = std::bit_cast<uint32_t>(user_state.materials[palette_id].metal);
        } else {
            voxel_data = 0;
        }
        break;
    case GVOX_CHANNEL_ID_TRANSPARENCY:
        if (palette_id < 255 && ((user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_ALPHA_BIT) != 0u)) {
            voxel_data = std::bit_cast<uint32_t>(user_state.materials[palette_id].alpha);
        } else {
            voxel_data = 0;
        }
        break;
    case GVOX_CHANNEL_ID_IOR:
        if (palette_id < 255 && ((user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_IOR_BIT) != 0u)) {
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
        if (palette_id < 255 && ((user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_EMIT_BIT) != 0u)) {
            voxel_data = std::bit_cast<uint32_t>(user_state.materials[palette_id].emit);
        } else {
            voxel_data = 0;
        }
        break;
    default:
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Requested unsupported channel from magicavoxel file");
        break;
    }

    return voxel_data;
}
