
function(GVOX_CREATE_TEST)
    set(OPT_ARGS BRUH)
    set(SNG_ARGS FILE LANG)
    set(MUL_ARGS FOLDER LIBS)
    cmake_parse_arguments(PARSE_ARGV 0 GCT "${OPT_ARGS}" "${SNG_ARGS}" "${MUL_ARGS}")

    list(JOIN GCT_FOLDER "_" FOLDER_NAME)
    list(JOIN GCT_FOLDER "/" FOLDER_PATH)

    set(TGT gvox_test_${FOLDER_NAME}_${GCT_FILE})
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

    foreach(FORMAT_NAME ${GVOX_FORMATS})
        add_custom_command(
            TARGET ${TGT}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                $<TARGET_FILE:gvox_format_${FORMAT_NAME}>
                $<TARGET_FILE_DIR:${TGT}>
        )
    endforeach()
    set_project_sanitizers(${TGT})
endfunction()

find_package(daxa CONFIG REQUIRED)

GVOX_CREATE_TEST(
    FOLDER simple
    FILE main
    LANG c
    LIBS
)
GVOX_CREATE_TEST(
    FOLDER simple
    FILE comparison
    LANG cpp
    LIBS daxa::daxa
)

set(DAXA_INCLUDE_DIR "$<TARGET_FILE_DIR:gvox>/../vcpkg_installed/x64-$<LOWER_CASE:$<PLATFORM_ID>>/include")
target_compile_definitions(${PROJECT_NAME}_test_simple_comparison PRIVATE DAXA_SHADER_INCLUDE_DIR="${DAXA_INCLUDE_DIR}")
