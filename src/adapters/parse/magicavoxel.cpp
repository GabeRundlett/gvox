#include <gvox/gvox.h>
#include <gvox/adapters/parse/magicavoxel.h>

#include "../shared/magicavoxel.hpp"

#include <algorithm>
#include <new>
#include <memory>
#include <limits>
#include <numeric>

#include "../shared/thread_pool.hpp"
using namespace gvox_detail::thread_pool;
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
using namespace std::chrono_literals;
#endif

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
    ThreadPool thread_pool{};
};

void initialize_model(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, magicavoxel::Model const &model) {
#if __wasm32__
    auto packed_voxel_data = std::vector<uint8_t>();
#else
    thread_local auto packed_voxel_data = std::vector<uint8_t>();
#endif
    packed_voxel_data.resize(static_cast<size_t>(model.num_voxels_in_chunk) * 4);
    gvox_input_read(blit_ctx, model.input_offset, packed_voxel_data.size(), packed_voxel_data.data());
    const uint32_t k_stride_x = 1;
    const uint32_t k_stride_y = model.extent.x;
    const uint32_t k_stride_z = model.extent.x * model.extent.y;
    model.palette_ids.resize(static_cast<size_t>(model.extent.x) * model.extent.y * model.extent.z);
    std::fill(model.palette_ids.begin(), model.palette_ids.end(), uint8_t{255});
    for (uint32_t i = 0; i < model.num_voxels_in_chunk; i++) {
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
}

void ensure_initialized_model(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, magicavoxel::Model const &model) {
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
    auto lock = std::lock_guard{model.mtx};
#endif
    if (model.palette_ids.empty()) {
        initialize_model(blit_ctx, ctx, model);
    }
}

void construct_scene(magicavoxel::Scene &scene, magicavoxel::SceneInfo &scene_info, uint32_t node_index, uint32_t depth, magicavoxel::Transform trn, GvoxOffset3D &min_p, GvoxOffset3D &max_p) {
    auto const &node_info = scene_info.node_infos[node_index];
    if (std::holds_alternative<magicavoxel::SceneTransformInfo>(node_info)) {
        auto const &t_node_info = std::get<magicavoxel::SceneTransformInfo>(node_info);
        auto new_trn = trn;
        auto rotated_offset = magicavoxel::rotate(trn.rotation, t_node_info.transform.offset);
        new_trn.offset.x += rotated_offset.x;
        new_trn.offset.y += rotated_offset.y;
        new_trn.offset.z += rotated_offset.z;
        new_trn.rotation = magicavoxel::rotate(new_trn.rotation, t_node_info.transform.rotation);
        construct_scene(scene, scene_info, t_node_info.child_node_id, depth + 1, new_trn, min_p, max_p);
    } else if (std::holds_alternative<magicavoxel::SceneGroupInfo>(node_info)) {
        auto const &g_node_info = std::get<magicavoxel::SceneGroupInfo>(node_info);
        scene.model_instances.reserve(scene.model_instances.size() + g_node_info.num_child_nodes);
        for (uint32_t child_i = 0; child_i < g_node_info.num_child_nodes; ++child_i) {
            construct_scene(
                scene, scene_info,
                scene_info.group_children_ids[g_node_info.first_child_node_id_index + child_i],
                depth + 1, trn, min_p, max_p);
        }
    } else if (std::holds_alternative<magicavoxel::SceneShapeInfo>(node_info)) {
        auto const &s_node_info = std::get<magicavoxel::SceneShapeInfo>(node_info);
        scene.model_instances.push_back({});
        auto &s_current_node = scene.model_instances.back();
        s_current_node.rotation = trn.rotation;
        s_current_node.index = s_node_info.model_id;
        auto &model = scene.models[s_current_node.index];
        auto extent = magicavoxel::rotate(
            magicavoxel::inverse(trn.rotation),
            GvoxExtent3D{
                model.extent.x,
                model.extent.y,
                model.extent.z,
            });
        auto extent_offset =
            GvoxExtent3D{
                static_cast<uint32_t>((trn.rotation >> 4) & 1),
                static_cast<uint32_t>((trn.rotation >> 5) & 1),
                static_cast<uint32_t>((trn.rotation >> 6) & 1),
            };
        s_current_node.aabb_min = {
            trn.offset.x - static_cast<int32_t>(extent.x + extent_offset.x) / 2,
            trn.offset.y - static_cast<int32_t>(extent.y + extent_offset.y) / 2,
            trn.offset.z - static_cast<int32_t>(extent.z + extent_offset.z) / 2,
        };
        s_current_node.aabb_max = {
            s_current_node.aabb_min.x + static_cast<int32_t>(extent.x),
            s_current_node.aabb_min.y + static_cast<int32_t>(extent.y),
            s_current_node.aabb_min.z + static_cast<int32_t>(extent.z),
        };
        min_p.x = std::min(min_p.x, s_current_node.aabb_min.x);
        min_p.y = std::min(min_p.y, s_current_node.aabb_min.y);
        min_p.z = std::min(min_p.z, s_current_node.aabb_min.z);
        max_p.x = std::max(max_p.x, s_current_node.aabb_max.x);
        max_p.y = std::max(max_p.y, s_current_node.aabb_max.y);
        max_p.z = std::max(max_p.z, s_current_node.aabb_max.z);
    }
}

void calc_bvh_node_range(std::vector<magicavoxel::ModelInstance> const &model_instances, magicavoxel::BvhNode &node) {
    node.aabb_min = {
        std::numeric_limits<int32_t>::max(),
        std::numeric_limits<int32_t>::max(),
        std::numeric_limits<int32_t>::max(),
    };
    node.aabb_max = {
        std::numeric_limits<int32_t>::min(),
        std::numeric_limits<int32_t>::min(),
        std::numeric_limits<int32_t>::min(),
    };
    if (auto *node_data = std::get_if<magicavoxel::BvhNode::Range>(&node.data)) {
        for (uint32_t node_i = 0; node_i < node_data->count; node_i++) {
            auto const &s_current_node = model_instances[node_data->first + node_i];
            node.aabb_min.x = std::min(node.aabb_min.x, s_current_node.aabb_min.x);
            node.aabb_min.y = std::min(node.aabb_min.y, s_current_node.aabb_min.y);
            node.aabb_min.z = std::min(node.aabb_min.z, s_current_node.aabb_min.z);
            node.aabb_max.x = std::max(node.aabb_max.x, s_current_node.aabb_max.x);
            node.aabb_max.y = std::max(node.aabb_max.y, s_current_node.aabb_max.y);
            node.aabb_max.z = std::max(node.aabb_max.z, s_current_node.aabb_max.z);
        }
    }
}

void subdivide_scene_bvh(std::vector<magicavoxel::ModelInstance> &model_instances, std::vector<magicavoxel::BvhNode> &bvh_nodes, magicavoxel::BvhNode &node) {
    if (!node.needs_subdivide()) {
        return;
    }
    auto const &node_range = std::get<magicavoxel::BvhNode::Range>(node.data);
    GvoxOffset3D const extent = {
        node.aabb_max.x - node.aabb_min.x,
        node.aabb_max.y - node.aabb_min.y,
        node.aabb_max.z - node.aabb_min.z,
    };
    int32_t axis = 0;
    int32_t split_p = 0;
    if (extent.y > extent.x) {
        if (extent.z > extent.y) {
            axis = 2;
        } else {
            axis = 1;
        }
    } else {
        if (extent.z > extent.x) {
            axis = 2;
        } else {
            axis = 0;
        }
    }
    switch (axis) {
    default:
    case 0: split_p = node.aabb_min.x + extent.x / 2; break;
    case 1: split_p = node.aabb_min.y + extent.y / 2; break;
    case 2: split_p = node.aabb_min.z + extent.z / 2; break;
    }
    using IterDiff = std::vector<magicavoxel::ModelInstance>::difference_type;
    auto first_iter = model_instances.begin() + static_cast<IterDiff>(node_range.first);
    auto split_iter = std::partition(
        first_iter,
        first_iter + static_cast<IterDiff>(node_range.count),
        [axis, split_p](auto const &i) -> bool {
            switch (axis) {
            default:
            case 0: return std::midpoint(i.aabb_min.x, i.aabb_max.x) < split_p;
            case 1: return std::midpoint(i.aabb_min.y, i.aabb_max.y) < split_p;
            case 2: return std::midpoint(i.aabb_min.z, i.aabb_max.z) < split_p;
            }
        });
    auto a_count = static_cast<uint32_t>(std::distance(first_iter, split_iter));
    if (a_count == 0 || a_count == node_range.count) {
        return;
    }
    auto insert_offset = bvh_nodes.size();
    bvh_nodes.push_back({});
    bvh_nodes.push_back({});
    auto &node_a = bvh_nodes[insert_offset + 0];
    auto &node_b = bvh_nodes[insert_offset + 1];
    node_a.data = magicavoxel::BvhNode::Range{.first = node_range.first, .count = a_count};
    node_b.data = magicavoxel::BvhNode::Range{.first = node_range.first + a_count, .count = node_range.count - a_count};
    node.data = magicavoxel::BvhNode::Children{.offset = static_cast<uint32_t>(insert_offset)};
    calc_bvh_node_range(model_instances, node_a);
    calc_bvh_node_range(model_instances, node_b);
    subdivide_scene_bvh(model_instances, bvh_nodes, node_a);
    subdivide_scene_bvh(model_instances, bvh_nodes, node_b);
}

void construct_scene_bvh(magicavoxel::Scene &scene) {
    scene.bvh_nodes.reserve(scene.model_instances.size() * 2);
    uint32_t const root_node_i = 0;
    auto &root = scene.bvh_nodes[root_node_i];
    root.data = magicavoxel::BvhNode::Range{0, static_cast<uint32_t>(scene.model_instances.size())};
    subdivide_scene_bvh(scene.model_instances, scene.bvh_nodes, root);
}

void sample_scene_bvh(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, magicavoxel::Scene const &scene, magicavoxel::BvhNode const &node, GvoxOffset3D const &sample_pos, uint32_t &sampled_voxel) {
    if (sample_pos.x < node.aabb_min.x ||
        sample_pos.y < node.aabb_min.y ||
        sample_pos.z < node.aabb_min.z ||
        sample_pos.x >= node.aabb_max.x ||
        sample_pos.y >= node.aabb_max.y ||
        sample_pos.z >= node.aabb_max.z ||
        sampled_voxel != 255) {
        return;
    }
    if (node.is_leaf()) {
        auto const &node_data = std::get<magicavoxel::BvhNode::Range>(node.data);
        for (uint32_t i = 0; i < node_data.count; ++i) {
            auto const &s_current_node = scene.model_instances[node_data.first + i];
            auto const &model = scene.models[s_current_node.index];
            ensure_initialized_model(blit_ctx, ctx, model);

            if (sample_pos.x < s_current_node.aabb_min.x ||
                sample_pos.y < s_current_node.aabb_min.y ||
                sample_pos.z < s_current_node.aabb_min.z ||
                sample_pos.x >= s_current_node.aabb_max.x ||
                sample_pos.y >= s_current_node.aabb_max.y ||
                sample_pos.z >= s_current_node.aabb_max.z) {
                continue;
            }
            auto rel_p = GvoxExtent3D{
                static_cast<uint32_t>(sample_pos.x - s_current_node.aabb_min.x),
                static_cast<uint32_t>(sample_pos.y - s_current_node.aabb_min.y),
                static_cast<uint32_t>(sample_pos.z - s_current_node.aabb_min.z),
            };
            rel_p = magicavoxel::rotate((s_current_node.rotation), rel_p, model.extent);
            auto const index = rel_p.x + rel_p.y * model.extent.x + rel_p.z * model.extent.x * model.extent.y;

            if (rel_p.x >= model.extent.x ||
                rel_p.y >= model.extent.y ||
                rel_p.z >= model.extent.z) {
                continue;
            }
            if (index >= model.palette_ids.size()) {
                continue;
            }
            sampled_voxel = model.palette_ids[index];
            if (sampled_voxel != 255) {
                break;
            }
        }
    } else {
        auto const &node_data = std::get<magicavoxel::BvhNode::Children>(node.data);
        auto const &node_a = scene.bvh_nodes[node_data.offset + 0];
        auto const &node_b = scene.bvh_nodes[node_data.offset + 1];
        sample_scene_bvh(blit_ctx, ctx, scene, node_a, sample_pos, sampled_voxel);
        if (sampled_voxel != 255) {
            return;
        }
        sample_scene_bvh(blit_ctx, ctx, scene, node_b, sample_pos, sampled_voxel);
    }
}

void sample_scene(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, magicavoxel::Scene &scene, GvoxOffset3D const &sample_pos, uint32_t &sampled_voxel) {
    if (scene.bvh_nodes.empty()) {
        return;
    }
    sample_scene_bvh(blit_ctx, ctx, scene, scene.bvh_nodes[0], sample_pos, sampled_voxel);
}

// Base
extern "C" void gvox_parse_adapter_magicavoxel_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(MagicavoxelParseUserState));
    new (user_state_ptr) MagicavoxelParseUserState();
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_parse_adapter_magicavoxel_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<MagicavoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~MagicavoxelParseUserState();
    free(&user_state);
}

