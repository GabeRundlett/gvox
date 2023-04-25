# This repo is the home of the Gvox library

`.gvox` is a meta-format that allows for several voxel data structures to co-exist within a single file.

Gvox, the **G**eneral **vox**el format translation library, is what ties it all together, supplying parsing, conversion and serialization mechanisms to the consumer.

## Goals
 * Gvox should be accessible to everyone making a voxel engine, including but not limited to
   * People who are targeting the Web, using Rust or other WASM compatible languages
   * People using any language that has simple C bindability, such as ZIG, C++, Java and many more
 
   This is why the library uses a C99 interface, and I plan on creating bindings for as many projects that want it (Open an issue on GitHub, and we can work together!)

 * Gvox should be extremely extensible, so anyone can create their own formats that are then automatically convertible to any of the existing formats supported by GVOX!
 * Gvox should NOT have an internal intermediary data representation (such as a managed flat array of voxels), so that there is no wasted memory consumption during translation
 * Gvox should be multi-threadable
 * Gvox should allow for at least 2D, 3D, and 4D grids 

## Building
For now, you must have the following things installed to build the repository
 * A C++ compiler
 * CMake (3.21 or higher)
 * Ninja build
 * vcpkg (plus the VCPKG_ROOT environment variable)

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
