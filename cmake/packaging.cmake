
include(CMakePackageConfigHelpers)
file(WRITE ${CMAKE_BINARY_DIR}/config.cmake.in [=[
@PACKAGE_INIT@
include(${CMAKE_CURRENT_LIST_DIR}/gvox-targets.cmake)
check_required_components(gvox)
]=])

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
install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/../include/ TYPE INCLUDE)
