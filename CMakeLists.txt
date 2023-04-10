cmake_minimum_required(VERSION 3.21)

include(cmake/warnings.cmake)

if(GVOX_ENABLE_FILE_IO)
    list(APPEND VCPKG_MANIFEST_FEATURES "file-io")
endif()
if(GVOX_ENABLE_TESTS)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(gvox VERSION 1.2.6)

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

set(GVOX_INPUT_ADAPTERS
    "byte_buffer"
)
set(GVOX_OUTPUT_ADAPTERS
    "byte_buffer"
)
set(GVOX_PARSE_ADAPTERS
    "gvox_raw"
    "gvox_palette"
    "voxlap"
    "magicavoxel"
    "goxel"
)
set(GVOX_SERIALIZE_ADAPTERS
    "gvox_raw"
    "gvox_palette"
    "colored_text"
)

if(GVOX_BUILD_FOR_JAVA)
    set(BUILD_SHARED_LIBS ON)
endif()

add_library(${PROJECT_NAME}
    "src/gvox.cpp"
)

if(GVOX_ENABLE_THREADSAFETY)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GVOX_ENABLE_THREADSAFETY=1)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC GVOX_ENABLE_THREADSAFETY=0)
endif()

if(GVOX_BUILD_FOR_WEB)
    set(GVOX_ENABLE_MULTITHREADED_ADAPTERS false)
else()
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)
endif()

if(GVOX_BUILD_FOR_RUST)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_BUILD_FOR_RUST=1)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_BUILD_FOR_RUST=0)
endif()

if(GVOX_BUILD_FOR_ODIN)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_BUILD_FOR_ODIN=1)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_BUILD_FOR_ODIN=0)
endif()

if(GVOX_BUILD_FOR_JAVA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_BUILD_FOR_JAVA=1)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_BUILD_FOR_JAVA=0)
endif()

if(GVOX_ENABLE_MULTITHREADED_ADAPTERS)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GVOX_ENABLE_MULTITHREADED_ADAPTERS=1)
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC GVOX_ENABLE_MULTITHREADED_ADAPTERS=0)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_VERSION_MAJOR=${PROJECT_VERSION_MAJOR})
target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_VERSION_MINOR=${PROJECT_VERSION_MINOR})
target_compile_definitions(${PROJECT_NAME} PRIVATE GVOX_VERSION_PATCH=${PROJECT_VERSION_PATCH})

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

if(GVOX_ENABLE_FILE_IO)
    target_compile_definitions(${PROJECT_NAME} PUBLIC GVOX_ENABLE_FILE_IO=1)
    list(APPEND GVOX_INPUT_ADAPTERS "file")
    list(APPEND GVOX_OUTPUT_ADAPTERS "file" "stdout")
else()
    target_compile_definitions(${PROJECT_NAME} PUBLIC GVOX_ENABLE_FILE_IO=0)
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

include("cmake/adapters.cmake")

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
