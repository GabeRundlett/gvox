set(VCPKG_ENV_PASSTHROUGH_UNTRACKED WASI_SDK_PATH PATH)

if(NOT DEFINED ENV{WASI_SDK_PATH})
    message(FATAL_ERROR "You must set the WASI_SDK_PATH env variable")
else()
    file(TO_CMAKE_PATH "$ENV{WASI_SDK_PATH}" WASI_SDK_PREFIX)
endif()

if(NOT EXISTS "${WASI_SDK_PREFIX}/share/cmake/wasi-sdk.cmake")
    message(FATAL_ERROR "wasi-sdk.cmake toolchain file not found")
endif()

set(VCPKG_TARGET_ARCHITECTURE wasm32)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME WasiSDK)
set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE "${WASI_SDK_PREFIX}/share/cmake/wasi-sdk.cmake")

set(VCPKG_C_FLAGS "-emit-llvm -D__wasi__=true -nostartfiles -fno-exceptions -Wl,--no-entry -Wl,--strip-all -Wl,--export-dynamic -Wl,--import-memory -fvisibility=hidden --target=wasm32-unknown-unknown")
set(VCPKG_CXX_FLAGS "-emit-llvm -D__wasi__=true -nostartfiles -fno-exceptions -Wl,--no-entry -Wl,--strip-all -Wl,--export-dynamic -Wl,--import-memory -fvisibility=hidden  -D_LIBCPP_HAS_NO_FILESYSTEM_LIBRARY=false --target=wasm32-unknown-unknown")
set(VCPKG_CMAKE_CONFIGURE_OPTIONS -DCMAKE_C_COMPILER_WORKS=1 -DCMAKE_CXX_COMPILER_WORKS=1 -DWASI_SDK_PREFIX=${WASI_SDK_PREFIX})
