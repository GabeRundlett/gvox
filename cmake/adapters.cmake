
foreach(NAME ${GVOX_INPUT_ADAPTERS})
target_sources(${PROJECT_NAME} PRIVATE "src/adapters/input/${NAME}.cpp")
set(INPUT_ADAPTER_NAMES_CONTENT "${INPUT_ADAPTER_NAMES_CONTENT}
\"${NAME}\",")
set(INPUT_ADAPTER_INFOS_CONTENT "${INPUT_ADAPTER_INFOS_CONTENT}
GVoxInputAdapterInfo{
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
GVoxOutputAdapterInfo{
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
GVoxParseAdapterInfo{
    .name_str = \"${NAME}\",
    .begin = gvox_parse_adapter_${NAME}_begin,
    .end = gvox_parse_adapter_${NAME}_end,
    .query_region_flags = gvox_parse_adapter_${NAME}_query_region_flags,
    .load_region = gvox_parse_adapter_${NAME}_load_region,
    .sample_data = gvox_parse_adapter_${NAME}_sample_data,
},")
endforeach()
foreach(NAME ${GVOX_SERIALIZE_ADAPTERS})
target_sources(${PROJECT_NAME} PRIVATE "src/adapters/serialize/${NAME}.cpp")
set(SERIALIZE_ADAPTER_NAMES_CONTENT "${SERIALIZE_ADAPTER_NAMES_CONTENT}
\"${NAME}\",")
set(SERIALIZE_ADAPTER_INFOS_CONTENT "${SERIALIZE_ADAPTER_INFOS_CONTENT}
GVoxSerializeAdapterInfo{
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
extern \"C\" void gvox_input_adapter_${NAME}_begin(GVoxAdapterContext *ctx, void *config);
extern \"C\" void gvox_input_adapter_${NAME}_end(GVoxAdapterContext *ctx);
extern \"C\" void gvox_input_adapter_${NAME}_read(GVoxAdapterContext *ctx, size_t position, size_t size, void *data);
")
endforeach()
foreach(NAME ${GVOX_OUTPUT_ADAPTERS})
string(MAKE_C_IDENTIFIER "${NAME}" NAME_UPPER)
string(TOUPPER "${NAME_UPPER}" NAME_UPPER)
target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_OUTPUT_ADAPTER_${NAME_UPPER}_BUILT_STATIC=1)
set(ADAPTERS_HEADER_CONTENT "${ADAPTERS_HEADER_CONTENT}
extern \"C\" void gvox_output_adapter_${NAME}_begin(GVoxAdapterContext *ctx, void *config);
extern \"C\" void gvox_output_adapter_${NAME}_end(GVoxAdapterContext *ctx);
extern \"C\" void gvox_output_adapter_${NAME}_write(GVoxAdapterContext *ctx, size_t position, size_t size, void const *data);
extern \"C\" void gvox_output_adapter_${NAME}_reserve(GVoxAdapterContext *ctx, size_t size);
")
endforeach()
foreach(NAME ${GVOX_PARSE_ADAPTERS})
string(MAKE_C_IDENTIFIER "${NAME}" NAME_UPPER)
string(TOUPPER "${NAME_UPPER}" NAME_UPPER)
target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_PARSE_ADAPTER_${NAME_UPPER}_BUILT_STATIC=1)
set(ADAPTERS_HEADER_CONTENT "${ADAPTERS_HEADER_CONTENT}
extern \"C\" void gvox_parse_adapter_${NAME}_begin(GVoxAdapterContext *ctx, void *config);
extern \"C\" void gvox_parse_adapter_${NAME}_end(GVoxAdapterContext *ctx);
extern \"C\" auto gvox_parse_adapter_${NAME}_query_region_flags(GVoxAdapterContext *ctx, GVoxRegionRange const *range, uint32_t channel_index) -> uint32_t;
extern \"C\" void gvox_parse_adapter_${NAME}_load_region(GVoxAdapterContext *ctx, GVoxOffset3D const *offset, uint32_t channel_index);
extern \"C\" auto gvox_parse_adapter_${NAME}_sample_data(GVoxAdapterContext *ctx, GVoxRegion const *region, GVoxOffset3D const *offset, uint32_t channel_index) -> uint32_t;
")
endforeach()
foreach(NAME ${GVOX_SERIALIZE_ADAPTERS})
string(MAKE_C_IDENTIFIER "${NAME}" NAME_UPPER)
string(TOUPPER "${NAME_UPPER}" NAME_UPPER)
target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_SERIALIZE_ADAPTER_${NAME_UPPER}_BUILT_STATIC=1)
set(ADAPTERS_HEADER_CONTENT "${ADAPTERS_HEADER_CONTENT}
extern \"C\" void gvox_serialize_adapter_${NAME}_begin(GVoxAdapterContext *ctx, void *config);
extern \"C\" void gvox_serialize_adapter_${NAME}_end(GVoxAdapterContext *ctx);
extern \"C\" void gvox_serialize_adapter_${NAME}_serialize_region(GVoxAdapterContext *ctx, GVoxRegionRange const *range);
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