extern "C" void gvox_parse_adapter_magicavoxel_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) {
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
                user_state.scene.models.back().input_offset != 0) {
                gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "expected a SIZE chunk before XYZI chunk");
                return;
            }
            auto &model = user_state.scene.models.back();
            uint32_t num_voxels_in_chunk = 0;
            read_var(num_voxels_in_chunk);

            model.input_offset = user_state.offset;
            model.num_voxels_in_chunk = num_voxels_in_chunk;

            if (num_voxels_in_chunk == 0) {
                user_state.offset += chunk_size - sizeof(num_voxels_in_chunk);
            }
            else {
                user_state.offset += static_cast<size_t>(num_voxels_in_chunk) * 4;
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
            const auto *name_str = temp_dict.get<char const *>("_name", nullptr);
            if (name_str != nullptr) {
                std::copy(name_str, name_str + std::min<size_t>(strlen(name_str) + 1, 65), result_transform.name.begin());
            }
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
                    magicavoxel::string_r_dict_entry(r_str, trn.rotation);
                }
                const auto *t_str = temp_dict.get<char const *>("_t", nullptr);
                if (t_str != nullptr) {
                    magicavoxel::string_t_dict_entry(t_str, trn.offset);
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
                magicavoxel::string_color_dict_entry(color_string, r, g, b);
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
            default:
                // This should never happen.
                break;
            }
            user_state.offset += chunk_size - 16u;
        } break;
        default: {
            user_state.offset += chunk_size;
        } break;
        }
    }

    if (temp_scene_info.node_infos.empty()) {
        for (size_t model_i = 0; model_i < user_state.scene.models.size(); ++model_i) {
            temp_scene_info.node_infos.emplace_back(magicavoxel::SceneShapeInfo{
                .model_id = static_cast<uint32_t>(model_i),
                .num_keyframes = 1,
                .keyframe_offset = 0,
                .loop = false,
            });
        }
    }

    if (!temp_scene_info.node_infos.empty()) {
        user_state.scene.bvh_nodes.push_back({});
        auto &root_node = user_state.scene.bvh_nodes[0];
        root_node.aabb_min = {
            std::numeric_limits<int32_t>::max(),
            std::numeric_limits<int32_t>::max(),
            std::numeric_limits<int32_t>::max(),
        };
        root_node.aabb_max = {
            std::numeric_limits<int32_t>::min(),
            std::numeric_limits<int32_t>::min(),
            std::numeric_limits<int32_t>::min(),
        };
        construct_scene(user_state.scene, temp_scene_info, 0, 0, {}, root_node.aabb_min, root_node.aabb_max);
        construct_scene_bvh(user_state.scene);
    } else {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER, "Somehow there were no scene nodes parsed from this model. Please let us know in the gvox GitHub issues what to do to reproduce this bug");
    }
}

