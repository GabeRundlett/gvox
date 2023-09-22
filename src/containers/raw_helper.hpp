#pragma once

#include "gvox/core.h"

#include <cstdint>
using Word = uint32_t;

struct Voxel {
    uint8_t *ptr{};
    uint32_t size{};
};

namespace {
    inline void fill_1d(uint8_t *&voxel_ptr, Voxel in_voxel, GvoxExtentMut voxel_range_extent) {
        for (uint32_t xi = 0; xi < voxel_range_extent.axis[0]; xi++) {
            for (uint32_t i = 0; i < in_voxel.size; i++) {
                *voxel_ptr++ = in_voxel.ptr[i];
            }
        }
    }
    inline void fill_2d(uint8_t *&voxel_ptr, Voxel in_voxel, GvoxExtentMut voxel_range_extent, GvoxOffsetMut voxel_next) {
        auto max_x = static_cast<uint32_t>(voxel_range_extent.axis[0]);
        auto max_y = static_cast<uint32_t>(voxel_range_extent.axis[1]);
        auto max_b = in_voxel.size;
        auto next_axis_0 = static_cast<size_t>(voxel_next.axis[0]);
        auto next_word_axis_0 = next_axis_0 / sizeof(Word);
        auto word_n = (max_b + sizeof(Word) - 1) / sizeof(Word);
        auto *in_word_ptr = reinterpret_cast<Word *>(in_voxel.ptr);
        auto *out_word_ptr = reinterpret_cast<Word *>(voxel_ptr);
        if (max_b == sizeof(Word)) {
            auto in_word = *in_word_ptr;
            for (uint32_t yi = 0; yi < max_y; yi++) {
                for (uint32_t xi = 0; xi < max_x; xi++) {
                    *out_word_ptr++ = in_word;
                }
                out_word_ptr += next_word_axis_0;
            }
        } else if ((max_b % sizeof(Word)) == 0) {
            for (uint32_t yi = 0; yi < max_y; yi++) {
                for (uint32_t vi = 0; vi < word_n; ++vi) {
                    auto *line_begin = out_word_ptr + vi;
                    auto in_word = in_word_ptr[vi];
                    for (uint32_t xi = 0; xi < max_x; xi++) {
                        *line_begin = in_word;
                        line_begin += word_n;
                    }
                }
                out_word_ptr += word_n * max_x;
                out_word_ptr += next_word_axis_0;
            }
        } else {
            for (uint32_t yi = 0; yi < max_y; yi++) {
                for (uint32_t xi = 0; xi < max_x; xi++) {
                    for (uint32_t i = 0; i < max_b; i++) {
                        *voxel_ptr++ = in_voxel.ptr[i];
                    }
                }
                voxel_ptr += next_axis_0;
            }
        }
    }
    inline void fill_3d(uint8_t *&voxel_ptr, Voxel in_voxel, GvoxExtentMut voxel_range_extent, GvoxOffsetMut voxel_next) {
        auto max_x = static_cast<uint32_t>(voxel_range_extent.axis[0]);
        auto max_y = static_cast<uint32_t>(voxel_range_extent.axis[1]);
        auto max_z = static_cast<uint32_t>(voxel_range_extent.axis[2]);
        auto max_b = in_voxel.size;
        auto next_axis_0 = static_cast<size_t>(voxel_next.axis[0]);
        auto next_axis_1 = static_cast<size_t>(voxel_next.axis[1]);
        auto next_word_axis_0 = next_axis_0 / sizeof(Word);
        auto next_word_axis_1 = next_axis_1 / sizeof(Word);
        auto word_n = (max_b + sizeof(Word) - 1) / sizeof(Word);
        auto *in_word_ptr = reinterpret_cast<Word *>(in_voxel.ptr);
        auto *out_word_ptr = reinterpret_cast<Word *>(voxel_ptr);
        if (max_b == sizeof(Word)) {
            auto in_word = *in_word_ptr;
            for (uint32_t zi = 0; zi < max_z; zi++) {
                for (uint32_t yi = 0; yi < max_y; yi++) {
                    for (uint32_t xi = 0; xi < max_x; xi++) {
                        *out_word_ptr++ = in_word;
                    }
                    out_word_ptr += next_word_axis_0;
                }
                out_word_ptr += next_word_axis_1;
            }
        } else if ((max_b % sizeof(Word)) == 0) {
            for (uint32_t zi = 0; zi < max_z; zi++) {
                for (uint32_t yi = 0; yi < max_y; yi++) {
                    for (uint32_t vi = 0; vi < word_n; ++vi) {
                        auto *line_begin = out_word_ptr + vi;
                        auto in_word = in_word_ptr[vi];
                        for (uint32_t xi = 0; xi < max_x; xi++) {
                            *line_begin = in_word;
                            line_begin += word_n;
                        }
                    }
                    out_word_ptr += word_n * max_x;
                    out_word_ptr += next_word_axis_0;
                }
                out_word_ptr += next_word_axis_1;
            }
        } else {
            for (uint32_t zi = 0; zi < max_z; zi++) {
                for (uint32_t yi = 0; yi < max_y; yi++) {
                    for (uint32_t xi = 0; xi < max_x; xi++) {
                        for (uint32_t i = 0; i < max_b; i++) {
                            *voxel_ptr++ = in_voxel.ptr[i];
                        }
                    }
                    voxel_ptr += next_axis_0;
                }
                voxel_ptr += next_axis_1;
            }
        }
    }
    inline void fill_4d(uint8_t *&voxel_ptr, Voxel in_voxel, GvoxExtentMut voxel_range_extent, GvoxOffsetMut voxel_next) {
        auto max_x = static_cast<uint32_t>(voxel_range_extent.axis[0]);
        auto max_y = static_cast<uint32_t>(voxel_range_extent.axis[1]);
        auto max_z = static_cast<uint32_t>(voxel_range_extent.axis[2]);
        auto max_w = static_cast<uint32_t>(voxel_range_extent.axis[3]);
        auto max_b = in_voxel.size;
        auto next_axis_0 = static_cast<size_t>(voxel_next.axis[0]);
        auto next_axis_1 = static_cast<size_t>(voxel_next.axis[1]);
        auto next_axis_2 = static_cast<size_t>(voxel_next.axis[2]);
        for (uint32_t wi = 0; wi < max_w; wi++) {
            for (uint32_t zi = 0; zi < max_z; zi++) {
                for (uint32_t yi = 0; yi < max_y; yi++) {
                    for (uint32_t xi = 0; xi < max_x; xi++) {
                        for (uint32_t i = 0; i < max_b; i++) {
                            *voxel_ptr++ = in_voxel.ptr[i];
                        }
                    }
                    voxel_ptr += next_axis_0;
                }
                voxel_ptr += next_axis_1;
            }
            voxel_ptr += next_axis_2;
        }
    }

