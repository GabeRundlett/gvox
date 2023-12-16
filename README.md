# This repo is the home of the GVOX library

`.gvox` is a meta-format that allows for several voxel data structures to co-exist within a single file.

GVOX is the library that ties it all together, supplying parsing, conversion and serialization mechanisms to the consumer.
Gvox 

## Goals
 * I want GVOX to be accessible to everyone making a voxel engine, including but not limited to
   * People who are targeting the Web, using Rust or other WASM compatible languages
   * People using any language that has simple C bindability, such as ZIG, C++, (Java?) and many more
 
   This is why the library uses a C99 interface, and I plan on creating bindings for as many projects that want it (Open an issue on GitHub, and we can work together!)

 * I want GVOX to be extremely extensible, so anyone can create their own formats that are then automatically convertible to any of the existing formats supported by GVOX!
 
 * I want to create an asset importer and optimizer, that allows you to test certain rendering algorithms and storage algorithms

## Building
For now, you must have the following things installed to build the repository
 * A C++ compiler
 * CMake (3.21 or higher)
 * Ninja build
and optionally
 * vcpkg (plus the VCPKG_ROOT environment variable. If it doesn't exist, it's automatically downloaded)

Once you have these things installed, you should be able to build just by running these commands in the root directory of the repository

### Windows

```
cmake --preset=cl-x86_64-windows-msvc
cmake --build --preset=cl-x86_64-windows-msvc-debug
```

### Linux

```
cmake --preset=gcc-x86_64-linux-gnu
cmake --build --preset=gcc-x86_64-linux-gnu-debug
```

The plan in the future is to have language bindings accessible to as many languages I can
