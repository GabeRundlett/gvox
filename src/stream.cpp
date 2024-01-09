#include <gvox/gvox.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <array>

#include "types.hpp"
#include "utils/handle.hpp"
#include "utils/tracy.hpp"

#define HANDLE_CREATE(Type, TYPE)                                                      \
    ZoneScoped;                                                                        \
    HANDLE_NEW(Type, TYPE)                                                             \
    (*handle)->desc = info->description;                                               \
    {                                                                                  \
        auto create_result = (*handle)->desc.create(&(*handle)->self, &info->cb_args); \
        if (create_result != GVOX_SUCCESS) {                                           \
            delete *handle;                                                            \
            return create_result;                                                      \
        }                                                                              \
    }

auto gvox_create_input_stream(GvoxInputStreamCreateInfo const *info, GvoxInputStream *handle) GVOX_FUNC_ATTRIB->GvoxResult {
    HANDLE_CREATE(InputStream, INPUT_STREAM)

    if (info->stream_chain == nullptr) {
        return GVOX_SUCCESS;
    }

    return gvox_create_input_stream(info->stream_chain, &((*handle)->next));
}
auto gvox_create_output_stream(GvoxOutputStreamCreateInfo const *info, GvoxOutputStream *handle) GVOX_FUNC_ATTRIB->GvoxResult {
    HANDLE_CREATE(OutputStream, OUTPUT_STREAM)
    return GVOX_SUCCESS;
}
auto gvox_create_parser(GvoxParserCreateInfo const *info, GvoxParser *handle) GVOX_FUNC_ATTRIB->GvoxResult {
    HANDLE_CREATE(Parser, PARSER)
    return GVOX_SUCCESS;
}
auto gvox_create_serializer(GvoxSerializerCreateInfo const *info, GvoxSerializer *handle) GVOX_FUNC_ATTRIB->GvoxResult {
    HANDLE_CREATE(Serializer, SERIALIZER)
    return GVOX_SUCCESS;
}
auto gvox_create_container(GvoxContainerCreateInfo const *info, GvoxContainer *handle) GVOX_FUNC_ATTRIB->GvoxResult {
    HANDLE_CREATE(Container, CONTAINER)
    return GVOX_SUCCESS;
}
auto gvox_create_iterator(GvoxIteratorCreateInfo const *info, GvoxIterator *handle) GVOX_FUNC_ATTRIB->GvoxResult {
    HANDLE_NEW(Iterator, ITERATOR)

    if (info->next == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }

    auto s_type = static_cast<GvoxChainStruct const *>(info->next)->struct_type;
    switch (s_type) {
    case GVOX_STRUCT_TYPE_PARSE_ITERATOR_CREATE_INFO: {
        auto p_info = static_cast<GvoxParseIteratorCreateInfo const *>(info->next);
        (*handle)->destroy_iterator = p_info->parser->desc.destroy_iterator;
        (*handle)->iterator_advance = p_info->parser->desc.iterator_advance;
        (*handle)->parent_self = p_info->parser->self;
        p_info->parser->desc.create_iterator((*handle)->parent_self, &(*handle)->self);
    } break;
    case GVOX_STRUCT_TYPE_CONTAINER_ITERATOR_CREATE_INFO: {
        auto p_info = static_cast<GvoxContainerIteratorCreateInfo const *>(info->next);
        (*handle)->destroy_iterator = p_info->container->desc.destroy_iterator;
        (*handle)->iterator_advance = p_info->container->desc.iterator_advance;
        (*handle)->parent_self = p_info->container->self;
        p_info->container->desc.create_iterator((*handle)->parent_self, &(*handle)->self);
    } break;
    default: return GVOX_ERROR_INVALID_ARGUMENT;
    }

    return GVOX_SUCCESS;
}

void gvox_destroy_input_stream(GvoxInputStream handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }
void gvox_destroy_output_stream(GvoxOutputStream handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }
void gvox_destroy_parser(GvoxParser handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }
void gvox_destroy_serializer(GvoxSerializer handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }
void gvox_destroy_container(GvoxContainer handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }
void gvox_destroy_iterator(GvoxIterator handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }

auto gvox_input_read(GvoxInputStream handle, void *data, size_t size) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    return handle->desc.read(handle->self, handle->next, data, size);
}
auto gvox_input_seek(GvoxInputStream handle, int64_t offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    return handle->desc.seek(handle->self, handle->next, offset, origin);
}
auto gvox_input_tell(GvoxInputStream handle) GVOX_FUNC_ATTRIB->int64_t {
    ZoneScoped;
    return handle->desc.tell(handle->self, handle->next);
}

auto gvox_output_write(GvoxOutputStream handle, void *data, size_t size) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    return handle->desc.write(handle->self, handle->next, data, size);
}
auto gvox_output_seek(GvoxOutputStream handle, int64_t offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    return handle->desc.seek(handle->self, handle->next, offset, origin);
}
auto gvox_output_seek(GvoxOutputStream handle) -> int64_t {
    ZoneScoped;
    return handle->desc.tell(handle->self, handle->next);
}

// #include <gvox/parsers/image.h>
#include <gvox/parsers/magicavoxel.h>

namespace {
    const auto std_parser_descriptions = std::array{
        // gvox_parser_image_description(),
        gvox_parser_magicavoxel_description(),
        gvox_parser_magicavoxel_xraw_description(),
    };
} // namespace

auto gvox_enumerate_standard_parser_descriptions(GvoxParserDescription const **out_descriptions, uint32_t *description_n) GVOX_FUNC_ATTRIB->GvoxResult {
    *out_descriptions = std_parser_descriptions.data();
    *description_n = static_cast<uint32_t>(std_parser_descriptions.size());
    return GVOX_SUCCESS;
}

auto gvox_create_parser_from_input(GvoxParserDescriptionCollection const *parsers, GvoxInputStream input_stream, GvoxParser *user_parser) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    auto initial_input_pos = gvox_input_tell(input_stream);
    auto const *parser_iter = parsers;
    while (parser_iter != nullptr) {
        if (parser_iter->struct_type == GVOX_STRUCT_TYPE_PARSER_DESCRIPTION_COLLECTION) {
            auto parser_descriptions = std::span<GvoxParserDescription const>(parser_iter->descriptions, parser_iter->description_n);
            for (auto const &parser_desc : parser_descriptions) {
                ZoneScoped;
                if (parser_desc.create_from_input != nullptr) {
                    auto creation_result = parser_desc.create_from_input(input_stream, user_parser);
                    // we should reset the input stream to where it was before the
                    // checking took place.
                    gvox_input_seek(input_stream, initial_input_pos, GVOX_SEEK_ORIGIN_BEG);
                    if (creation_result == GVOX_SUCCESS) {
                        return GVOX_SUCCESS;
                    }
                }
            }
        }
        parser_iter = static_cast<GvoxParserDescriptionCollection const *>(parser_iter->next);
    }
    return GVOX_ERROR_UNKNOWN_STANDARD_PARSER;
}

void gvox_iterator_advance(GvoxIterator handle, GvoxIteratorAdvanceInfo const *info, GvoxIteratorValue *value) {
    handle->iterator_advance(handle->parent_self, &handle->self, info, value);
}
