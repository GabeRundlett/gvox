
set(HEADER_CONTENT "#pragma once

#include <gvox/stream.h>
#include <string_view>
#include <array>
#include <utility>

using StdInputStreamInfo     = std::pair<std::string_view, GvoxInputStreamDescription>;
using StdOutputStreamInfo    = std::pair<std::string_view, GvoxOutputStreamDescription>;
using StdParserInfo           = std::pair<std::string_view, GvoxParserDescription>;
using StdSerializerInfo       = std::pair<std::string_view, GvoxSerializerDescription>;
using StdContainerInfo        = std::pair<std::string_view, GvoxContainerDescription>;
")

foreach(NAME ${GVOX_INPUT_STREAMS})
    target_sources(${PROJECT_NAME} PRIVATE "src/streams/input/${NAME}.cpp")
    set(HEADER_CONTENT "${HEADER_CONTENT}
    auto gvox_input_stream_${NAME}_create(void **self, GvoxInputStreamCreateCbArgs const *args) -> GvoxResult;
    auto gvox_input_stream_${NAME}_read(void *self, GvoxInputStream next_handle, uint8_t *data, size_t size) -> GvoxResult;
    auto gvox_input_stream_${NAME}_seek(void *self, GvoxInputStream next_handle, long offset, GvoxSeekOrigin origin) -> GvoxResult;
    auto gvox_input_stream_${NAME}_tell(void *self, GvoxInputStream next_handle) -> long;
    void gvox_input_stream_${NAME}_destroy(void *self);
    ")
    set(INPUT_STREAM_INFOS_CONTENT "${INPUT_STREAM_INFOS_CONTENT}
    StdInputStreamInfo{
        \"${NAME}\",
        {
            .create  = gvox_input_stream_${NAME}_create,
            .read    = gvox_input_stream_${NAME}_read,
            .seek    = gvox_input_stream_${NAME}_seek,
            .tell    = gvox_input_stream_${NAME}_tell,
            .destroy = gvox_input_stream_${NAME}_destroy,
        },
    },")
endforeach()

foreach(NAME ${GVOX_OUTPUT_STREAMS})
    target_sources(${PROJECT_NAME} PRIVATE "src/streams/output/${NAME}.cpp")
    set(HEADER_CONTENT "${HEADER_CONTENT}
    auto gvox_output_stream_${NAME}_create(void **self, GvoxOutputStreamCreateCbArgs const *) -> GvoxResult;
    auto gvox_output_stream_${NAME}_write(void *self, GvoxOutputStream next_handle, uint8_t *data, size_t size) -> GvoxResult;
    auto gvox_output_stream_${NAME}_seek (void *self, GvoxOutputStream next_handle, long offset, GvoxSeekOrigin origin) -> GvoxResult;
    auto gvox_output_stream_${NAME}_tell (void *self, GvoxOutputStream next_handle) -> long;
    void gvox_output_stream_${NAME}_destroy(void *self);
    ")
    set(OUTPUT_STREAM_INFOS_CONTENT "${OUTPUT_STREAM_INFOS_CONTENT}
    StdOutputStreamInfo{
        \"${NAME}\",
        {
            .create  = gvox_output_stream_${NAME}_create,
            .write   = gvox_output_stream_${NAME}_write,
            .seek    = gvox_output_stream_${NAME}_seek,
            .tell    = gvox_output_stream_${NAME}_tell,
            .destroy = gvox_output_stream_${NAME}_destroy,
        },
    },")
endforeach()

foreach(NAME ${GVOX_PARSERS})
    target_sources(${PROJECT_NAME} PRIVATE "src/parsers/${NAME}.cpp")
    set(HEADER_CONTENT "${HEADER_CONTENT}
    auto gvox_parser_${NAME}_create(void **self, GvoxParserCreateCbArgs const *args) -> GvoxResult;
    void gvox_parser_${NAME}_destroy(void *self);
    auto gvox_parser_${NAME}_create_from_input(GvoxInputStream input_stream, GvoxParser *user_parser) -> GvoxResult;
    ")
    set(PARSER_INFOS_CONTENT "${PARSER_INFOS_CONTENT}
    StdParserInfo{
        \"${NAME}\",
        {
            .create  = gvox_parser_${NAME}_create,
            .destroy = gvox_parser_${NAME}_destroy,
            .create_from_input = gvox_parser_${NAME}_create_from_input,
        },
    },")
endforeach()

foreach(NAME ${GVOX_SERIALIZERS})
    target_sources(${PROJECT_NAME} PRIVATE "src/serializers/${NAME}.cpp")
    set(HEADER_CONTENT "${HEADER_CONTENT}
    auto gvox_serializer_${NAME}_create(void **self, GvoxSerializerCreateCbArgs const *args) -> GvoxResult;
    void gvox_serializer_${NAME}_destroy(void *self);
    ")
    set(SERIALIZER_INFOS_CONTENT "${SERIALIZER_INFOS_CONTENT}
    StdSerializerInfo{
        \"${NAME}\",
        {
            .create  = gvox_serializer_${NAME}_create,
            .destroy = gvox_serializer_${NAME}_destroy,
        },
    },")
endforeach()

foreach(NAME ${GVOX_CONTAINERS})
    target_sources(${PROJECT_NAME} PRIVATE "src/containers/${NAME}.cpp")
    set(HEADER_CONTENT "${HEADER_CONTENT}
    auto gvox_container_${NAME}_create(void **self_ptr, GvoxContainerCreateCbArgs const *args) -> GvoxResult;
    void gvox_container_${NAME}_destroy(void *self_ptr);
    auto gvox_container_${NAME}_fill(void *self_ptr, void *single_voxel_data, GvoxVoxelDesc src_voxel_desc, GvoxRange range) -> GvoxResult;
    auto gvox_container_${NAME}_sample(void *self_ptr, void *out_voxel_data, GvoxVoxelDesc out_voxel_desc, GvoxOffset offset) -> GvoxResult;
    ")
    set(CONTAINER_INFOS_CONTENT "${CONTAINER_INFOS_CONTENT}
    StdContainerInfo{
        \"${NAME}\",
        GvoxContainerDescription{
            .create         = gvox_container_${NAME}_create,
            .destroy        = gvox_container_${NAME}_destroy,
            .fill           = gvox_container_${NAME}_fill,
            .sample         = gvox_container_${NAME}_sample,
        },
    },")
endforeach()

set(HEADER_CONTENT "${HEADER_CONTENT}
static constexpr auto standard_input_streams = std::array{${INPUT_STREAM_INFOS_CONTENT}
    StdInputStreamInfo{\"null\", {}},
};
static constexpr auto standard_output_streams = std::array{${OUTPUT_STREAM_INFOS_CONTENT}
    StdOutputStreamInfo{\"null\", {}},
};
static constexpr auto standard_parsers = std::array{${PARSER_INFOS_CONTENT}
    StdParserInfo{\"null\", {}},
};
static constexpr auto standard_serializers = std::array{${SERIALIZER_INFOS_CONTENT}
    StdSerializerInfo{\"null\", {}},
};
static constexpr auto standard_containers = std::array{${CONTAINER_INFOS_CONTENT}
    StdContainerInfo{\"null\", {}},
};
")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gvox_standard_functions.hpp" "${HEADER_CONTENT}")
