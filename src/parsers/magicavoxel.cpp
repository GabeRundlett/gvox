#include <gvox/stream.h>
#include <gvox/parsers/magicavoxel.h>

#include <iostream>
#include <iomanip>
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

    struct Iterator {
        size_t index{};
        GvoxOffset3D pos{};
        std::array<uint8_t, 4> voxel;
    };

    explicit MagicavoxelParser(MagicavoxelParserConfig const &a_config) : config{a_config} {}
    ~MagicavoxelParser() {
        gvox_destroy_voxel_desc(desc);
    }
};

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

            // auto load_res = static_cast<MagicavoxelParser *>(*self)->load(args->input_stream);
            // if (load_res != GVOX_SUCCESS) {
            //     return load_res;
            // }
            return GVOX_SUCCESS;
        },
        .destroy = [](void *self) { delete static_cast<MagicavoxelParser *>(self); },
        .create_from_input = [](GvoxInputStream input_stream, GvoxParser *user_parser) -> GvoxResult {
            uint32_t file_header = 0;
            uint32_t file_version = 0;
            gvox_input_read(input_stream, &file_header, sizeof(file_header));
            gvox_input_read(input_stream, &file_version, sizeof(file_version));

            if (file_header != magicavoxel::CHUNK_ID_VOX_ || (file_version != 150 && file_version != 200)) {
                return GVOX_ERROR_UNPARSABLE_INPUT;
            }

            auto parser_ci = GvoxParserCreateInfo{};
            parser_ci.struct_type = GVOX_STRUCT_TYPE_PARSER_CREATE_INFO;
            parser_ci.next = nullptr;
            parser_ci.cb_args.config = nullptr;
            parser_ci.cb_args.input_stream = input_stream;
            parser_ci.description = gvox_parser_magicavoxel_description();

            return gvox_create_parser(&parser_ci, user_parser);
        },
        .create_input_iterator = [](void *self_ptr, void **out_iterator_ptr) -> void {
            *out_iterator_ptr = new (std::nothrow) MagicavoxelParser::Iterator();
        },
        .destroy_iterator = [](void *self_ptr, void *iterator_ptr) -> void {
        },
        .iterator_next = [](void *self_ptr, void **iterator_ptr, GvoxIteratorValue *out) -> void {
            auto &self = *static_cast<MagicavoxelParser *>(self_ptr);
            auto &iter = *static_cast<MagicavoxelParser::Iterator *>(*iterator_ptr);
            if (iter.index < 10) {
                out->tag = GVOX_ITERATOR_VALUE_TYPE_VOXEL;

                // TODO
                iter.pos = {static_cast<int64_t>(iter.index), 3, 2};

                out->voxel.pos = {
                    .axis_n = 3,
                    .axis = &iter.pos.x,
                };
                out->voxel.data = static_cast<void *>(iter.voxel.data());
                out->voxel.desc = self.desc;
            } else {
                out->tag = GVOX_ITERATOR_VALUE_TYPE_NULL;
            }
            ++iter.index;
        },
    };
}
