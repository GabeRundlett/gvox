#include <gvox/gvox.h>
#include <gvox/parsers/image.h>

#include <FreeImage.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <span>

struct Pixel {
    uint8_t b, g, r, a;
};

struct GvoxImageParser {
    GvoxImageParserConfig config{};
    uint32_t size_x{}, size_y{};
    std::vector<Pixel> pixels;

    explicit GvoxImageParser(GvoxImageParserConfig const &a_config) : config{a_config} {}

    auto load(GvoxInputStream input_stream) -> GvoxResult;

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

auto GvoxImageParser::load(GvoxInputStream input_stream) -> GvoxResult {
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
            return gvox_input_tell(static_cast<GvoxInputStream>(handle));
        },
    };

    FREE_IMAGE_FORMAT fi_format = FreeImage_GetFileTypeFromHandle(&io, static_cast<fi_handle>(input_stream), 0);
    if (fi_format == FREE_IMAGE_FORMAT::FIF_UNKNOWN) {
        return GVOX_ERROR_UNKNOWN;
    }
    FIBITMAP *fi_bitmap = FreeImage_LoadFromHandle(fi_format, &io, static_cast<fi_handle>(input_stream));
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
    return GVOX_SUCCESS;
}

auto gvox_parser_image_create(void **self, GvoxParserCreateCbArgs const *args) -> GvoxResult {
    GvoxImageParserConfig config;
    if (args->config != nullptr) {
        config = *static_cast<GvoxImageParserConfig const *>(args->config);
    } else {
        config = {};
    }
    *self = new GvoxImageParser(config);
    auto load_res = static_cast<GvoxImageParser *>(*self)->load(args->input_stream);
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
