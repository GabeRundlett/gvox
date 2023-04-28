#include <gvox/gvox.h>
#include <gvox/parsers/image.h>

#include <stb_image.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <span>

struct Pixel {
    uint8_t r, g, b, a;
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
    stbi_io_callbacks callbacks;
    callbacks.read = [](void *user, char *data, int size) -> int {
        auto *input_stream = static_cast<GvoxInputStream>(user);
        gvox_input_read(input_stream, reinterpret_cast<uint8_t *>(data), static_cast<size_t>(size));
        return size;
    };
    callbacks.skip = [](void *user, int n) {
        auto *input_stream = static_cast<GvoxInputStream>(user);
        gvox_input_seek(input_stream, n, GVOX_SEEK_ORIGIN_CUR);
    };
    callbacks.eof = [](void *) -> int {
        // No need to do anything
        return 0;
    };
    int read_size_x = 0;
    int read_size_y = 0;
    int read_comp = 0;
    auto *data = stbi_load_from_callbacks(&callbacks, input_stream, &read_size_x, &read_size_y, &read_comp, 4);
    if (data == nullptr) {
        // Failed to load the texture
        return GVOX_ERROR_UNKNOWN;
    }

    size_x = static_cast<uint32_t>(read_size_x);
    size_y = static_cast<uint32_t>(read_size_y);
    pixels = std::vector(reinterpret_cast<Pixel *>(data), reinterpret_cast<Pixel *>(data) + static_cast<size_t>(size_x * size_y));

    stbi_image_free(data);
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
