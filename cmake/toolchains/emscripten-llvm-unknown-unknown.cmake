
include("$ENV{EMSDK}/upadapter/emscripten/cmake/Modules/Platform/Emscripten.cmake")

unset(CMAKE_C_FLAGS CACHE)
set(CMAKE_C_FLAGS "-emit-llvm" CACHE STRING "" FORCE)
unset(CMAKE_CXX_FLAGS CACHE)
set(CMAKE_CXX_FLAGS "-emit-llvm" CACHE STRING "" FORCE)
