
foreach(NAME ${GVOX_INPUT_ADAPTERS})
    target_sources(${PROJECT_NAME} PRIVATE "src/adapters/input/${NAME}.cpp")
    set(INPUT_ADAPTER_NAMES_CONTENT "${INPUT_ADAPTER_NAMES_CONTENT}
    \"${NAME}\",")
    set(INPUT_ADAPTER_INFOS_CONTENT "${INPUT_ADAPTER_INFOS_CONTENT}
    GvoxInputAdapterInfo{
        .name_str = \"${NAME}\",
        .begin = gvox_input_adapter_${NAME}_begin,
        .end = gvox_input_adapter_${NAME}_end,
        .read = gvox_input_adapter_${NAME}_read,
    },")
endforeach()
    foreach(NAME ${GVOX_OUTPUT_ADAPTERS})
    target_sources(${PROJECT_NAME} PRIVATE "src/adapters/output/${NAME}.cpp")
    set(OUTPUT_ADAPTER_NAMES_CONTENT "${OUTPUT_ADAPTER_NAMES_CONTENT}
    \"${NAME}\",")
    set(OUTPUT_ADAPTER_INFOS_CONTENT "${OUTPUT_ADAPTER_INFOS_CONTENT}
    GvoxOutputAdapterInfo{
        .name_str = \"${NAME}\",
        .begin = gvox_output_adapter_${NAME}_begin,
        .end = gvox_output_adapter_${NAME}_end,
        .write = gvox_output_adapter_${NAME}_write,
        .reserve = gvox_output_adapter_${NAME}_reserve,
    },")
endforeach()
    foreach(NAME ${GVOX_PARSE_ADAPTERS})
    target_sources(${PROJECT_NAME} PRIVATE "src/adapters/parse/${NAME}.cpp")
    set(PARSE_ADAPTER_NAMES_CONTENT "${PARSE_ADAPTER_NAMES_CONTENT}
    \"${NAME}\",")
    set(PARSE_ADAPTER_INFOS_CONTENT "${PARSE_ADAPTER_INFOS_CONTENT}
    GvoxParseAdapterInfo{
        .name_str = \"${NAME}\",
        .begin = gvox_parse_adapter_${NAME}_begin,
        .end = gvox_parse_adapter_${NAME}_end,
        .query_region_flags = gvox_parse_adapter_${NAME}_query_region_flags,
        .load_region = gvox_parse_adapter_${NAME}_load_region,
        .unload_region = gvox_parse_adapter_${NAME}_unload_region,
        .sample_region = gvox_parse_adapter_${NAME}_sample_region,
    },")
endforeach()
    foreach(NAME ${GVOX_SERIALIZE_ADAPTERS})
    target_sources(${PROJECT_NAME} PRIVATE "src/adapters/serialize/${NAME}.cpp")
    set(SERIALIZE_ADAPTER_NAMES_CONTENT "${SERIALIZE_ADAPTER_NAMES_CONTENT}
    \"${NAME}\",")
    set(SERIALIZE_ADAPTER_INFOS_CONTENT "${SERIALIZE_ADAPTER_INFOS_CONTENT}
    GvoxSerializeAdapterInfo{
        .name_str = \"${NAME}\",
        .begin = gvox_serialize_adapter_${NAME}_begin,
        .end = gvox_serialize_adapter_${NAME}_end,
        .serialize_region = gvox_serialize_adapter_${NAME}_serialize_region,
    },")
endforeach()

set(ADAPTERS_HEADER_CONTENT "#pragma once

#include <array>
#include <string_view>
#include <gvox/gvox.h>

static constexpr std::array input_adapter_names = {${INPUT_ADAPTER_NAMES_CONTENT}
};
static constexpr std::array output_adapter_names = {${OUTPUT_ADAPTER_NAMES_CONTENT}
};
static constexpr std::array parse_adapter_names = {${PARSE_ADAPTER_NAMES_CONTENT}
};
static constexpr std::array serialize_adapter_names = {${SERIALIZE_ADAPTER_NAMES_CONTENT}
};
")

foreach(NAME ${GVOX_INPUT_ADAPTERS})
    string(MAKE_C_IDENTIFIER "${NAME}" NAME_UPPER)
    string(TOUPPER "${NAME_UPPER}" NAME_UPPER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_INPUT_ADAPTER_${NAME_UPPER}_BUILT_STATIC=1)
    set(ADAPTERS_HEADER_CONTENT "${ADAPTERS_HEADER_CONTENT}
extern \"C\" void gvox_input_adapter_${NAME}_begin(GvoxAdapterContext *ctx, void *config);
extern \"C\" void gvox_input_adapter_${NAME}_end(GvoxAdapterContext *ctx);
extern \"C\" void gvox_input_adapter_${NAME}_read(GvoxAdapterContext *ctx, size_t position, size_t size, void *data);
")
endforeach()
    foreach(NAME ${GVOX_OUTPUT_ADAPTERS})
    string(MAKE_C_IDENTIFIER "${NAME}" NAME_UPPER)
    string(TOUPPER "${NAME_UPPER}" NAME_UPPER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_OUTPUT_ADAPTER_${NAME_UPPER}_BUILT_STATIC=1)
    set(ADAPTERS_HEADER_CONTENT "${ADAPTERS_HEADER_CONTENT}
extern \"C\" void gvox_output_adapter_${NAME}_begin(GvoxAdapterContext *ctx, void *config);
extern \"C\" void gvox_output_adapter_${NAME}_end(GvoxAdapterContext *ctx);
extern \"C\" void gvox_output_adapter_${NAME}_write(GvoxAdapterContext *ctx, size_t position, size_t size, void const *data);
extern \"C\" void gvox_output_adapter_${NAME}_reserve(GvoxAdapterContext *ctx, size_t size);
")
endforeach()
foreach(NAME ${GVOX_PARSE_ADAPTERS})
    string(MAKE_C_IDENTIFIER "${NAME}" NAME_UPPER)
    string(TOUPPER "${NAME_UPPER}" NAME_UPPER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_PARSE_ADAPTER_${NAME_UPPER}_BUILT_STATIC=1)
    set(ADAPTERS_HEADER_CONTENT "${ADAPTERS_HEADER_CONTENT}
extern \"C\" void gvox_parse_adapter_${NAME}_begin(GvoxAdapterContext *ctx, void *config);
extern \"C\" void gvox_parse_adapter_${NAME}_end(GvoxAdapterContext *ctx);
extern \"C\" auto gvox_parse_adapter_${NAME}_query_region_flags(GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_id) -> uint32_t;
extern \"C\" auto gvox_parse_adapter_${NAME}_load_region(GvoxAdapterContext *ctx, GvoxOffset3D const *offset, uint32_t channel_id) -> GvoxRegion;
extern \"C\" void gvox_parse_adapter_${NAME}_unload_region(GvoxAdapterContext *ctx, GvoxRegion *region);
extern \"C\" auto gvox_parse_adapter_${NAME}_sample_region(GvoxAdapterContext *ctx, GvoxRegion const *region, GvoxOffset3D const *offset, uint32_t channel_id) -> uint32_t;
")
endforeach()
foreach(NAME ${GVOX_SERIALIZE_ADAPTERS})
    string(MAKE_C_IDENTIFIER "${NAME}" NAME_UPPER)
    string(TOUPPER "${NAME_UPPER}" NAME_UPPER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_SERIALIZE_ADAPTER_${NAME_UPPER}_BUILT_STATIC=1)
    set(ADAPTERS_HEADER_CONTENT "${ADAPTERS_HEADER_CONTENT}
extern \"C\" void gvox_serialize_adapter_${NAME}_begin(GvoxAdapterContext *ctx, void *config);
extern \"C\" void gvox_serialize_adapter_${NAME}_end(GvoxAdapterContext *ctx);
extern \"C\" void gvox_serialize_adapter_${NAME}_serialize_region(GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags);
")
endforeach()

set(ADAPTERS_HEADER_CONTENT "${ADAPTERS_HEADER_CONTENT}
static constexpr std::array input_adapter_infos = {${INPUT_ADAPTER_INFOS_CONTENT}
};
static constexpr std::array output_adapter_infos = {${OUTPUT_ADAPTER_INFOS_CONTENT}
};
static constexpr std::array parse_adapter_infos = {${PARSE_ADAPTER_INFOS_CONTENT}
};
static constexpr std::array serialize_adapter_infos = {${SERIALIZE_ADAPTER_INFOS_CONTENT}
};
")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/adapters.hpp" "${ADAPTERS_HEADER_CONTENT}")
