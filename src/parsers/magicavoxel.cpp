#include <bit>
#include <gvox/stream.h>
#include <gvox/parsers/magicavoxel.h>

#include <iostream>
#include <iomanip>
#include <limits>
#include <new>
#include <vector>
#include <span>
#include <algorithm>
#include <numeric>

#include "../utils/tracy.hpp"
#include "gvox/core.h"
#include "gvox/format.h"
#include "magicavoxel.hpp"

struct MagicavoxelParser {
    MagicavoxelParserConfig config{};
    GvoxVoxelDesc desc{};
    std::vector<magicavoxel::TransformKeyframe> transform_keyframes{};
    magicavoxel::Palette palette = {};
    magicavoxel::MaterialList materials{};
    magicavoxel::Scene scene{};
    std::array<uint8_t, 256> index_map{};
    bool has_index_map{};

    struct Iterator {
        size_t model_index{};
        size_t voxel_index{std::numeric_limits<size_t>::max()};
        GvoxOffset3D offset{};
        GvoxExtent3D extent{};
        magicavoxel::Color voxel{};
    };

    explicit MagicavoxelParser(MagicavoxelParserConfig const &a_config) : config{a_config} {}
    MagicavoxelParser(MagicavoxelParser const &) = delete;
    MagicavoxelParser(MagicavoxelParser &&) = delete;
    auto operator=(MagicavoxelParser const &) -> MagicavoxelParser & = delete;
    auto operator=(MagicavoxelParser &&) -> MagicavoxelParser & = delete;
    ~MagicavoxelParser() {
        gvox_destroy_voxel_desc(desc);
    }
};

void construct_scene(magicavoxel::Scene &scene, magicavoxel::SceneInfo &scene_info, uint32_t node_index, uint32_t depth, magicavoxel::Transform trn, GvoxOffset3D &min_p, GvoxOffset3D &max_p, std::string const &indent = "") {
    auto const &node_info = scene_info.node_infos[node_index];
    if (std::holds_alternative<magicavoxel::SceneTransformInfo>(node_info)) {
        auto const &t_node_info = std::get<magicavoxel::SceneTransformInfo>(node_info);
        auto new_trn = trn;
        auto rotated_offset = magicavoxel::rotate(trn.rotation, t_node_info.transform.offset);
        new_trn.offset.x += rotated_offset.x;
        new_trn.offset.y += rotated_offset.y;
        new_trn.offset.z += rotated_offset.z;
        new_trn.rotation = magicavoxel::rotate(new_trn.rotation, t_node_info.transform.rotation);
        // std::cout << indent << "transform\n";
        construct_scene(scene, scene_info, t_node_info.child_node_id, depth + 1, new_trn, min_p, max_p, indent);
    } else if (std::holds_alternative<magicavoxel::SceneGroupInfo>(node_info)) {
        auto const &g_node_info = std::get<magicavoxel::SceneGroupInfo>(node_info);
        scene.model_instances.reserve(scene.model_instances.size() + g_node_info.num_child_nodes);
        // std::cout << indent << "{\n";
        for (uint32_t child_i = 0; child_i < g_node_info.num_child_nodes; ++child_i) {
            construct_scene(
                scene, scene_info,
                scene_info.group_children_ids[g_node_info.first_child_node_id_index + child_i],
                depth + 1, trn, min_p, max_p, indent + "  ");
        }
        // std::cout << indent << "}\n";
    } else if (std::holds_alternative<magicavoxel::SceneShapeInfo>(node_info)) {
        // std::cout << indent << "model\n";
        auto const &s_node_info = std::get<magicavoxel::SceneShapeInfo>(node_info);
        scene.model_instances.push_back({});
        auto &s_current_node = scene.model_instances.back();
        s_current_node.rotation = magicavoxel::inverse(trn.rotation);
        s_current_node.index = s_node_info.model_id;
        auto &model = scene.models[s_current_node.index];
        auto extent = magicavoxel::rotate(
            magicavoxel::inverse(trn.rotation),
            GvoxExtent3D{
                model.extent[0],
                model.extent[1],
                model.extent[2],
            });
        auto extent_offset =
            GvoxExtent3D{
                static_cast<uint32_t>((trn.rotation >> 4) & 1),
                static_cast<uint32_t>((trn.rotation >> 5) & 1),
                static_cast<uint32_t>((trn.rotation >> 6) & 1),
            };
        s_current_node.offset = {
            trn.offset.x - static_cast<int32_t>(extent.x + extent_offset.x) / 2,
            trn.offset.y - static_cast<int32_t>(extent.y + extent_offset.y) / 2,
            trn.offset.z - static_cast<int32_t>(extent.z + extent_offset.z) / 2,
        };
        s_current_node.extent = extent;
        // s_current_node.aabb_min = {
        //     trn.offset.x - static_cast<int32_t>(extent.x + extent_offset.x) / 2,
        //     trn.offset.y - static_cast<int32_t>(extent.y + extent_offset.y) / 2,
        //     trn.offset.z - static_cast<int32_t>(extent.z + extent_offset.z) / 2,
        // };
        // s_current_node.aabb_max = {
        //     s_current_node.aabb_min.x + static_cast<int32_t>(extent.x),
        //     s_current_node.aabb_min.y + static_cast<int32_t>(extent.y),
        //     s_current_node.aabb_min.z + static_cast<int32_t>(extent.z),
        // };
        // min_p.x = std::min(min_p.x, s_current_node.aabb_min.x);
        // min_p.y = std::min(min_p.y, s_current_node.aabb_min.y);
        // min_p.z = std::min(min_p.z, s_current_node.aabb_min.z);
        // max_p.x = std::max(max_p.x, s_current_node.aabb_max.x);
        // max_p.y = std::max(max_p.y, s_current_node.aabb_max.y);
        // max_p.z = std::max(max_p.z, s_current_node.aabb_max.z);
    }
}

