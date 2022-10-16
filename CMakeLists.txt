cmake_minimum_required(VERSION 3.21)

include(cmake/warnings.cmake)

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

add_library(${PROJECT_NAME} STATIC
    "src/gvox.c"
    "src/formats/gvox_simple.c"
    "src/formats/magicavoxel.c"
)

add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_compile_features(${PROJECT_NAME} PUBLIC c_std_99)

find_path(GLIB_INCLUDE "glib.h" PATH_SUFFIXES "glib-2.0")
find_library(GLIB_GLIB_2_LIBRARY "glib-2.0")
# find_library(GLIB_GIO_2_LIBRARY "gio-2.0")
# find_library(GLIB_GOBJECT_2_LIBRARY "gobject-2.0")
# find_library(GLIB_GMODULE_2_LIBRARY "gmodule-2.0")
# find_library(GLIB_GTHREAD_2_LIBRARY "gthread-2.0")

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    ${GLIB_GLIB_2_LIBRARY}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
    ${GLIB_INCLUDE}
    ${GLIB_INCLUDE}/../../lib/glib-2.0/include
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_project_warnings(${PROJECT_NAME})

if(GVOX_ENABLE_TESTS)
    add_subdirectory(tests)
endif()

# Packaging
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
file(WRITE ${CMAKE_BINARY_DIR}/config.cmake.in [=[
@PACKAGE_INIT@
include(${CMAKE_CURRENT_LIST_DIR}/${PROJECT_NAME}-targets.cmake)
check_required_components(${PROJECT_NAME})
]=])

configure_package_config_file(${CMAKE_BINARY_DIR}/config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
    NO_SET_AND_CHECK_MACRO)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)
install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION
    ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME})
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}-targets)
install(EXPORT ${PROJECT_NAME}-targets DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME} NAMESPACE ${PROJECT_NAME}::)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ TYPE INCLUDE)
