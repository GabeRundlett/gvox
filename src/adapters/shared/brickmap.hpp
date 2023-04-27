#pragma once

#include <cstdlib>
#include <array>

struct Brick {
    std::array<uint32_t, 512> voxels;
};

union BrickmapHeader {
    struct Loaded {
        uint32_t heap_index : 31;
        uint32_t is_loaded : 1;
    } loaded;

    struct Unloaded {
        uint32_t lod_color : 24;
        uint32_t flags : 7;
        uint32_t is_loaded : 1;
    } unloaded;
};