extern "C" void gvox_parse_adapter_magicavoxel_blit_end(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/) {
}

// General
extern "C" auto gvox_parse_adapter_magicavoxel_query_details() -> GvoxParseAdapterDetails {
    return {
        .preferred_blit_mode = GVOX_BLIT_MODE_PARSE_DRIVEN,
    };
}

extern "C" auto gvox_parse_adapter_magicavoxel_query_parsable_range(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx) -> GvoxRegionRange {
    auto &user_state = *static_cast<MagicavoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    if (!user_state.scene.bvh_nodes.empty()) {
        auto const &root_node = user_state.scene.bvh_nodes[0];
        return {
            root_node.aabb_min,
            {
                static_cast<uint32_t>(root_node.aabb_max.x - root_node.aabb_min.x),
                static_cast<uint32_t>(root_node.aabb_max.y - root_node.aabb_min.y),
                static_cast<uint32_t>(root_node.aabb_max.z - root_node.aabb_min.z),
            },
        };
    }
    return {{0, 0, 0}, {0, 0, 0}};
}

extern "C" auto gvox_parse_adapter_magicavoxel_sample_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxSample {
    auto &user_state = *static_cast<MagicavoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    uint32_t voxel_data = 0;
    auto palette_id = 255u;
    if (region->data != nullptr) {
        auto const &node = *reinterpret_cast<magicavoxel::BvhNode const *>(region->data);
        sample_scene_bvh(blit_ctx, ctx, user_state.scene, node, *offset, palette_id);
    } else {
        sample_scene(blit_ctx, ctx, user_state.scene, *offset, palette_id);
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
        voxel_data = static_cast<uint8_t>(palette_id + 1);
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
    case GVOX_CHANNEL_ID_EMISSIVITY:
        if (palette_id < 255) {
            auto const palette_val = user_state.palette[palette_id];
            auto is_emissive = (user_state.materials[palette_id].content_flags & magicavoxel::MATERIAL_EMIT_BIT) != 0;
            voxel_data = std::bit_cast<uint32_t>(palette_val) * static_cast<uint32_t>(is_emissive);
        } else {
            voxel_data = 0;
        }
        break;
    default:
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Requested unsupported channel from magicavoxel file");
        break;
    }

    return {voxel_data, static_cast<uint8_t>(palette_id != 255u)};
}

