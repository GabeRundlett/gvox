#include <gvox_standard_functions.hpp>

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

#define HANDLE_GET_DESC(type, Type, TYPE)                                                                                                                       \
    ZoneScoped;                                                                                                                                                 \
    auto name_str = std::string_view{name};                                                                                                                     \
    auto iter = std::find_if(standard_##type##s.begin(), standard_##type##s.end(), [name_str](Std##Type##Info const &info) { return info.first == name_str; }); \
    if (iter == standard_##type##s.end()) {                                                                                                                     \
        return GVOX_ERROR_UNKNOWN_STANDARD_##TYPE;                                                                                                              \
    }                                                                                                                                                           \
    *desc = iter->second;                                                                                                                                       \
    return GVOX_SUCCESS

auto gvox_get_standard_input_stream_description(char const *name, GvoxInputStreamDescription *desc) GVOX_FUNC_ATTRIB->GvoxResult { HANDLE_GET_DESC(input_stream, InputStream, INPUT_STREAM); }
auto gvox_get_standard_output_stream_description(char const *name, GvoxOutputStreamDescription *desc) GVOX_FUNC_ATTRIB->GvoxResult { HANDLE_GET_DESC(output_stream, OutputStream, OUTPUT_STREAM); }
auto gvox_get_standard_parser_description(char const *name, GvoxParserDescription *desc) GVOX_FUNC_ATTRIB->GvoxResult { HANDLE_GET_DESC(parser, Parser, PARSER); }
auto gvox_get_standard_serializer_description(char const *name, GvoxSerializerDescription *desc) GVOX_FUNC_ATTRIB->GvoxResult { HANDLE_GET_DESC(serializer, Serializer, SERIALIZER); }
auto gvox_get_standard_container_description(char const *name, GvoxContainerDescription *desc) GVOX_FUNC_ATTRIB->GvoxResult { HANDLE_GET_DESC(container, Container, CONTAINER); }

#undef HANDLE_GET_DESC

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

auto gvox_create_parser_from_input(GvoxInputStream input_stream, GvoxParser *user_parser) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    auto initial_input_pos = gvox_input_tell(input_stream);
    for (auto const &[parser_name, parser_desc] : standard_parsers) {
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
