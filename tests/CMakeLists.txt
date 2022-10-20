
function(GVOX_CREATE_TEST)
    set(OPT_ARGS BRUH)
    set(SNG_ARGS BRUH2)
    set(MUL_ARGS FOLDER LIBS)
    cmake_parse_arguments(PARSE_ARGV 0 DCT "${OPT_ARGS}" "${SNG_ARGS}" "${MUL_ARGS}")

    list(JOIN DCT_FOLDER "_" FOLDER_NAME)
    list(JOIN DCT_FOLDER "/" FOLDER_PATH)

    add_executable(
        gvox_test_${FOLDER_NAME}
        "${FOLDER_PATH}/main.c"
    )
    target_link_libraries(gvox_test_${FOLDER_NAME} PRIVATE gvox::gvox ${DCT_LIBS})
    target_include_directories(gvox_test_${FOLDER_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
    target_compile_definitions(gvox_test_${FOLDER_NAME} PRIVATE GVOX_SAMPLE_PATH="${CMAKE_CURRENT_LIST_DIR}/${FOLDER_PATH}")

    if(GVOX_TESTS_DISABLE_WINDOWS_CONSOLE)
        if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
            target_link_options(gvox_test_${FOLDER_NAME}
                PRIVATE "-Wl,/ENTRY:mainCRTStartup,/SUBSYSTEM:WINDOWS"
            )
        endif()
    endif()

    foreach(FORMAT_NAME ${GVOX_FORMATS})
        add_custom_command(
            TARGET gvox_test_${FOLDER_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:gvox_format_${FORMAT_NAME}>
                $<TARGET_FILE_DIR:gvox_test_${FOLDER_NAME}>
        )
    endforeach()
endfunction()

GVOX_CREATE_TEST(
    FOLDER simple
    LIBS
)
