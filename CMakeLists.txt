cmake_minimum_required(VERSION 3.21)

include(cmake/warnings.cmake)

project(gvox VERSION 0.0.1)

# find_package(ZLIB REQUIRED)

if(GVOX_ENABLE_STATIC_ANALYSIS)
    set(CPPCHECK_TEMPLATE "gcc")
    find_program(CPPCHECK cppcheck)
    find_program(CLANG_TIDY clang-tidy)
    if(CPPCHECK)
        set(CMAKE_CXX_CPPCHECK
            ${CPPCHECK}
            --template=${CPPCHECK_TEMPLATE}
            --enable=style,performance,warning,portability
            --inline-suppr
            --suppress=cppcheckError
            --suppress=internalAstError
            --suppress=unmatchedSuppression
            --suppress=preprocessorErrorDirective
            --suppress=exceptThrowInDestructor
            --suppress=functionStatic
            --inconclusive)
    endif()
    if(CLANG_TIDY)
        set(CMAKE_CXX_CLANG_TIDY
            ${CLANG_TIDY}
            --fix)
    endif()
endif()

add_library(${PROJECT_NAME} STATIC
    "src/gvox.cpp"
)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_DL_LIBS}
    # ZLIB::ZLIB
)
target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_project_warnings(${PROJECT_NAME})

set(GVOX_FORMATS
    magicavoxel
)

set(GVOX_STATIC_FORMATS
    gvox_simple
    gvox_u32
    gvox_u32_palette
)

set(static_formats_header_content "#pragma once

#include <array>
#include <string_view>

struct StaticFormatInfo {
    std::string_view name;
    GVoxFormatCreateContextFunc create_context;
    GVoxFormatDestroyContextFunc destroy_context;
    GVoxFormatCreatePayloadFunc create_payload;
    GVoxFormatDestroyPayloadFunc destroy_payload;
    GVoxFormatParsePayloadFunc parse_payload;
};
")
foreach(FORMAT_NAME ${GVOX_STATIC_FORMATS})
    target_sources(${PROJECT_NAME} PRIVATE "src/formats/${FORMAT_NAME}.cpp")
    string(MAKE_C_IDENTIFIER "${FORMAT_NAME}" FORMAT_NAME_UPPER)
    string(TOUPPER "${FORMAT_NAME_UPPER}" FORMAT_NAME_UPPER)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_FORMAT_${FORMAT_NAME_UPPER}_BUILT_STATIC=true)
    set(static_formats_header_content "${static_formats_header_content}
extern \"C\" auto gvox_format_${FORMAT_NAME}_create_context() -> void *;
extern \"C\" void gvox_format_${FORMAT_NAME}_destroy_context(void *context_ptr);
extern \"C\" auto gvox_format_${FORMAT_NAME}_create_payload(void *context_ptr, GVoxScene scene) -> GVoxPayload;
extern \"C\" void gvox_format_${FORMAT_NAME}_destroy_payload(void *context_ptr, GVoxPayload payload);
extern \"C\" auto gvox_format_${FORMAT_NAME}_parse_payload(void *context_ptr, GVoxPayload payload) -> GVoxScene;
")
    set(static_format_infos_content "${static_format_infos_content}
    StaticFormatInfo{
        .name = \"gvox_format_${FORMAT_NAME}\",
        .create_context = gvox_format_${FORMAT_NAME}_create_context,
        .destroy_context = gvox_format_${FORMAT_NAME}_destroy_context,
        .create_payload = gvox_format_${FORMAT_NAME}_create_payload,
        .destroy_payload = gvox_format_${FORMAT_NAME}_destroy_payload,
        .parse_payload = gvox_format_${FORMAT_NAME}_parse_payload,
    },")
endforeach()

set(static_formats_header_content "${static_formats_header_content}
static constexpr std::array static_format_infos = {${static_format_infos_content}
};
")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/static_formats.hpp" "${static_formats_header_content}")

function(set_project_sanitizers TARGET_NAME)
    set(SANITIZER_FLAGS
        # $<$<CXX_COMPILER_ID:MSVC>:/fsanitize=address>
        $<$<AND:$<CXX_COMPILER_ID:GNU,Clang>,$<NOT:$<PLATFORM_ID:Windows>>>:-fsanitize=address>
    )
    if(GVOX_ENABLE_ASAN)
        target_compile_options(${TARGET_NAME} PRIVATE ${SANITIZER_FLAGS})
        target_link_options(${TARGET_NAME} PRIVATE ${SANITIZER_FLAGS})
    endif()
endfunction()

set_project_sanitizers(${PROJECT_NAME})

foreach(FORMAT_NAME ${GVOX_FORMATS})
    add_library(gvox_format_${FORMAT_NAME} SHARED
        "src/formats/${FORMAT_NAME}.cpp"
    )
    target_link_libraries(gvox_format_${FORMAT_NAME} PRIVATE gvox::gvox)
    set_project_sanitizers(gvox_format_${FORMAT_NAME})
endforeach()

if(GVOX_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

# Packaging
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
file(WRITE ${CMAKE_BINARY_DIR}/config.cmake.in [=[
@PACKAGE_INIT@
include(${CMAKE_CURRENT_LIST_DIR}/gvox-targets.cmake)
check_required_components(gvox)
]=])

configure_package_config_file(${CMAKE_BINARY_DIR}/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/gvox-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/gvox
    NO_SET_AND_CHECK_MACRO)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/gvox-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/gvox-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/gvox-config-version.cmake
    DESTINATION
    ${CMAKE_INSTALL_DATADIR}/gvox)
install(TARGETS gvox EXPORT gvox-targets)
foreach(FORMAT_NAME ${GVOX_FORMATS})
    install(TARGETS gvox_format_${FORMAT_NAME} EXPORT gvox-targets)
endforeach()
install(EXPORT gvox-targets DESTINATION ${CMAKE_INSTALL_DATADIR}/gvox NAMESPACE gvox::)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ TYPE INCLUDE)
