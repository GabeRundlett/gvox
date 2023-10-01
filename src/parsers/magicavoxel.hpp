#pragma once

#include <gvox/gvox.h>
#include <bit>
#include <array>
#include <vector>
#include <string>
#include <variant>

#define MAGICAVOXEL_USE_WASM_FIX 1

#if MAGICAVOXEL_USE_WASM_FIX
#include <cstdlib>
#include <cstring>
#else
#include <sstream>
#endif

namespace magicavoxel {
    static constexpr uint32_t CHUNK_ID_VOX_ = std::bit_cast<uint32_t>(std::array{'V', 'O', 'X', ' '});
    static constexpr uint32_t CHUNK_ID_MAIN = std::bit_cast<uint32_t>(std::array{'M', 'A', 'I', 'N'});
    static constexpr uint32_t CHUNK_ID_SIZE = std::bit_cast<uint32_t>(std::array{'S', 'I', 'Z', 'E'});
    static constexpr uint32_t CHUNK_ID_XYZI = std::bit_cast<uint32_t>(std::array{'X', 'Y', 'Z', 'I'});
    static constexpr uint32_t CHUNK_ID_RGBA = std::bit_cast<uint32_t>(std::array{'R', 'G', 'B', 'A'});
    static constexpr uint32_t CHUNK_ID_nTRN = std::bit_cast<uint32_t>(std::array{'n', 'T', 'R', 'N'});
    static constexpr uint32_t CHUNK_ID_nGRP = std::bit_cast<uint32_t>(std::array{'n', 'G', 'R', 'P'});
    static constexpr uint32_t CHUNK_ID_nSHP = std::bit_cast<uint32_t>(std::array{'n', 'S', 'H', 'P'});
    static constexpr uint32_t CHUNK_ID_IMAP = std::bit_cast<uint32_t>(std::array{'I', 'M', 'A', 'P'});
    static constexpr uint32_t CHUNK_ID_LAYR = std::bit_cast<uint32_t>(std::array{'L', 'A', 'Y', 'R'});
    static constexpr uint32_t CHUNK_ID_MATL = std::bit_cast<uint32_t>(std::array{'M', 'A', 'T', 'L'});
    static constexpr uint32_t CHUNK_ID_MATT = std::bit_cast<uint32_t>(std::array{'M', 'A', 'T', 'T'});

    struct Color {
        uint8_t r, g, b, a;
    };
    using Palette = std::array<Color, 256>;

    struct Header {
        uint32_t file_header = 0;
        uint32_t file_version = 0;
    };

    struct ChunkHeader {
        uint32_t id = 0;
        uint32_t size = 0;
        uint32_t child_size = 0;
    };

    struct XyziModel {
        std::array<uint32_t, 3> extent{};
        uint32_t voxel_count{};
        int64_t input_offset{};

        [[nodiscard]] auto valid() const -> bool {
            return input_offset != 0;
        }
    };
} // namespace magicavoxel
