#include <bit>
#include <gvox/stream.h>
#include <gvox/parsers/magicavoxel.h>

#include <iostream>
#include <iomanip>
#include <limits>
#include <new>
#include <variant>
#include <vector>
#include <span>
#include <algorithm>
#include <numeric>

#include "../utils/tracy.hpp"
#include "gvox/core.h"
#include "gvox/format.h"
#include "gvox/gvox.h"
#include "magicavoxel.hpp"

struct ModelInstance {
    size_t parent_group_begin_index{};
    GvoxOffset3D offset{};
    GvoxExtent3D extent{};
    uint32_t index{};
    int8_t rotation{1 << 2};
};

struct GroupInstanceBegin {
    size_t parent_group_begin_index{};
    GvoxOffset3D offset{};
    GvoxExtent3D extent{};
    size_t end_index{};
};
struct GroupInstanceEnd {
    size_t parent_group_begin_index{};
};

using IteratorNode = std::variant<ModelInstance, GroupInstanceBegin, GroupInstanceEnd>;

struct Scene {
    std::vector<magicavoxel::Model> models{};
    std::vector<IteratorNode> iterator_nodes{};
    magicavoxel::Transform transform;
};

struct MagicavoxelParser {
    struct Iterator {
        size_t iterator_index{};
        size_t voxel_index{std::numeric_limits<size_t>::max()};
        GvoxOffset3D offset{};
        GvoxExtent3D extent{};
        magicavoxel::Color voxel{};
    };

    MagicavoxelParserConfig config{};
    GvoxVoxelDesc desc{};
    std::vector<magicavoxel::TransformKeyframe> transform_keyframes{};
    magicavoxel::Palette palette = {};
    magicavoxel::MaterialList materials{};
    Scene scene{};
    std::array<uint8_t, 256> index_map{};
    bool has_index_map{};

    explicit MagicavoxelParser(MagicavoxelParserConfig const &a_config) : config{a_config} {}
    MagicavoxelParser(MagicavoxelParser const &) = delete;
    MagicavoxelParser(MagicavoxelParser &&) = delete;
    auto operator=(MagicavoxelParser const &) -> MagicavoxelParser & = delete;
    auto operator=(MagicavoxelParser &&) -> MagicavoxelParser & = delete;
    ~MagicavoxelParser() {
        gvox_destroy_voxel_desc(desc);
    }
};

