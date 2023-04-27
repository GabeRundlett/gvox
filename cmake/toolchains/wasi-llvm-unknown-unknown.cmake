
file(TO_CMAKE_PATH "$ENV{WASI_SDK_PATH}" WASI_SDK_PREFIX)
include("$ENV{WASI_SDK_PATH}/share/cmake/wasi-sdk.cmake")

set(CMAKE_C_COMPILER_ID Clang)
set(CMAKE_CXX_COMPILER_ID Clang)

unset(CMAKE_C_FLAGS CACHE)
set(CMAKE_C_FLAGS "-emit-llvm -D__wasi__=true -fno-exceptions -fvisibility=hidden --target=wasm32-unknown-unknown" CACHE STRING "" FORCE)
unset(CMAKE_CXX_FLAGS CACHE)
set(CMAKE_CXX_FLAGS "-emit-llvm -D__wasi__=true -fno-exceptions -fvisibility=hidden  -D_LIBCPP_HAS_NO_FILESYSTEM_LIBRARY=false --target=wasm32-unknown-unknown" CACHE STRING "" FORCE)

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)
