
function(GVOX_CREATE_TEST)
    set(OPT_ARGS UNUSED)
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

    set_project_sanitizers(${TGT})
endfunction()

add_library(procedural_parse_adapter "adapters/procedural.cpp")
target_include_directories(procedural_parse_adapter PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(procedural_parse_adapter PUBLIC gvox::gvox)

GVOX_CREATE_TEST(
    FOLDER simple
    FILE main
    LANG c
    LIBS procedural_parse_adapter
)