void construct_scene(Scene &scene, magicavoxel::SceneInfo &scene_info, uint32_t node_index, size_t parent_group_begin_index, magicavoxel::Transform trn, GvoxOffset3D &min_p, GvoxOffset3D &max_p) {
    auto const &node_info = scene_info.node_infos[node_index];
    if (std::holds_alternative<magicavoxel::SceneTransformInfo>(node_info)) {
        auto const &t_node_info = std::get<magicavoxel::SceneTransformInfo>(node_info);
        auto new_trn = trn;
        auto rotated_offset = magicavoxel::rotate(trn.rotation, t_node_info.transform.offset);
        new_trn.offset.x += rotated_offset.x;
        new_trn.offset.y += rotated_offset.y;
        new_trn.offset.z += rotated_offset.z;
        new_trn.rotation = magicavoxel::rotate(new_trn.rotation, t_node_info.transform.rotation);
        construct_scene(scene, scene_info, t_node_info.child_node_id, parent_group_begin_index, new_trn, min_p, max_p);
    } else if (std::holds_alternative<magicavoxel::SceneGroupInfo>(node_info)) {
        auto const &g_node_info = std::get<magicavoxel::SceneGroupInfo>(node_info);
        scene.iterator_nodes.reserve(scene.iterator_nodes.size() + g_node_info.num_child_nodes + 2);
        auto group_begin_index = scene.iterator_nodes.size();
        scene.iterator_nodes.emplace_back(GroupInstanceBegin{.parent_group_begin_index = parent_group_begin_index});
        GvoxOffset3D group_min_p{};
        group_min_p.x = std::numeric_limits<int64_t>::max();
        group_min_p.y = group_min_p.x;
        group_min_p.z = group_min_p.x;
        GvoxOffset3D group_max_p{};
        group_max_p.x = std::numeric_limits<int64_t>::min();
        group_max_p.y = group_max_p.x;
        group_max_p.z = group_max_p.x;
        for (uint32_t child_i = 0; child_i < g_node_info.num_child_nodes; ++child_i) {
            construct_scene(
                scene, scene_info,
                scene_info.group_children_ids[g_node_info.first_child_node_id_index + child_i],
                group_begin_index, trn, group_min_p, group_max_p);
        }
        auto end_index = scene.iterator_nodes.size();
        scene.iterator_nodes.emplace_back(GroupInstanceEnd{.parent_group_begin_index = group_begin_index});
        auto &group_begin = std::get<GroupInstanceBegin>(scene.iterator_nodes[group_begin_index]);
        group_begin.offset = group_min_p;
        group_begin.extent = GvoxExtent3D{
            static_cast<uint64_t>(group_max_p.x - group_min_p.x),
            static_cast<uint64_t>(group_max_p.y - group_min_p.y),
            static_cast<uint64_t>(group_max_p.z - group_min_p.z),
        };
        group_begin.end_index = end_index;
        std::get<GroupInstanceBegin>(scene.iterator_nodes[group_begin_index]).end_index = end_index;
        min_p.x = std::min(min_p.x, group_min_p.x);
        min_p.y = std::min(min_p.y, group_min_p.y);
        min_p.z = std::min(min_p.z, group_min_p.z);
        max_p.x = std::max(max_p.x, group_max_p.x);
        max_p.y = std::max(max_p.y, group_max_p.y);
        max_p.z = std::max(max_p.z, group_max_p.z);
    } else if (std::holds_alternative<magicavoxel::SceneShapeInfo>(node_info)) {
        auto const &s_node_info = std::get<magicavoxel::SceneShapeInfo>(node_info);
        scene.iterator_nodes.emplace_back(ModelInstance{});
        auto &s_current_node = std::get<ModelInstance>(scene.iterator_nodes.back());
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
        auto extent_offset = GvoxExtent3D{
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
        min_p.x = std::min(min_p.x, s_current_node.offset.x);
        min_p.y = std::min(min_p.y, s_current_node.offset.y);
        min_p.z = std::min(min_p.z, s_current_node.offset.z);
        max_p.x = std::max(max_p.x, s_current_node.offset.x + static_cast<int64_t>(s_current_node.extent.x));
        max_p.y = std::max(max_p.y, s_current_node.offset.y + static_cast<int64_t>(s_current_node.extent.y));
        max_p.z = std::max(max_p.z, s_current_node.offset.z + static_cast<int64_t>(s_current_node.extent.z));
    }
}

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
                GvoxOffset3D min_p{};
                min_p.x = std::numeric_limits<int64_t>::max();
                min_p.y = min_p.x;
                min_p.z = min_p.x;
                GvoxOffset3D max_p{};
                max_p.x = std::numeric_limits<int64_t>::min();
                max_p.y = max_p.x;
                max_p.z = max_p.x;
                construct_scene(self.scene, scene_info, 0, 0, {}, min_p, max_p);
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
        .create_input_iterator = [](void * /*self_ptr*/, void **out_iterator_ptr) -> void {
            *out_iterator_ptr = new (std::nothrow) MagicavoxelParser::Iterator();
        },
        .destroy_iterator = [](void * /*self_ptr*/, void *iterator_ptr) -> void { delete static_cast<MagicavoxelParser::Iterator *>(iterator_ptr); },
        .iterator_advance = [](void *self_ptr, void **iterator_ptr, GvoxIteratorAdvanceInfo const *info, GvoxIteratorValue *out) -> void {
            auto &self = *static_cast<MagicavoxelParser *>(self_ptr);
            auto &iter = *static_cast<MagicavoxelParser::Iterator *>(*iterator_ptr);
            auto mode = info->mode;
            while (true) {
                if (iter.iterator_index >= self.scene.iterator_nodes.size()) {
                    out->tag = GVOX_ITERATOR_VALUE_TYPE_NULL;
                    return;
                }
                auto &node = self.scene.iterator_nodes[iter.iterator_index];
                if (std::holds_alternative<ModelInstance>(node)) {
                    auto &model_instance = std::get<ModelInstance>(node);
                    auto &model = self.scene.models[model_instance.index];
                    if (iter.voxel_index == std::numeric_limits<size_t>::max()) {
                        if (info->mode == GVOX_ITERATOR_ADVANCE_MODE_SKIP_BRANCH) {
                            auto &parent_group_begin = std::get<GroupInstanceBegin>(self.scene.iterator_nodes[model_instance.parent_group_begin_index]);
                            iter.iterator_index = parent_group_begin.end_index;
                            mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT;
                            continue;
                        } else {
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
                    }
                    if (iter.voxel_index >= model.voxel_count || info->mode == GVOX_ITERATOR_ADVANCE_MODE_SKIP_BRANCH) {
                        // Exit node
                        iter.offset = model_instance.offset;
                        iter.extent = model_instance.extent;
                        iter.voxel_index = std::numeric_limits<size_t>::max();
                        out->tag = GVOX_ITERATOR_VALUE_TYPE_NODE_END;
                        out->range = GvoxRange{
                            .offset = {.axis_n = 3, .axis = &iter.offset.x},
                            .extent = {.axis_n = 3, .axis = &iter.extent.x},
                        };
                        ++iter.iterator_index;
                        return;
                    }
                    {
                        // Next voxel
                        auto voxel = std::array<uint8_t, 4>{};
                        gvox_input_seek(info->input_stream, model.input_offset + static_cast<int64_t>(iter.voxel_index * sizeof(voxel)), GVOX_SEEK_ORIGIN_BEG);
                        gvox_input_read(info->input_stream, &voxel, sizeof(voxel));
                        auto offset = magicavoxel::rotate(model_instance.rotation, GvoxExtent3D{static_cast<uint64_t>(voxel[0]), static_cast<uint64_t>(voxel[1]), static_cast<uint64_t>(voxel[2])}, model_instance.extent);
                        iter.offset.x = static_cast<int64_t>(offset.x) + model_instance.offset.x;
                        iter.offset.y = static_cast<int64_t>(offset.y) + model_instance.offset.y;
                        iter.offset.z = static_cast<int64_t>(offset.z) + model_instance.offset.z;
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
                        return;
                    }
                } else if (std::holds_alternative<GroupInstanceBegin>(node)) {
                    // Enter node
                    auto &group_begin = std::get<GroupInstanceBegin>(node);
                    if (info->mode == GVOX_ITERATOR_ADVANCE_MODE_SKIP_BRANCH) {
                        auto &parent_group_begin = std::get<GroupInstanceBegin>(self.scene.iterator_nodes[group_begin.parent_group_begin_index]);
                        iter.iterator_index = parent_group_begin.end_index;
                        mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT;
                        continue;
                    } else {
                        iter.offset = group_begin.offset;
                        iter.extent = group_begin.extent;
                        out->tag = GVOX_ITERATOR_VALUE_TYPE_NODE_BEGIN;
                        out->range = GvoxRange{
                            .offset = {.axis_n = 3, .axis = &iter.offset.x},
                            .extent = {.axis_n = 3, .axis = &iter.extent.x},
                        };
                        ++iter.iterator_index;
                        return;
                    }
                } else if (std::holds_alternative<GroupInstanceEnd>(node)) {
                    // Exit node
                    auto &group_end = std::get<GroupInstanceEnd>(node);
                    auto &group_begin = std::get<GroupInstanceBegin>(self.scene.iterator_nodes[group_end.parent_group_begin_index]);
                    iter.offset = group_begin.offset;
                    iter.extent = group_begin.extent;
                    out->tag = GVOX_ITERATOR_VALUE_TYPE_NODE_END;
                    out->range = GvoxRange{
                        .offset = {.axis_n = 3, .axis = &iter.offset.x},
                        .extent = {.axis_n = 3, .axis = &iter.extent.x},
                    };
                    ++iter.iterator_index;
                    return;
                }
            }
        },
    };
}
