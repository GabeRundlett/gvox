#include <gvox/adapter.h>
#include <gvox/parsers/image.h>

#include <FreeImage.h>

#include <iostream>
#include <iomanip>
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

    auto load(GvoxInputAdapter input_adapter) -> GvoxResult;

    static auto sample_region() -> GvoxResult;

    static auto blit_begin() -> GvoxResult;
    static auto blit_end() -> GvoxResult;
    static auto blit_query_region_flags() -> GvoxResult;
    static auto blit_load_region() -> GvoxResult;
    static auto blit_unload_region() -> GvoxResult;
    static auto blit_emit_regions_in_range() -> GvoxResult;
};

auto GvoxImageParser::sample_region() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

auto GvoxImageParser::blit_begin() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

auto GvoxImageParser::blit_end() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

// Serialize Driven
auto GvoxImageParser::blit_query_region_flags() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}
auto GvoxImageParser::blit_load_region() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}
auto GvoxImageParser::blit_unload_region() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

// Parse Driven
auto GvoxImageParser::blit_emit_regions_in_range() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

namespace {
    inline auto create_io_and_get_voxel_desc(GvoxInputAdapter input_adapter) -> std::pair<FreeImageIO, FREE_IMAGE_FORMAT> {
        ZoneScoped;
        auto io = FreeImageIO{
            .read_proc = [](void *buffer, unsigned size, unsigned count, fi_handle handle) -> unsigned {
                gvox_input_read(static_cast<GvoxInputAdapter>(handle), reinterpret_cast<uint8_t *>(buffer), static_cast<size_t>(size * count));
                return size;
            },
            .write_proc = [](void *, unsigned, unsigned, fi_handle) -> unsigned {
                return 0;
            },
            .seek_proc = [](fi_handle handle, long offset, int origin) -> int {
                return gvox_input_seek(static_cast<GvoxInputAdapter>(handle), offset, static_cast<GvoxSeekOrigin>(origin));
            },
            .tell_proc = [](fi_handle handle) -> long {
                return gvox_input_tell(static_cast<GvoxInputAdapter>(handle));
            },
        };
        auto fi_voxel_desc = FreeImage_GetFileTypeFromHandle(&io, static_cast<fi_handle>(input_adapter), 0);
        return {io, fi_voxel_desc};
    }
} // namespace

auto GvoxImageParser::load(GvoxInputAdapter input_adapter) -> GvoxResult {
    ZoneScoped;
    auto [io, fi_voxel_desc] = create_io_and_get_voxel_desc(input_adapter);
    if (fi_voxel_desc == FREE_IMAGE_FORMAT::FIF_UNKNOWN) {
        return GVOX_ERROR_UNKNOWN;
    }
    FIBITMAP *fi_bitmap = FreeImage_LoadFromHandle(fi_voxel_desc, &io, static_cast<fi_handle>(input_adapter));
    if (fi_bitmap == nullptr) {
        // Failed to load the image
        return GVOX_ERROR_UNKNOWN;
    }
    size_x = FreeImage_GetWidth(fi_bitmap);
    size_y = FreeImage_GetHeight(fi_bitmap);
    auto data = FreeImage_GetBits(fi_bitmap);
    if (data == nullptr) {
        // Failed to load the image
        return GVOX_ERROR_UNKNOWN;
    }
    pixels = std::vector(reinterpret_cast<Pixel *>(data), reinterpret_cast<Pixel *>(data) + static_cast<size_t>(size_x * size_y));

    FreeImage_Unload(fi_bitmap);

    {
        ZoneScoped;
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

auto gvox_parser_image_create(void **self, GvoxParserCreateCbArgs const *args) -> GvoxResult {
    ZoneScoped;
    GvoxImageParserConfig config;
    if (args->config != nullptr) {
        config = *static_cast<GvoxImageParserConfig const *>(args->config);
    } else {
        config = {};
    }
    *self = new GvoxImageParser(config);
    auto load_res = static_cast<GvoxImageParser *>(*self)->load(args->input_adapter);
    if (load_res != GVOX_SUCCESS) {
        return load_res;
    }
    return GVOX_SUCCESS;
}
void gvox_parser_image_destroy(void *self) { delete static_cast<GvoxImageParser *>(self); }

auto gvox_parser_image_sample_region(void *self) -> GvoxResult { return static_cast<GvoxImageParser *>(self)->sample_region(); }
auto gvox_parser_image_blit_begin(void *self) -> GvoxResult { return static_cast<GvoxImageParser *>(self)->blit_begin(); }
auto gvox_parser_image_blit_end(void *self) -> GvoxResult { return static_cast<GvoxImageParser *>(self)->blit_end(); }
auto gvox_parser_image_blit_query_region_flags(void *self) -> GvoxResult { return static_cast<GvoxImageParser *>(self)->blit_query_region_flags(); }
auto gvox_parser_image_blit_load_region(void *self) -> GvoxResult { return static_cast<GvoxImageParser *>(self)->blit_load_region(); }
auto gvox_parser_image_blit_unload_region(void *self) -> GvoxResult { return static_cast<GvoxImageParser *>(self)->blit_unload_region(); }
auto gvox_parser_image_blit_emit_regions_in_range(void *self) -> GvoxResult { return static_cast<GvoxImageParser *>(self)->blit_emit_regions_in_range(); }

auto gvox_parser_image_create_from_input(GvoxInputAdapter input_adapter, GvoxParser *user_parser) -> GvoxResult {
    ZoneScoped;
    auto [io, fi_voxel_desc] = create_io_and_get_voxel_desc(input_adapter);
    if (fi_voxel_desc == FREE_IMAGE_FORMAT::FIF_UNKNOWN) {
        return GVOX_ERROR_UNKNOWN;
    }

    auto parser_ci = GvoxParserCreateInfo{};
    parser_ci.struct_type = GVOX_STRUCT_TYPE_PARSER_CREATE_INFO;
    parser_ci.next = NULL;
    parser_ci.cb_args.config = NULL;
    parser_ci.cb_args.input_adapter = input_adapter;

    auto result = GVOX_SUCCESS;

    result = gvox_get_standard_parser_description("image", &parser_ci.description);
    if (result != GVOX_SUCCESS) { // Failed to find standard parser 'image' description
        return result;
    }

    return gvox_create_parser(&parser_ci, user_parser);
}