    // Mustn't be called with depth < 4
    void fill_Nd_over_4(uint32_t depth, uint8_t *&voxel_ptr, Voxel in_voxel, GvoxExtentMut voxel_range_extent, GvoxOffsetMut voxel_next) {
        if (depth == 4) {
            fill_4d(voxel_ptr, in_voxel, voxel_range_extent, voxel_next);
        } else {
            auto max_v = static_cast<uint32_t>(voxel_range_extent.axis[depth - 1]);
            auto next_axis_v = static_cast<size_t>(voxel_next.axis[depth - 2]);
            for (uint32_t vi = 0; vi < max_v; vi++) {
                fill_Nd_over_4(depth - 1, voxel_ptr, in_voxel, voxel_range_extent, voxel_next);
                voxel_ptr += next_axis_v;
            }
        }
    }

    void fill_Nd(uint32_t dim, uint8_t *&voxel_ptr, Voxel in_voxel, GvoxExtentMut voxel_range_extent, GvoxOffsetMut voxel_next) {
        switch (dim) {
        case 1: fill_1d(voxel_ptr, in_voxel, voxel_range_extent); break;
        case 2: fill_2d(voxel_ptr, in_voxel, voxel_range_extent, voxel_next); break;
        case 3: fill_3d(voxel_ptr, in_voxel, voxel_range_extent, voxel_next); break;
        case 4: fill_4d(voxel_ptr, in_voxel, voxel_range_extent, voxel_next); break;
        default: fill_Nd_over_4(dim, voxel_ptr, in_voxel, voxel_range_extent, voxel_next); break;
        }
    }
} // namespace