#if MAGICAVOXEL_ENABLE_BVH
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
    int64_t split_p = 0;
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
    case 0: split_p = node.aabb_min.x + static_cast<int64_t>(extent.x / 2); break;
    case 1: split_p = node.aabb_min.y + static_cast<int64_t>(extent.y / 2); break;
    case 2: split_p = node.aabb_min.z + static_cast<int64_t>(extent.z / 2); break;
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
    scene.bvh_nodes.reserve(scene.model_instances.size() * 2 - 1);
    uint32_t const root_node_i = 0;
    auto &root = scene.bvh_nodes[root_node_i];
    root.data = magicavoxel::BvhNode::Range{0, static_cast<uint32_t>(scene.model_instances.size())};
    subdivide_scene_bvh(scene.model_instances, scene.bvh_nodes, root);
}
#endif

auto operator<<(std::ostream &out, magicavoxel::Model const &m) -> std::ostream & {
    out << "extent = {" << m.extent[0] << ", " << m.extent[1] << ", " << m.extent[2] << "}\n";
    out << "voxel_count = " << m.voxel_count << "\n";
    out << "input_offset = " << m.input_offset;
    return out;
}

auto gvox_parser_magicavoxel_description() GVOX_FUNC_ATTRIB->GvoxParserDescription {
    return GvoxParserDescription{
        .create = [](void **out_self, GvoxParserCreateCbArgs const *args) -> GvoxResult {
            MagicavoxelParserConfig config;
            if (args->config != nullptr) {
                config = *static_cast<MagicavoxelParserConfig const *>(args->config);
            } else {
                config = {};
            }
            auto &self = *(new (std::nothrow) MagicavoxelParser(config));
            *out_self = &self;

            auto const attribs = std::array{
                GvoxAttribute{
                    .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                    .next = nullptr,
                    .type = GVOX_ATTRIBUTE_TYPE_ALBEDO,
                    .format = GVOX_FORMAT_R8G8B8A8_SRGB,
                },
            };
            auto voxel_desc_info = GvoxVoxelDescCreateInfo{
                .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
                .next = nullptr,
                .attribute_count = static_cast<uint32_t>(attribs.size()),
                .attributes = attribs.data(),
            };
            auto res = gvox_create_voxel_desc(&voxel_desc_info, &self.desc);
            if (res != GVOX_SUCCESS) {
                delete &self;
                return res;
            }

            // Load Magicavoxel skeleton

            {
                auto header = magicavoxel::Header{};
                gvox_input_read(args->input_stream, &header, sizeof(header));

                if (header.file_header != magicavoxel::CHUNK_ID_VOX_ ||
                    (header.file_version != 150 && header.file_version != 200)) {
                    delete &self;
                    return GVOX_ERROR_UNPARSABLE_INPUT;
                }
            }

            auto main_chunk_header = magicavoxel::ChunkHeader{};
            gvox_input_read(args->input_stream, &main_chunk_header, sizeof(main_chunk_header));

            auto stream_end = static_cast<int64_t>(gvox_input_tell(args->input_stream) + main_chunk_header.child_size);
            // std::cout << "end: " << stream_end << std::endl;

            auto &models = self.scene.models;
            auto &palette = self.palette;
            auto &transform_keyframes = self.transform_keyframes;
            auto &materials = self.materials;
            magicavoxel::SceneInfo scene_info{};
            std::vector<magicavoxel::ModelKeyframe> shape_keyframes{};
            std::vector<magicavoxel::Layer> layers{};

            auto temp_dict = magicavoxel::Dictionary{};

            while (true) {
                auto curr = gvox_input_tell(args->input_stream);
                if (curr >= stream_end) {
                    break;
                }
                auto chunk_header = magicavoxel::ChunkHeader{};
                gvox_input_read(args->input_stream, &chunk_header, sizeof(chunk_header));
                // auto char_array = std::bit_cast<std::array<char, 4>>(chunk_header.id);
                // curr = gvox_input_tell(args->input_stream);
                // std::cout << " - [" << curr << ", " << curr + chunk_header.size << "]: " << std::string_view{char_array.data(), char_array.size()} << std::endl;
                switch (chunk_header.id) {
                case magicavoxel::CHUNK_ID_SIZE: {
                    if (chunk_header.size != 12 || chunk_header.child_size != 0) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected chunk size for SIZE chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }

                    models.push_back(magicavoxel::Model{});
                    auto &model = models.back();
                    gvox_input_read(args->input_stream, &model.extent, sizeof(model.extent));
                } break;
                case magicavoxel::CHUNK_ID_XYZI: {
                    if (models.empty() ||
                        models.back().extent[0] == 0 ||
                        models.back().extent[1] == 0 ||
                        models.back().extent[2] == 0 ||
                        models.back().valid()) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "expected a SIZE chunk before XYZI chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    auto &model = models.back();
                    gvox_input_read(args->input_stream, &model.voxel_count, sizeof(model.voxel_count));
                    model.input_offset = gvox_input_tell(args->input_stream);
                    gvox_input_seek(args->input_stream, static_cast<int64_t>(chunk_header.size) - static_cast<int64_t>(sizeof(model.voxel_count)), GVOX_SEEK_ORIGIN_CUR);
                } break;
                case magicavoxel::CHUNK_ID_RGBA: {
                    if (chunk_header.size != sizeof(palette)) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected chunk size for RGBA chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    gvox_input_read(args->input_stream, &palette, sizeof(palette));
                } break;
                case magicavoxel::CHUNK_ID_nTRN: {
                    uint32_t node_id = 0;
                    gvox_input_read(args->input_stream, &node_id, sizeof(node_id));
                    if (!magicavoxel::read_dict(args->input_stream, temp_dict)) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read nTRN dictionary");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    auto result_transform = magicavoxel::SceneTransformInfo{};
                    const auto *name_str = temp_dict.get<char const *>("_name", nullptr);
                    if (name_str != nullptr) {
                        std::copy(name_str, name_str + std::min<size_t>(strlen(name_str) + 1, 65), result_transform.name.begin());
                    }
                    result_transform.hidden = temp_dict.get<bool>("_hidden", false);
                    result_transform.loop = temp_dict.get<bool>("_loop", false);
                    uint32_t reserved_id = 0;
                    gvox_input_read(args->input_stream, &result_transform.child_node_id, sizeof(result_transform.child_node_id));
                    gvox_input_read(args->input_stream, &reserved_id, sizeof(reserved_id));
                    gvox_input_read(args->input_stream, &result_transform.layer_id, sizeof(result_transform.layer_id));
                    gvox_input_read(args->input_stream, &result_transform.num_keyframes, sizeof(result_transform.num_keyframes));
                    if (reserved_id != UINT32_MAX) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected values for reserved_id in nTRN chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    if (result_transform.num_keyframes == 0) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "must have at least 1 frame in nTRN chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    result_transform.keyframe_offset = transform_keyframes.size();
                    transform_keyframes.resize(result_transform.keyframe_offset + result_transform.num_keyframes);
                    for (uint32_t i = 0; i < result_transform.num_keyframes; i++) {
                        if (!magicavoxel::read_dict(args->input_stream, temp_dict)) {
                            // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read nTRN keyframe dictionary");
                            delete &self;
                            return GVOX_ERROR_UNKNOWN;
                        }
                        auto &trn = transform_keyframes[result_transform.keyframe_offset + i].transform;
                        const auto *r_str = temp_dict.get<char const *>("_r", nullptr);
                        if (r_str != nullptr) {
                            magicavoxel::string_r_dict_entry(r_str, trn.rotation);
                        }
                        const auto *t_str = temp_dict.get<char const *>("_t", nullptr);
                        if (t_str != nullptr) {
                            magicavoxel::string_t_dict_entry(t_str, trn.offset);
                        }
                        transform_keyframes[result_transform.keyframe_offset + i].frame_index = temp_dict.get<uint32_t>("_f", 0);
                    }
                    result_transform.transform = transform_keyframes[result_transform.keyframe_offset].transform;
                    scene_info.node_infos.resize(std::max<size_t>(node_id + 1, scene_info.node_infos.size()));
                    scene_info.node_infos[node_id] = result_transform;
                } break;
                case magicavoxel::CHUNK_ID_nGRP: {
                    uint32_t node_id = 0;
                    gvox_input_read(args->input_stream, &node_id, sizeof(node_id));
                    // has dictionary, we don't care
                    magicavoxel::read_dict(args->input_stream, temp_dict);
                    auto result_group = magicavoxel::SceneGroupInfo{};
                    uint32_t num_child_nodes = 0;
                    gvox_input_read(args->input_stream, &num_child_nodes, sizeof(num_child_nodes));
                    if (num_child_nodes != 0u) {
                        size_t const prior_size = scene_info.group_children_ids.size();
                        scene_info.group_children_ids.resize(prior_size + num_child_nodes);
                        gvox_input_read(args->input_stream, &scene_info.group_children_ids[prior_size], sizeof(uint32_t) * num_child_nodes);
                        result_group.first_child_node_id_index = (uint32_t)prior_size;
                        result_group.num_child_nodes = num_child_nodes;
                    }
                    scene_info.node_infos.resize(std::max<size_t>(node_id + 1, scene_info.node_infos.size()));
                    scene_info.node_infos[node_id] = result_group;
                } break;
                case magicavoxel::CHUNK_ID_nSHP: {
                    uint32_t node_id = 0;
                    gvox_input_read(args->input_stream, &node_id, sizeof(node_id));
                    magicavoxel::read_dict(args->input_stream, temp_dict);
                    auto result_shape = magicavoxel::SceneShapeInfo{};
                    result_shape.loop = temp_dict.get<bool>("_loop", false);
                    gvox_input_read(args->input_stream, &result_shape.num_keyframes, sizeof(result_shape.num_keyframes));
                    if (result_shape.num_keyframes == 0) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "must have at least 1 frame in nSHP chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    result_shape.keyframe_offset = shape_keyframes.size();
                    shape_keyframes.resize(result_shape.keyframe_offset + result_shape.num_keyframes);
                    for (uint32_t i = 0; i < result_shape.num_keyframes; i++) {
                        auto &model_index = shape_keyframes[result_shape.keyframe_offset + i].model_index;
                        gvox_input_read(args->input_stream, &model_index, sizeof(model_index));
                        if (!magicavoxel::read_dict(args->input_stream, temp_dict)) {
                            // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read nSHP keyframe dictionary");
                            delete &self;
                            return GVOX_ERROR_UNKNOWN;
                        }
                        shape_keyframes[result_shape.keyframe_offset + i].frame_index = temp_dict.get<uint32_t>("_f", 0);
                    }
                    result_shape.model_id = shape_keyframes[result_shape.keyframe_offset].model_index;
                    scene_info.node_infos.resize(std::max<size_t>(node_id + 1, scene_info.node_infos.size()));
                    scene_info.node_infos[node_id] = result_shape;
                } break;
                case magicavoxel::CHUNK_ID_IMAP: {
                    if (chunk_header.size != 256) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected chunk size for IMAP chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    gvox_input_read(args->input_stream, &self.index_map, sizeof(self.index_map));
                    self.has_index_map = true;
                } break;
                case magicavoxel::CHUNK_ID_LAYR: {
                    int32_t layer_id = 0;
                    int32_t reserved_id = 0;
                    gvox_input_read(args->input_stream, &layer_id, sizeof(layer_id));
                    if (!magicavoxel::read_dict(args->input_stream, temp_dict)) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read dictionary in LAYR chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    gvox_input_read(args->input_stream, &reserved_id, sizeof(reserved_id));
                    if (reserved_id != -1) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected value for reserved_id in LAYR chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
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
                    layers.resize(std::max<size_t>(static_cast<size_t>(layer_id + 1), layers.size()));
                    layers[static_cast<size_t>(layer_id)] = result_layer;
                } break;
                case magicavoxel::CHUNK_ID_MATL: {
                    int32_t material_id = 0;
                    gvox_input_read(args->input_stream, &material_id, sizeof(material_id));
                    material_id = (material_id - 1) & 0xFF; // incoming material 256 is material 0
                    if (!magicavoxel::read_dict(args->input_stream, temp_dict)) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "failed to read dictionary in MATL chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    char const *type_string = temp_dict.get<char const *>("_type", nullptr);
                    if (type_string != nullptr) {
                        if (0 == strcmp(type_string, "_diffuse")) {
                            materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::DIFFUSE;
                        } else if (0 == strcmp(type_string, "_metal")) {
                            materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::METAL;
                        } else if (0 == strcmp(type_string, "_glass")) {
                            materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::GLASS;
                        } else if (0 == strcmp(type_string, "_emit")) {
                            materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::EMIT;
                        } else if (0 == strcmp(type_string, "_blend")) {
                            materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::BLEND;
                        } else if (0 == strcmp(type_string, "_media")) {
                            materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::MEDIA;
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
                            materials[static_cast<size_t>(material_id)].content_flags |= mat_bit;
                            *(&materials[static_cast<size_t>(material_id)].metal + field_offset) = static_cast<float>(atof(prop_str));
                        }
                        ++field_offset;
                    }
                } break;
                case magicavoxel::CHUNK_ID_MATT: {
                    if (chunk_header.size < 16u) {
                        // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "unexpected chunk size for MATT chunk");
                        delete &self;
                        return GVOX_ERROR_UNKNOWN;
                    }
                    int32_t material_id = 0;
                    gvox_input_read(args->input_stream, &material_id, sizeof(material_id));
                    material_id = material_id & 0xFF;
                    int32_t material_type = 0;
                    gvox_input_read(args->input_stream, &material_type, sizeof(material_type));
                    float material_weight = 0.0f;
                    gvox_input_read(args->input_stream, &material_weight, sizeof(material_weight));
                    uint32_t property_bits = 0u;
                    gvox_input_read(args->input_stream, &property_bits, sizeof(property_bits));
                    switch (material_type) {
                    case 0:
                        materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::DIFFUSE;
                        break;
                    case 1:
                        materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::METAL;
                        materials[static_cast<size_t>(material_id)].content_flags |= magicavoxel::MATERIAL_METAL_BIT;
                        materials[static_cast<size_t>(material_id)].metal = material_weight;
                        break;
                    case 2:
                        materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::GLASS;
                        materials[static_cast<size_t>(material_id)].content_flags |= magicavoxel::MATERIAL_TRANS_BIT;
                        materials[static_cast<size_t>(material_id)].trans = material_weight;
                        break;
                    case 3:
                        materials[static_cast<size_t>(material_id)].type = magicavoxel::MaterialType::EMIT;
                        materials[static_cast<size_t>(material_id)].content_flags |= magicavoxel::MATERIAL_EMIT_BIT;
                        materials[static_cast<size_t>(material_id)].emit = material_weight;
                        break;
                    default:
                        // This should never happen.
                        break;
                    }
                    if (chunk_header.size > 16) {
                        gvox_input_seek(args->input_stream, static_cast<int64_t>(chunk_header.size) - 16, GVOX_SEEK_ORIGIN_CUR);
                    }
                } break;
                default: {
                    gvox_input_seek(args->input_stream, static_cast<int64_t>(chunk_header.size), GVOX_SEEK_ORIGIN_CUR);
                } break;
                }
            }

            if (scene_info.node_infos.empty()) {
                for (size_t model_i = 0; model_i < models.size(); ++model_i) {
                    scene_info.node_infos.emplace_back(magicavoxel::SceneShapeInfo{
                        .model_id = static_cast<uint32_t>(model_i),
                        .num_keyframes = 1,
                        .keyframe_offset = 0,
                        .loop = false,
                    });
                }
            }

            if (!scene_info.node_infos.empty()) {
#if MAGICAVOXEL_ENABLE_BVH
                self.scene.bvh_nodes.push_back({});
                auto &root_node = self.scene.bvh_nodes[0];
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
                GvoxOffset3D &aabb_min = root_node.aabb_min;
                GvoxOffset3D &aabb_max = root_node.aabb_max;
#else
                GvoxOffset3D aabb_min{};
                GvoxOffset3D aabb_max{};
#endif
                construct_scene(self.scene, scene_info, 0, 0, {}, aabb_min, aabb_max);
#if MAGICAVOXEL_ENABLE_BVH
                construct_scene_bvh(self.scene);
#endif
            } else {
                // gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER, "Somehow there were no scene nodes parsed from this model. Please let us know in the gvox GitHub issues what to do to reproduce this bug");
                return GVOX_ERROR_UNKNOWN;
            }

            return GVOX_SUCCESS;
        },
        .destroy = [](void *self) { delete static_cast<MagicavoxelParser *>(self); },
        .create_from_input = [](GvoxInputStream input_stream, GvoxParser *user_parser) -> GvoxResult {
            auto header = magicavoxel::Header{};
            gvox_input_read(input_stream, &header, sizeof(header));

            if (header.file_header != magicavoxel::CHUNK_ID_VOX_ || (header.file_version != 150 && header.file_version != 200)) {
                return GVOX_ERROR_UNPARSABLE_INPUT;
            }

            auto parser_ci = GvoxParserCreateInfo{};
            parser_ci.struct_type = GVOX_STRUCT_TYPE_PARSER_CREATE_INFO;
            parser_ci.next = nullptr;
            parser_ci.cb_args.config = nullptr;
            parser_ci.cb_args.input_stream = input_stream;
            parser_ci.description = gvox_parser_magicavoxel_description();

            gvox_input_seek(input_stream, -static_cast<int64_t>(sizeof(header)), GVOX_SEEK_ORIGIN_CUR);
            return gvox_create_parser(&parser_ci, user_parser);
        },
        .create_input_iterator = [](void *self_ptr, void **out_iterator_ptr) -> void {
            *out_iterator_ptr = new (std::nothrow) MagicavoxelParser::Iterator();
        },
        .destroy_iterator = [](void *self_ptr, void *iterator_ptr) -> void {
        },
        .iterator_next = [](void *self_ptr, void **iterator_ptr, GvoxInputStream input_stream, GvoxIteratorValue *out) -> void {
            auto &self = *static_cast<MagicavoxelParser *>(self_ptr);
            auto &iter = *static_cast<MagicavoxelParser::Iterator *>(*iterator_ptr);

            if (iter.model_index >= self.scene.model_instances.size()) {
                out->tag = GVOX_ITERATOR_VALUE_TYPE_NULL;
                return;
            }

            auto &model_instance = self.scene.model_instances[iter.model_index];
            auto &model = self.scene.models[model_instance.index];

            if (iter.voxel_index == std::numeric_limits<size_t>::max()) {
                // Enter node
                iter.offset = model_instance.offset;
                iter.extent = model_instance.extent;
                out->tag = GVOX_ITERATOR_VALUE_TYPE_NODE_BEGIN;
                out->range = GvoxRange{
                    .offset = {.axis_n = 3, .axis = &iter.offset.x},
                    .extent = {.axis_n = 3, .axis = &iter.extent.x},
                };
                iter.voxel_index = 0;
                return;
            }

            if (iter.voxel_index >= model.voxel_count) {
                // Exit node
                iter.offset = model_instance.offset;
                iter.extent = model_instance.extent;
                iter.voxel_index = std::numeric_limits<size_t>::max();
                out->tag = GVOX_ITERATOR_VALUE_TYPE_NODE_END;
                out->range = GvoxRange{
                    .offset = {.axis_n = 3, .axis = &iter.offset.x},
                    .extent = {.axis_n = 3, .axis = &iter.extent.x},
                };
                ++iter.model_index;
                return;
            }

            {
                // Next voxel
                auto voxel = std::array<uint8_t, 4>{};
                gvox_input_seek(input_stream, model.input_offset + static_cast<int64_t>(iter.voxel_index * sizeof(voxel)), GVOX_SEEK_ORIGIN_BEG);
                gvox_input_read(input_stream, &voxel, sizeof(voxel));

                auto offset = magicavoxel::rotate(model_instance.rotation, GvoxExtent3D{static_cast<uint64_t>(voxel[0]), static_cast<uint64_t>(voxel[1]), static_cast<uint64_t>(voxel[2])}, model_instance.extent);
                iter.offset.x = offset.x + model_instance.offset.x;
                iter.offset.y = offset.y + model_instance.offset.y;
                iter.offset.z = offset.z + model_instance.offset.z;
                iter.extent = GvoxExtent3D{1, 1, 1};
                iter.voxel = self.palette[voxel[3] - 1];
                std::swap(iter.voxel.r, iter.voxel.b);
                out->tag = GVOX_ITERATOR_VALUE_TYPE_LEAF;
                out->range = GvoxRange{
                    .offset = {.axis_n = 3, .axis = &iter.offset.x},
                    .extent = {.axis_n = 3, .axis = &iter.extent.x},
                };
                out->voxel_data = static_cast<void *>(&iter.voxel);
                out->voxel_desc = self.desc;
                ++iter.voxel_index;
            }
        },
    };
}
