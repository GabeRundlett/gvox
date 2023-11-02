# Local-only code

set(SOURCE_PATH "${CMAKE_CURRENT_LIST_DIR}")

# Standard vcpkg stuff

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
    io-file WITH_IO_FILE
    io-image WITH_IO_IMAGE
    io-adapt-zip WITH_IO_ADAPT_ZIP
    container-openvdb WITH_CONTAINER_OPENVDB
    tracy WITH_TRACY
)

set(GVOX_DEFINES)

if(WITH_IO_FILE)
    list(APPEND GVOX_DEFINES "-DGVOX_FEATURE_IO_FILE=true")
endif()
if(WITH_IO_IMAGE)
    list(APPEND GVOX_DEFINES "-DGVOX_FEATURE_IO_IMAGE=true")
endif()
if(WITH_IO_ADAPT_ZIP)
    list(APPEND GVOX_DEFINES "-DGVOX_FEATURE_IO_ADAPT_ZIP=true")
endif()
if(WITH_IO_FILE)
    list(APPEND GVOX_DEFINES "-DGVOX_FEATURE_CONTAINER_OPENVDB=true")
endif()
if(WITH_TRACY)
    list(APPEND GVOX_DEFINES "-DGVOX_ENABLE_TRACY=true")
endif()

vcpkg_configure_cmake(
    SOURCE_PATH "${SOURCE_PATH}"
    PREFER_NINJA
    OPTIONS ${GVOX_DEFINES}
)

vcpkg_install_cmake()
vcpkg_copy_pdbs()
vcpkg_fixup_cmake_targets()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    RENAME copyright
)
