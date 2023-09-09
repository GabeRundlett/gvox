
find_package(Catch2 CONFIG REQUIRED)
find_package(daxa CONFIG REQUIRED)

include(CTest)
include(Catch)

function(GVOX_CREATE_TARGET)
    set(OPT_ARGS UNUSED)
    set(SNG_ARGS FILE LANG NAME_PREFIX OUTPUT_NAME)
    set(MUL_ARGS FOLDER LIBS)
    cmake_parse_arguments(PARSE_ARGV 0 GCT "${OPT_ARGS}" "${SNG_ARGS}" "${MUL_ARGS}")

    list(JOIN GCT_FOLDER "_" FOLDER_NAME)
    list(JOIN GCT_FOLDER "/" FOLDER_PATH)

    set(TGT ${GCT_NAME_PREFIX}_${FOLDER_NAME}_${GCT_FILE})
    set(${GCT_OUTPUT_NAME} ${TGT} PARENT_SCOPE)
    add_executable(${TGT} "${FOLDER_PATH}/${GCT_FILE}.${GCT_LANG}")
    target_link_libraries(${TGT} PRIVATE gvox::gvox ${GCT_LIBS})
    target_include_directories(${TGT} PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
    target_compile_definitions(${TGT} PRIVATE GVOX_SAMPLE_PATH="${CMAKE_CURRENT_LIST_DIR}/${FOLDER_PATH}")

    if(GVOX_TESTS_DISABLE_WINDOWS_CONSOLE)
        if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            target_link_options(${TGT}
                PRIVATE "-Wl,/ENTRY:mainCRTStartup,/SUBSYSTEM:WINDOWS"
            )
        endif()
    endif()

    set_project_sanitizers(${TGT})
endfunction()

function(GVOX_CREATE_SAMPLE)
    GVOX_CREATE_TARGET(
        NAME_PREFIX gvox_sample
        OUTPUT_NAME TGT_NAME
        ${ARGN}
    )
endfunction()

function(GVOX_CREATE_TEST)
    GVOX_CREATE_TARGET(
        NAME_PREFIX gvox_test
        OUTPUT_NAME TGT_NAME
        ${ARGN}
    )
    target_link_libraries(${TGT_NAME} PRIVATE
        Catch2::Catch2WithMain # Catch2::Catch2
    )
    catch_discover_tests(
        ${TGT_NAME}
        TEST_PREFIX
        "unittests."
        REPORTER
        XML
        OUTPUT_DIR
        .
        OUTPUT_PREFIX
        "unittests."
        OUTPUT_SUFFIX
        .xml
    )
endfunction()

GVOX_CREATE_SAMPLE(
    FOLDER simple
    FILE main
    LANG c
    LIBS
)

GVOX_CREATE_TEST(
    FOLDER simple
    FILE unittests
    LANG cpp
    LIBS
)

GVOX_CREATE_SAMPLE(
    FOLDER viewer
    FILE main
    LANG cpp
    LIBS daxa::daxa
)
