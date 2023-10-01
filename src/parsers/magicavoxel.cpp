#include <bit>
#include <gvox/stream.h>
#include <gvox/parsers/magicavoxel.h>

#include <iostream>
#include <iomanip>
#include <limits>
#include <new>
#include <vector>
#include <span>

#include "../utils/tracy.hpp"
#include "gvox/core.h"
#include "gvox/format.h"
#include "magicavoxel.hpp"

struct MagicavoxelParser {
    MagicavoxelParserConfig config{};
    GvoxVoxelDesc desc{};
    std::vector<magicavoxel::XyziModel> models = {};
    magicavoxel::Palette palette = {};

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

auto operator<<(std::ostream &out, magicavoxel::XyziModel const &m) -> std::ostream & {
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
                return res;
            }

            // Load Magicavoxel skeleton

            {
                auto header = magicavoxel::Header{};
                gvox_input_read(args->input_stream, &header, sizeof(header));

                if (header.file_header != magicavoxel::CHUNK_ID_VOX_ ||
                    (header.file_version != 150 && header.file_version != 200)) {
                    return GVOX_ERROR_UNPARSABLE_INPUT;
                }
            }

            auto main_chunk_header = magicavoxel::ChunkHeader{};
            gvox_input_read(args->input_stream, &main_chunk_header, sizeof(main_chunk_header));

            auto stream_end = static_cast<int64_t>(gvox_input_tell(args->input_stream) + main_chunk_header.child_size);
            // std::cout << "end: " << stream_end << std::endl;

            auto &models = self.models;
            auto &palette = self.palette;

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
                        return GVOX_ERROR_UNKNOWN;
                    }

                    models.push_back(magicavoxel::XyziModel{});
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
                        return GVOX_ERROR_UNKNOWN;
                    }
                    gvox_input_read(args->input_stream, &palette, sizeof(palette));
                } break;
                default: {
                    gvox_input_seek(args->input_stream, static_cast<int64_t>(chunk_header.size), GVOX_SEEK_ORIGIN_CUR);
                } break;
                }
            }

            // for (auto const &model : models) {
            //     std::cout << model << "\n\n";
            // }
            // std::cout << std::flush;

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

            if (iter.model_index >= self.models.size()) {
                out->tag = GVOX_ITERATOR_VALUE_TYPE_NULL;
                return;
            }

            auto &model = self.models[iter.model_index];

            if (iter.voxel_index == std::numeric_limits<size_t>::max()) {
                // Enter node
                iter.offset = GvoxOffset3D{0, 0, 0};
                iter.extent = GvoxExtent3D{model.extent[0], model.extent[1], model.extent[2]};
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
                iter.offset = GvoxOffset3D{0, 0, 0};
                iter.extent = GvoxExtent3D{model.extent[0], model.extent[1], model.extent[2]};
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
                iter.offset = GvoxOffset3D{static_cast<int64_t>(voxel[0]), static_cast<int64_t>(voxel[1]), static_cast<int64_t>(voxel[2])};
                iter.extent = GvoxExtent3D{1, 1, 1};
                iter.voxel = self.palette[voxel[3] - 1];
                // std::swap(iter.voxel.r, iter.voxel.b);
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
