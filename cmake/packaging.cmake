
include(CMakePackageConfigHelpers)
include(GNUInstallDirs)
file(WRITE ${CMAKE_BINARY_DIR}/config.cmake.in [=[
@PACKAGE_INIT@
include(${CMAKE_CURRENT_LIST_DIR}/gvox-targets.cmake)
check_required_components(gvox)

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
install(EXPORT gvox-targets DESTINATION ${CMAKE_INSTALL_DATADIR}/gvox NAMESPACE gvox::)
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/ TYPE INCLUDE)
