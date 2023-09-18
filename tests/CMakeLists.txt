
find_package(minifb CONFIG REQUIRED)

function(GVOX_CREATE_TARGET)
    set(OPT_ARGS WEB_COMPATIBLE)
    set(SNG_ARGS FILE LANG NAME_PREFIX OUTPUT_NAME)
    set(MUL_ARGS FOLDER LIBS)
    cmake_parse_arguments(PARSE_ARGV 0 GCT "${OPT_ARGS}" "${SNG_ARGS}" "${MUL_ARGS}")

    if(GVOX_BUILD_FOR_WEB AND NOT GCT_WEB_COMPATIBLE)
        return()
    endif()

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

    if(GVOX_BUILD_FOR_WEB AND EMSCRIPTEN)
        add_custom_target(${TGT}_web_assets
            COMMAND ${CMAKE_COMMAND}
            -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/${FOLDER_PATH}/${GCT_FILE}.html
            $<TARGET_FILE_DIR:${TGT}>/${TGT}.html
            COMMAND ${CMAKE_COMMAND}
            -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/main.css
            $<TARGET_FILE_DIR:${TGT}>
        )
        add_dependencies(${TGT} ${TGT}_web_assets)

        target_link_options(${TGT} PRIVATE
            "-sSTRICT=1"
            "-sENVIRONMENT=web"
            "-sLLD_REPORT_UNDEFINED"
            "-sMODULARIZE=1"
            "-sALLOW_MEMORY_GROWTH=1"
            "-sALLOW_TABLE_GROWTH"
            "-sMALLOC=emmalloc"
            "-sEXPORT_ALL=1"
            "-sEXPORTED_FUNCTIONS=[\"_malloc\",\"_free\",\"_main\"]"
            "-sASYNCIFY"
            "--no-entry"
            "-sEXPORT_NAME=${TGT}"
        )
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

GVOX_CREATE_SAMPLE(
    FOLDER simple
    FILE main
    LANG c
    LIBS
)

GVOX_CREATE_SAMPLE(
    FOLDER viewer
    FILE main
    LANG cpp
    LIBS minifb::minifb
    WEB_COMPATIBLE
)
