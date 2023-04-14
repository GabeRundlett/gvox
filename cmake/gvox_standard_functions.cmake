
set(HEADER_CONTENT "#pragma once

#include <gvox/gvox.h>
#include <string_view>
#include <array>
#include <utility>

using StdIoAdapterInfo = std::pair<std::string_view, GvoxIoAdapterDescription>;
using StdContainerInfo = std::pair<std::string_view, GvoxContainerDescription>;
")

foreach(NAME ${GVOX_IO_ADAPTERS})
    target_sources(${PROJECT_NAME} PRIVATE "src/io_adapters/${NAME}.cpp")
    set(HEADER_CONTENT "${HEADER_CONTENT}
auto gvox_io_adapter_${NAME}_create(void **self, void const *config_ptr) -> GvoxResult;
auto gvox_io_adapter_${NAME}_input_read(void *self, uint8_t *data, size_t size) -> GvoxResult;
auto gvox_io_adapter_${NAME}_input_seek(void *self, long offset, GvoxSeekOrigin origin) -> GvoxResult;
void gvox_io_adapter_${NAME}_destroy(void *self);
")
    set(ADAPTER_INFOS_CONTENT "${ADAPTER_INFOS_CONTENT}
    StdIoAdapterInfo{
        \"${NAME}\",
        {
            .create     = gvox_io_adapter_${NAME}_create,
            .input_read = gvox_io_adapter_${NAME}_input_read,
            .input_seek = gvox_io_adapter_${NAME}_input_seek,
            .destroy    = gvox_io_adapter_${NAME}_destroy,
        },
    },")
endforeach()

foreach(NAME ${GVOX_CONTAINERS})
    target_sources(${PROJECT_NAME} PRIVATE "src/containers/${NAME}.cpp")
    set(HEADER_CONTENT "${HEADER_CONTENT}
auto gvox_container_${NAME}_create(void **self, void const *config_ptr) -> GvoxResult;
auto gvox_container_${NAME}_parse(void *self, GvoxParseCbInfo const *info) -> GvoxResult;
auto gvox_container_${NAME}_serialize(void *self) -> GvoxResult;
void gvox_container_${NAME}_destroy(void *self);
")
    set(CONTAINER_INFOS_CONTENT "${CONTAINER_INFOS_CONTENT}
    StdContainerInfo{
        \"${NAME}\",
        {
            .create    = gvox_container_${NAME}_create,
            .parse     = gvox_container_${NAME}_parse,
            .serialize = gvox_container_${NAME}_serialize,
            .destroy   = gvox_container_${NAME}_destroy,
        },
    },")
endforeach()

set(HEADER_CONTENT "${HEADER_CONTENT}
static constexpr auto standard_io_adapters = std::array{${ADAPTER_INFOS_CONTENT}
};
static constexpr auto standard_containers = std::array{${CONTAINER_INFOS_CONTENT}
};
")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/gvox_standard_functions.hpp" "${HEADER_CONTENT}")
