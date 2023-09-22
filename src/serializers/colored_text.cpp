#include <gvox/stream.h>

#include <gvox/serializers/colored_text.h>

#include <cstdlib>

#include <bit>
#include <vector>
#include <array>
#include <new>

#include <iostream>
#include <iomanip>

// static constexpr auto pixel = std::to_array("\033[48;2;000;000;000m  ");
// static constexpr auto newline_terminator = std::to_array("\033[0m\n");
// static constexpr auto channel_terminator = std::to_array("\n\n");
// static constexpr auto pixel_stride = pixel.size() - 1;

struct GvoxColoredTextSerializer {
    GvoxColoredTextSerializerConfig config{};
    std::array<char, 6> line_terminator{};
    std::vector<char> data;
    std::vector<uint8_t> channels;

    explicit GvoxColoredTextSerializer(GvoxColoredTextSerializerConfig const &a_config) : config{a_config} {}

    static auto blit_begin() -> GvoxResult;
    static auto blit_end() -> GvoxResult;
    static auto blit_range() -> GvoxResult;

    static auto request_regions() -> GvoxResult;
    static auto receive_region() -> GvoxResult;
};

auto GvoxColoredTextSerializer::blit_begin() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

auto GvoxColoredTextSerializer::blit_end() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

auto GvoxColoredTextSerializer::request_regions() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

auto GvoxColoredTextSerializer::receive_region() -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

auto gvox_serializer_colored_text_create(void **self, GvoxSerializerCreateCbArgs const *args) -> GvoxResult {
    GvoxColoredTextSerializerConfig config;
    if (args->config != nullptr) {
        config = *static_cast<GvoxColoredTextSerializerConfig const *>(args->config);
    } else {
        config = {};
    }
    *self = new GvoxColoredTextSerializer(config);
    return GVOX_SUCCESS;
}
void gvox_serializer_colored_text_destroy(void *self) { delete static_cast<GvoxColoredTextSerializer *>(self); }
auto gvox_serializer_colored_text_blit_begin(void *self) -> GvoxResult { return static_cast<GvoxColoredTextSerializer *>(self)->blit_begin(); }
auto gvox_serializer_colored_text_blit_end(void *self) -> GvoxResult { return static_cast<GvoxColoredTextSerializer *>(self)->blit_end(); }
auto gvox_serializer_colored_text_request_regions(void *self) -> GvoxResult { return static_cast<GvoxColoredTextSerializer *>(self)->request_regions(); }
auto gvox_serializer_colored_text_receive_region(void *self) -> GvoxResult { return static_cast<GvoxColoredTextSerializer *>(self)->receive_region(); }
