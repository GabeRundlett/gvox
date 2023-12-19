#include <gvox/stream.h>

#include <gvox/serializers/colored_text.h>

#include <new>
#include <cstdint>
#include <vector>
#include <array>

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
};

auto gvox_serializer_colored_text_description() GVOX_FUNC_ATTRIB->GvoxSerializerDescription {
    return GvoxSerializerDescription{
        .create = [](void **self, GvoxSerializerCreateCbArgs const *args) -> GvoxResult {
            GvoxColoredTextSerializerConfig config;
            if (args->config != nullptr) {
                config = *static_cast<GvoxColoredTextSerializerConfig const *>(args->config);
            } else {
                config = {};
            }
            *self = new (std::nothrow) GvoxColoredTextSerializer(config);
            return GVOX_SUCCESS;
        },
        .destroy = [](void *self) { delete static_cast<GvoxColoredTextSerializer *>(self); },
    };
}
