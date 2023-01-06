cmake_minimum_required(VERSION 3.21)

include(cmake/warnings.cmake)

if(GVOX_ENABLE_FILE_IO)
    list(APPEND VCPKG_MANIFEST_FEATURES "file-io")
endif()
if(GVOX_ENABLE_ZLIB)
    list(APPEND VCPKG_MANIFEST_FEATURES "zlib")
endif()
if(GVOX_ENABLE_GZIP)
    list(APPEND VCPKG_MANIFEST_FEATURES "gzip")
endif()
if(GVOX_ENABLE_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(gvox VERSION 0.0.1)

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

set(GVOX_DYNAMIC_FORMATS
)

set(GVOX_STATIC_FORMATS
    gvox_raw
    gvox_u32
    gvox_u32_palette
    ace_of_spades
    magicavoxel
)

add_library(${PROJECT_NAME} STATIC
    "src/gvox.cpp"
)

if(GVOX_ENABLE_FILE_IO)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GVOX_ENABLE_FILE_IO=1)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC GVOX_ENABLE_FILE_IO=0)
endif()
if(GVOX_ENABLE_ZLIB)
    list(APPEND GVOX_STATIC_FORMATS zlib)
    find_package(ZLIB REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
endif()
if(GVOX_ENABLE_GZIP)
    list(APPEND GVOX_STATIC_FORMATS gzip)
    find_path(GZIP_HPP_INCLUDE_DIRS "gzip/compress.hpp")
    target_include_directories(${PROJECT_NAME} PRIVATE ${GZIP_HPP_INCLUDE_DIRS})
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
target_link_libraries(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_DL_LIBS}
)
target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_project_warnings(${PROJECT_NAME})

set(GVOX_FORMATS ${GVOX_DYNAMIC_FORMATS} ${GVOX_STATIC_FORMATS})

foreach(FORMAT_NAME ${GVOX_FORMATS})
    set(format_names_content "${format_names_content}
    \"${FORMAT_NAME}\",")
endforeach()

set(formats_header_content "#pragma once

#include <array>
#include <string_view>

static constexpr std::array format_names = {${format_names_content}
};
")

set(formats_header_content "${formats_header_content}
#pragma region STATIC_FORMATS

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
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_FORMAT_${FORMAT_NAME_UPPER}_BUILT_STATIC=1)
    set(formats_header_content "${formats_header_content}
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

set(formats_header_content "${formats_header_content}
static constexpr std::array static_format_infos = {${static_format_infos_content}
};

#pragma endregion
")

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/formats.hpp" "${formats_header_content}")

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

# set_project_sanitizers(${PROJECT_NAME})

foreach(FORMAT_NAME ${GVOX_DYNAMIC_FORMATS})
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

set(GVOX_DYNAMIC_FORMATS @GVOX_DYNAMIC_FORMATS@)
set(GVOX_STATIC_FORMATS @GVOX_STATIC_FORMATS@)
set(GVOX_FORMATS @GVOX_FORMATS@)
]=])

if(GVOX_ENABLE_ZLIB)
    file(WRITE ${CMAKE_BINARY_DIR}/config.cmake.in [=[
find_package(ZLIB REQUIRED)
]=])
endif()
if(GVOX_ENABLE_GZIP)
    file(WRITE ${CMAKE_BINARY_DIR}/config.cmake.in [=[
find_path(GZIP_HPP_INCLUDE_DIRS "gzip/compress.hpp")
]=])
endif()

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
foreach(FORMAT_NAME ${GVOX_DYNAMIC_FORMATS})
    install(TARGETS gvox_format_${FORMAT_NAME} EXPORT gvox-targets)
endforeach()
install(EXPORT gvox-targets DESTINATION ${CMAKE_INSTALL_DATADIR}/gvox NAMESPACE gvox::)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ TYPE INCLUDE)
