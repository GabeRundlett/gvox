#include <string_view>
#include <span>

#include "types.hpp"
#include "utils/handle.hpp"

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

    if (!info->stream_chain) {
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

void gvox_destroy_input_stream(GvoxInputStream handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }
void gvox_destroy_output_stream(GvoxOutputStream handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }
void gvox_destroy_parser(GvoxParser handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }
void gvox_destroy_serializer(GvoxSerializer handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }
void gvox_destroy_container(GvoxContainer handle) GVOX_FUNC_ATTRIB { destroy_handle(handle); }

auto gvox_input_read(GvoxInputStream handle, uint8_t *data, size_t size) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    return handle->desc.read(handle->self, handle->next, data, size);
}
auto gvox_input_seek(GvoxInputStream handle, long offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    return handle->desc.seek(handle->self, handle->next, offset, origin);
}
auto gvox_input_tell(GvoxInputStream handle) GVOX_FUNC_ATTRIB->long {
    ZoneScoped;
    return handle->desc.tell(handle->self, handle->next);
}

auto gvox_output_write(GvoxOutputStream handle, uint8_t *data, size_t size) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    return handle->desc.write(handle->self, handle->next, data, size);
}
auto gvox_output_seek(GvoxOutputStream handle, long offset, GvoxSeekOrigin origin) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    return handle->desc.seek(handle->self, handle->next, offset, origin);
}
auto gvox_output_seek(GvoxOutputStream handle) -> long {
    ZoneScoped;
    return handle->desc.tell(handle->self, handle->next);
}

auto gvox_create_parser_from_input(GvoxParserDescription const *parsers_ptr, uint32_t parser_n, GvoxInputStream input_stream, GvoxParser *user_parser) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    auto initial_input_pos = gvox_input_tell(input_stream);
    auto parsers = std::span<GvoxParserDescription const>(parsers_ptr, parser_n);
    for (auto const &parser_desc : parsers) {
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
    return GVOX_ERROR_UNKNOWN_STANDARD_PARSER;
}