// Serialize Driven
extern "C" auto gvox_parse_adapter_magicavoxel_query_region_flags(GvoxBlitContext * /*unused*/, GvoxAdapterContext * /*unused*/, GvoxRegionRange const * /*unused*/, uint32_t /*unused*/) -> uint32_t {
    return 0;
}

extern "C" auto gvox_parse_adapter_magicavoxel_load_region(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) -> GvoxRegion {
    auto const available_channels =
        uint32_t{GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_MATERIAL_ID | GVOX_CHANNEL_BIT_ROUGHNESS |
                 GVOX_CHANNEL_BIT_METALNESS | GVOX_CHANNEL_BIT_TRANSPARENCY | GVOX_CHANNEL_BIT_IOR |
                 GVOX_CHANNEL_BIT_EMISSIVITY};
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

void foreach_bvh_leaf(GvoxBlitContext *blit_ctx, MagicavoxelParseUserState &user_state, magicavoxel::Scene const &scene, magicavoxel::BvhNode const &node, uint32_t channel_flags) {
    if (node.is_leaf()) {
        user_state.thread_pool.enqueue([blit_ctx, &node, channel_flags]() {
            GvoxRegion const region = {
                .range = GvoxRegionRange{
                    .offset = {
                        node.aabb_min.x,
                        node.aabb_min.y,
                        node.aabb_min.z,
                    },
                    .extent = {
                        static_cast<uint32_t>(node.aabb_max.x - node.aabb_min.x),
                        static_cast<uint32_t>(node.aabb_max.y - node.aabb_min.y),
                        static_cast<uint32_t>(node.aabb_max.z - node.aabb_min.z),
                    },
                },
                .channels = channel_flags,
                .flags = 0u,
                .data = &node,
            };
            gvox_emit_region(blit_ctx, &region);
        });
    } else {
        auto const &node_data = std::get<magicavoxel::BvhNode::Children>(node.data);
        auto const &node_a = scene.bvh_nodes[node_data.offset + 0];
        auto const &node_b = scene.bvh_nodes[node_data.offset + 1];
        foreach_bvh_leaf(blit_ctx, user_state, scene, node_a, channel_flags);
        foreach_bvh_leaf(blit_ctx, user_state, scene, node_b, channel_flags);
    }
}

// Parse Driven
extern "C" void gvox_parse_adapter_magicavoxel_parse_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const * /*range*/, uint32_t channel_flags) {
    auto const available_channels =
        uint32_t{GVOX_CHANNEL_BIT_COLOR | GVOX_CHANNEL_BIT_MATERIAL_ID | GVOX_CHANNEL_BIT_ROUGHNESS |
                 GVOX_CHANNEL_BIT_METALNESS | GVOX_CHANNEL_BIT_TRANSPARENCY | GVOX_CHANNEL_BIT_IOR |
                 GVOX_CHANNEL_BIT_EMISSIVITY};
    if ((channel_flags & ~available_channels) != 0) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_REQUESTED_CHANNEL_NOT_PRESENT, "Tried loading a region with a channel that wasn't present in the original data");
    }
    auto &user_state = *static_cast<MagicavoxelParseUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.thread_pool.start();
    foreach_bvh_leaf(blit_ctx, user_state, user_state.scene, user_state.scene.bvh_nodes[0], channel_flags & available_channels);
    while (user_state.thread_pool.busy()) {
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
        std::this_thread::sleep_for(10ms);
#endif
    }
    user_state.thread_pool.stop();
}
