cmake_minimum_required(VERSION 3.21)

include(cmake/warnings.cmake)
if(GVOX_USE_VCPKG)
    include(cmake/vcpkg.cmake)
endif()

if(GVOX_FEATURE_IO_FILE)
    list(APPEND VCPKG_MANIFEST_FEATURES "io-file")
endif()
if(GVOX_FEATURE_IO_IMAGE)
    list(APPEND VCPKG_MANIFEST_FEATURES "io-image")
endif()
if(GVOX_FEATURE_IO_ADAPT_ZIP)
    list(APPEND VCPKG_MANIFEST_FEATURES "io-adapt-zip")
endif()
if(GVOX_ENABLE_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()
if(GVOX_ENABLE_TRACY)
    list(APPEND VCPKG_MANIFEST_FEATURES "tracy")
endif()

project(gvox VERSION 2.0.0)

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

if(GVOX_BUILD_FOR_JAVA)
    set(BUILD_SHARED_LIBS ON)
endif()

add_library(${PROJECT_NAME}
    "src/gvox.cpp"
    "src/stream.cpp"
    "src/format.cpp"
)

set(GVOX_INPUT_STREAMS
    "byte_buffer"
)

set(GVOX_OUTPUT_STREAMS
    # "byte_buffer"
)

set(GVOX_PARSERS
)

set(GVOX_SERIALIZERS
    "colored_text"
)

set(GVOX_CONTAINERS
    "raw"
    "bounded_raw"
)

target_compile_definitions(${PROJECT_NAME}
PUBLIC
    GVOX_FEATURE_IO_FILE=${GVOX_FEATURE_IO_FILE}
    GVOX_FEATURE_IO_IMAGE=${GVOX_FEATURE_IO_IMAGE}
    GVOX_FEATURE_IO_ADAPT_ZIP=${GVOX_FEATURE_IO_ADAPT_ZIP}
    GVOX_ENABLE_TRACY=${GVOX_ENABLE_TRACY}

    GVOX_BUILD_FOR_RUST=${GVOX_BUILD_FOR_RUST}
    GVOX_BUILD_FOR_ODIN=${GVOX_BUILD_FOR_ODIN}
    GVOX_BUILD_FOR_JAVA=${GVOX_BUILD_FOR_JAVA}

PRIVATE
    GVOX_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    GVOX_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    GVOX_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

if(GVOX_FEATURE_IO_FILE)
    list(APPEND GVOX_INPUT_STREAMS
        "file"
    )
    list(APPEND GVOX_OUTPUT_STREAMS
        # "file"
        "stdout"
    )
endif()

if(GVOX_FEATURE_IO_IMAGE)
    find_package(freeimage CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE freeimage::FreeImage freeimage::FreeImagePlus)

    function(FIXUP_TARGET TGT_NAME)
        if(CMAKE_CXX_COMPILER_ID MATCHES Clang)
            get_property(A_OPTIONS TARGET ${TGT_NAME} PROPERTY INTERFACE_COMPILE_OPTIONS)
            string(REPLACE "/EHsc" "" A_OPTIONS "")
            set_property(TARGET ${TGT_NAME} PROPERTY INTERFACE_COMPILE_OPTIONS "${A_OPTIONS}")
        endif()
    endfunction()

    FIXUP_TARGET(Imath::Imath)
    FIXUP_TARGET(OpenEXR::Iex)
    FIXUP_TARGET(OpenEXR::IlmThread)
    FIXUP_TARGET(OpenEXR::OpenEXRCore)
    FIXUP_TARGET(OpenEXR::OpenEXR)
    FIXUP_TARGET(OpenEXR::OpenEXRUtil)

    list(APPEND GVOX_PARSERS
        "image"
    )
endif()

if(GVOX_FEATURE_IO_ADAPT_ZIP)
    list(APPEND GVOX_INPUT_STREAMS
        "gzip"
    )
    find_package(ZLIB REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
    find_path(GZIP_HPP_INCLUDE_DIRS "gzip/compress.hpp")
    target_include_directories(${PROJECT_NAME} PRIVATE ${GZIP_HPP_INCLUDE_DIRS})
endif()
if(GVOX_ENABLE_TRACY)
    find_package(Tracy CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Tracy::TracyClient)
endif()

find_package(gcem CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE gcem)

find_package(EnTT CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE EnTT::EnTT)

include("cmake/gvox_standard_functions.cmake")

if(BUILD_SHARED_LIBS)
    if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        target_compile_definitions(${PROJECT_NAME}
            PRIVATE "GVOX_EXPORT=__declspec(dllexport)"
            INTERFACE "GVOX_EXPORT=__declspec(dllimport)")
    else()
        target_compile_definitions(${PROJECT_NAME}
            PUBLIC "GVOX_EXPORT=")
    endif()
else()
    target_compile_definitions(${PROJECT_NAME}
        PUBLIC "GVOX_EXPORT=")
endif()

if((GVOX_BUILD_FOR_RUST OR GVOX_BUILD_FOR_ODIN) AND GVOX_USE_STATIC_CRT)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_20)
include(GNUInstallDirs)
target_include_directories(${PROJECT_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
set_project_warnings(${PROJECT_NAME})

target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/arch:AVX2>
    $<$<CXX_COMPILER_ID:GNU,Clang>:-ftree-vectorize -march=native -mtune=native -mavx2 -O3>
)

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

if(GVOX_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

# Packaging
if(NOT GVOX_DISABLE_PACKAGING)
    include(cmake/packaging.cmake)
endif()
