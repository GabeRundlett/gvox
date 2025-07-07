#include <gvox/gvox.h>

#include <gvox/containers/raw.h>
#include <gvox/streams/input/byte_buffer.h>

#include <iostream>
#include <array>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstdint>

#define HANDLE_RES(x, message)               \
    if ((x) != GVOX_SUCCESS) {               \
        std::cerr << (message) << std::endl; \
        return -1;                           \
    }


#pragma pack(push, 1)
struct VoxelData
{
    uint8_t color[3];
    uint8_t palette_id;
    uint8_t type;
    float roughness;
    float opacity;
    float metalness;
    float ior;
    float reflectivity;
    float emissivity;
    float density;
    float phase;
};
#pragma pack(pop)


auto main() -> int {
    // First create a voxel description.
    // This will describe the per-voxel data in the container we're using to store our model.
    auto *rgb_voxel_desc = GvoxVoxelDesc{};
    {
        auto const attribs = std::array{
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_ALBEDO_PACKED,
                .format = GVOX_STANDARD_FORMAT_R8G8B8_SRGB,
            },
        };
        auto voxel_desc_info = GvoxVoxelDescCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
            .next = nullptr,
            .attribute_count = static_cast<uint32_t>(attribs.size()),
            .attributes = attribs.data(),
        };
        HANDLE_RES(gvox_create_voxel_desc(&voxel_desc_info, &rgb_voxel_desc), "Failed to create voxel desc")
    }

    // Now we'll create that container.
    auto *raw_container = GvoxContainer{};
    {
        auto raw_container_conf = GvoxRaw3dContainerConfig{
            .voxel_desc = rgb_voxel_desc,
        };
        auto cont_info = GvoxContainerCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,
            .next = nullptr,
            .description = gvox_container_raw3d_description(),
            .cb_args = {
                .struct_type = {}, // ?
                .next = nullptr,
                .config = &raw_container_conf,
            },
        };
        HANDLE_RES(gvox_create_container(&cont_info, &raw_container), "Failed to create raw container")
    }

    // Now we'll create our input data. We can load from a file, but here we'll
    // first load the file into a buffer, and load from that buffer.
    auto *file_input = GvoxInputStream{};
    {
        auto file = std::ifstream{"assets/building.vox", std::ios::binary};
        auto size = std::filesystem::file_size("assets/building.vox");
        auto bytes = std::vector<uint8_t>(size);
        file.read(reinterpret_cast<char *>(bytes.data()), static_cast<std::streamsize>(size));
        auto config = GvoxByteBufferInputStreamConfig{.data = bytes.data(), .size = bytes.size()};
        auto input_ci = GvoxInputStreamCreateInfo{};
        input_ci.struct_type = GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_INFO;
        input_ci.next = nullptr;
        input_ci.cb_args.config = &config;
        input_ci.description = gvox_input_stream_byte_buffer_description();
        HANDLE_RES(gvox_create_input_stream(&input_ci, &file_input), "Failed to create (byte buffer) input stream");
    }

    // Now we'll create our parser. This can be automatically detected from the input data itself.
    auto *file_parser = GvoxParser{};
    {
        auto parser_collection = GvoxParserDescriptionCollection{
            .struct_type = GVOX_STRUCT_TYPE_PARSER_DESCRIPTION_COLLECTION,
            .next = nullptr,
            .descriptions = nullptr,
            .description_n = 0,
        };
        gvox_enumerate_standard_parser_descriptions(&parser_collection.descriptions, &parser_collection.description_n);
        HANDLE_RES(gvox_create_parser_from_input(&parser_collection, file_input, &file_parser), "Failed to create parser");
    }

    // And of course, create that iterator.
    auto *input_iterator = GvoxIterator{};
    {
        auto parse_iter_ci = GvoxParseIteratorCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_PARSE_ITERATOR_CREATE_INFO,
            .next = nullptr,
            .parser = file_parser,
        };
        auto iter_ci = GvoxIteratorCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_ITERATOR_CREATE_INFO,
            .next = &parse_iter_ci,
        };
        gvox_create_iterator(&iter_ci, &input_iterator);
    }

    // Now we'll simply iterate over the input.
    auto iter_value = GvoxIteratorValue{};
    auto advance_info = GvoxIteratorAdvanceInfo{
        .input_stream = file_input,
        .mode = GVOX_ITERATOR_ADVANCE_MODE_NEXT,
    };
    while (true) {
        gvox_iterator_advance(input_iterator, &advance_info, &iter_value);
        if (iter_value.tag == GVOX_ITERATOR_VALUE_TYPE_NULL) {
            break;
        }
        if (iter_value.tag == GVOX_ITERATOR_VALUE_TYPE_LEAF) {
            uint32_t desc_count =
                gvox_voxel_desc_attribute_count(iter_value.voxel_desc);

            std::vector<GvoxAttribute> attrs(desc_count);
            std::vector<GvoxAttributeMapping> mappings(desc_count);
            for (uint32_t i = 0; i < desc_count; ++i) {
                HANDLE_RES(
                    gvox_voxel_desc_get_attribute(iter_value.voxel_desc, i, &attrs[i]),
                    "Failed to get attribute");
                mappings[i] = GvoxAttributeMapping{ .dst_index = static_cast<uint8_t>(i),
                                                    .src_index = static_cast<uint8_t>(i),
                                                    .src1_index = static_cast<uint8_t>(0),
                                                    .src2_index = static_cast<uint8_t>(0) };
            }

            GvoxVoxelDesc leaf_desc{};
            GvoxVoxelDescCreateInfo di{
                .struct_type     = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
                .next            = nullptr,
                .attribute_count = desc_count,
                .attributes      = attrs.data(),
            };
            HANDLE_RES(
                gvox_create_voxel_desc(&di, &leaf_desc),
                "Failed to create temporary voxel descriptor");

            std::vector<uint8_t> buffer(sizeof(VoxelData));
            HANDLE_RES(
                gvox_translate_voxel(iter_value.voxel_data,
                                    iter_value.voxel_desc,
                                    buffer.data(),
                                    leaf_desc,
                                    mappings.data(),
                                    desc_count),
                "Failed to translate voxel data");

                VoxelData vd{};
                uint32_t arb_seen = 0;
                uint8_t *raw = buffer.data();

            for (uint32_t i = 0; i < desc_count; ++i) {
                GvoxAttribute attr;
                gvox_voxel_desc_get_attribute(iter_value.voxel_desc, i, &attr);

                switch (attr.type) {
                    case GVOX_ATTRIBUTE_TYPE_ALBEDO_PACKED:
                        // 3 bytes R,G,B
                        memcpy(vd.color, raw, 3);
                        raw += 3;
                        break;

                    case GVOX_ATTRIBUTE_TYPE_ARBITRARY_INTEGER:
                        if (arb_seen == 0)
                            vd.palette_id = raw[0];
                        else if (arb_seen == 1)
                            vd.type       = raw[0];
                        ++arb_seen;
                        raw += 1;
                        break;

                    case GVOX_ATTRIBUTE_TYPE_ROUGHNESS:
                        vd.roughness = *reinterpret_cast<float*>(raw);
                        raw += sizeof(float);
                        break;

                    case GVOX_ATTRIBUTE_TYPE_OPACITY:
                        vd.opacity = *reinterpret_cast<float*>(raw);
                        raw += sizeof(float);
                        break;

                    case GVOX_ATTRIBUTE_TYPE_METALNESS:
                        vd.metalness = *reinterpret_cast<float*>(raw);
                        raw += sizeof(float);
                        break;

                    case GVOX_ATTRIBUTE_TYPE_IOR:
                        vd.ior = *reinterpret_cast<float*>(raw);
                        raw += sizeof(float);
                        break;

                    case GVOX_ATTRIBUTE_TYPE_REFLECTIVITY:
                        vd.reflectivity = *reinterpret_cast<float*>(raw);
                        raw += sizeof(float);
                        break;

                    case GVOX_ATTRIBUTE_TYPE_EMISSIVITY:
                        vd.emissivity = *reinterpret_cast<float*>(raw);
                        raw += sizeof(float);
                        break;

                    case GVOX_ATTRIBUTE_TYPE_DENSITY:
                        vd.density = *reinterpret_cast<float*>(raw);
                        raw += sizeof(float);
                        break;

                    case GVOX_ATTRIBUTE_TYPE_PHASE:
                        vd.phase = *reinterpret_cast<float*>(raw);
                        raw += sizeof(float);
                        break;

                    default:
                        // TODO: check format and increment by size
                        break;
                }
            }

            std::cout
                << "Voxel leaf:\n"
                << "  color       = ("
                << static_cast<uint32_t>(vd.color[0]) << ", "
                << static_cast<uint32_t>(vd.color[1]) << ", "
                << static_cast<uint32_t>(vd.color[2]) << ")\n"
                << "  palette_id  = " << static_cast<uint32_t>(vd.palette_id) << "\n"
                << "  type        = " << static_cast<uint32_t>(vd.type) << "\n"
                << "  roughness   = " << vd.roughness << "\n"
                << "  opacity     = " << vd.opacity << "\n"
                << "  metalness   = " << vd.metalness << "\n"
                << "  ior         = " << vd.ior << "\n"
                << "  reflectivity= " << vd.reflectivity << "\n"
                << "  emissivity  = " << vd.emissivity << "\n"
                << "  density     = " << vd.density << "\n"
                << "  phase       = " << vd.phase << "\n"
                << std::endl;

            gvox_destroy_voxel_desc(leaf_desc);

            // And for every "Leaf" node in the model, we'll write it into our container.
            auto fill_info = GvoxFillInfo{
                .struct_type = GVOX_STRUCT_TYPE_FILL_INFO,
                .next = nullptr,
                .src_data = iter_value.voxel_data,
                .src_desc = iter_value.voxel_desc,
                .dst = raw_container,
                .range = iter_value.range,
            };
            HANDLE_RES(gvox_fill(&fill_info), "Failed to do fill");
        }
    }

    gvox_destroy_iterator(input_iterator);
    gvox_destroy_input_stream(file_input);
    gvox_destroy_parser(file_parser);
    gvox_destroy_container(raw_container);
    gvox_destroy_voxel_desc(rgb_voxel_desc);
}
