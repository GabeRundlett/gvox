#include <gvox/stream.h>
#include <gvox/parsers/image.h>

#include <FreeImage.h>

#include <iostream>
#include <iomanip>
#include <new>
#include <vector>
#include <span>

#include "../utils/tracy.hpp"

struct Pixel {
    uint8_t b, g, r, a;
};

struct GvoxImageParser {
    GvoxImageParserConfig config{};
    uint32_t size_x{}, size_y{};
    std::vector<Pixel> pixels;

    explicit GvoxImageParser(GvoxImageParserConfig const &a_config) : config{a_config} {}

    auto load(GvoxInputStream input_stream) -> GvoxResult;
};

namespace {
    inline auto create_io_and_get_voxel_desc(GvoxInputStream input_stream) -> std::pair<FreeImageIO, FREE_IMAGE_FORMAT> {
        auto io = FreeImageIO{
            .read_proc = [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned {
                gvox_input_read(static_cast<GvoxInputStream>(handle), reinterpret_cast<uint8_t *>(buffer), static_cast<size_t>(size * count));
                return size;
            },
            .write_proc = [](void *, unsigned, unsigned, fi_handle) -> unsigned {
                return 0;
            },
            .seek_proc = [](fi_handle handle, long offset, int origin) -> int {
                return gvox_input_seek(static_cast<GvoxInputStream>(handle), offset, static_cast<GvoxSeekOrigin>(origin));
            },
            .tell_proc = [](fi_handle handle) -> long {
                return static_cast<long>(gvox_input_tell(static_cast<GvoxInputStream>(handle)));
            },
        };
        auto fi_voxel_desc = FreeImage_GetFileTypeFromHandle(&io, static_cast<fi_handle>(input_stream), 0);
        return {io, fi_voxel_desc};
    }
} // namespace

auto GvoxImageParser::load(GvoxInputStream input_stream) -> GvoxResult {
    auto [io, fi_voxel_desc] = create_io_and_get_voxel_desc(input_stream);
    if (fi_voxel_desc == FREE_IMAGE_FORMAT::FIF_UNKNOWN) {
        return GVOX_ERROR_UNKNOWN;
    }
    FIBITMAP *fi_bitmap = FreeImage_LoadFromHandle(fi_voxel_desc, &io, static_cast<fi_handle>(input_stream));
    if (fi_bitmap == nullptr) {
        // Failed to load the image
        return GVOX_ERROR_UNKNOWN;
    }
    size_x = FreeImage_GetWidth(fi_bitmap);
    size_y = FreeImage_GetHeight(fi_bitmap);
    auto *data = FreeImage_GetBits(fi_bitmap);
    if (data == nullptr) {
        // Failed to load the image
        return GVOX_ERROR_UNKNOWN;
    }
    pixels = std::vector(reinterpret_cast<Pixel *>(data), reinterpret_cast<Pixel *>(data) + static_cast<size_t>(size_x * size_y));

    FreeImage_Unload(fi_bitmap);

    {
        std::cout.fill('0');
        for (uint32_t yi = 0; yi < size_y; ++yi) {
            for (uint32_t xi = 0; xi < size_x; ++xi) {
                auto index = static_cast<size_t>(xi + (size_y - 1 - yi) * size_x);
                auto r = static_cast<uint32_t>(pixels[index].r);
                auto g = static_cast<uint32_t>(pixels[index].g);
                auto b = static_cast<uint32_t>(pixels[index].b);
                r = std::min(std::max(r, 0u), 255u);
                g = std::min(std::max(g, 0u), 255u);
                b = std::min(std::max(b, 0u), 255u);
                std::cout << "\033[48;2;" << std::setw(3) << r << ";" << std::setw(3) << g << ";" << std::setw(3) << b << "m  ";
            }
            std::cout << "\033[0m\n";
        }
        std::cout << "\033[0m" << std::flush;
    }

    return GVOX_SUCCESS;
}

auto gvox_parser_image_description() GVOX_FUNC_ATTRIB->GvoxParserDescription {
    return GvoxParserDescription{
        .create = [](void **self, GvoxParserCreateCbArgs const *args) -> GvoxResult {
            GvoxImageParserConfig config;
            if (args->config != nullptr) {
                config = *static_cast<GvoxImageParserConfig const *>(args->config);
            } else {
                config = {};
            }
            *self = new (std::nothrow) GvoxImageParser(config);
            auto load_res = static_cast<GvoxImageParser *>(*self)->load(args->input_stream);
            if (load_res != GVOX_SUCCESS) {
                return load_res;
            }
            return GVOX_SUCCESS;
        },
        .destroy = [](void *self) -> void { delete static_cast<GvoxImageParser *>(self); },
        .create_from_input = [](GvoxInputStream input_stream, GvoxParser *user_parser) -> GvoxResult {
            auto [io, fi_voxel_desc] = create_io_and_get_voxel_desc(input_stream);
            if (fi_voxel_desc == FREE_IMAGE_FORMAT::FIF_UNKNOWN) {
                return GVOX_ERROR_UNPARSABLE_INPUT;
            }

            auto parser_ci = GvoxParserCreateInfo{};
            parser_ci.struct_type = GVOX_STRUCT_TYPE_PARSER_CREATE_INFO;
            parser_ci.next = nullptr;
            parser_ci.cb_args.config = nullptr;
            parser_ci.cb_args.input_stream = input_stream;
            parser_ci.description = gvox_parser_image_description();

            return gvox_create_parser(&parser_ci, user_parser);
        },
        .create_input_iterator = [](void *self_ptr, void **out_iterator_ptr) -> void {},
        .destroy_iterator = [](void *self_ptr, void *iterator_ptr) -> void {},
        .iterator_next = [](void *self_ptr, void **iterator_ptr, GvoxInputStream input_stream, GvoxIteratorValue *out) -> void {},
    };
}
