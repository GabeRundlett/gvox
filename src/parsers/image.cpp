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

struct AABB {
    float min_x;
    float min_y;
    float max_x;
    float max_y;
};

struct ImageLayer {
    uint32_t size_x, size_y;
    std::vector<Pixel> pixels;
    GvoxTransform transform;
    GvoxTransform inv_transform;
    AABB bounds;

    ImageLayer(uint32_t size_x, uint32_t size_y, std::vector<Pixel> &&pixels, GvoxTransform const &transform)
        : size_x{size_x}, size_y{size_y}, pixels{std::move(pixels)}, transform{transform}, inv_transform{gvox_inverse_transform(&transform)} {
        auto p0 = GvoxTranslation{{0.0f, 0.0f, 0.0f, 0.0f}};
        auto p1 = GvoxTranslation{{static_cast<float>(size_x), 0.0f, 0.0f, 0.0f}};
        auto p2 = GvoxTranslation{{0.0f, static_cast<float>(size_y), 0.0f, 0.0f}};
        auto p3 = GvoxTranslation{{static_cast<float>(size_x), static_cast<float>(size_y), 0.0f, 0.0f}};
        p0 = gvox_apply_transform(&transform, p0);
        p1 = gvox_apply_transform(&transform, p1);
        p2 = gvox_apply_transform(&transform, p2);
        p3 = gvox_apply_transform(&transform, p3);
        bounds.min_x = std::min(std::min(p0.axis.x, p1.axis.x), std::min(p2.axis.x, p3.axis.x));
        bounds.min_y = std::min(std::min(p0.axis.y, p1.axis.y), std::min(p2.axis.y, p3.axis.y));
        bounds.max_x = std::max(std::max(p0.axis.x, p1.axis.x), std::max(p2.axis.x, p3.axis.x));
        bounds.max_y = std::max(std::max(p0.axis.y, p1.axis.y), std::max(p2.axis.y, p3.axis.y));
    }

    ImageLayer(ImageLayer const &) = delete;
    ImageLayer &operator=(ImageLayer const &) = delete;
    ImageLayer(ImageLayer &&) = default;
    ImageLayer &operator=(ImageLayer &&) = default;
};

struct GvoxImageParser {
    GvoxImageParserConfig config{};
    std::vector<ImageLayer> layers{};
    AABB bounds{};

    GvoxImageParser(GvoxImageParserConfig const &a_config) : config{a_config} {}

    auto parse(GvoxParseCbInfo const &info) -> GvoxResult;
    auto serialize() -> GvoxResult;

    ~GvoxImageParser() {
        std::cout.fill('0');
        for (int yi = floor(bounds.min_y); yi < ceil(bounds.max_y); ++yi) {
            for (int xi = floor(bounds.min_x); xi < ceil(bounds.max_x); ++xi) {
                auto r = 0u;
                auto g = 0u;
                auto b = 0u;
                for (auto const &layer : layers) {
                    auto pt = gvox_apply_transform(&layer.inv_transform, GvoxTranslation{{static_cast<float>(xi), static_cast<float>(yi), 0.0f, 0.0f}});
                    if (pt.axis.x >= 0 && pt.axis.x < layer.size_x &&
                        pt.axis.y >= 0 && pt.axis.y < layer.size_y) {
                        auto x = static_cast<uint32_t>(pt.axis.x);
                        auto y = static_cast<uint32_t>(pt.axis.y);
                        auto index = static_cast<size_t>(x + y * layer.size_x);
                        r = static_cast<uint32_t>(layer.pixels[index].r);
                        g = static_cast<uint32_t>(layer.pixels[index].g);
                        b = static_cast<uint32_t>(layer.pixels[index].b);
                        // r = static_cast<uint32_t>(pt.axis.x / layer.size_x * 255);
                        // g = static_cast<uint32_t>(pt.axis.y / layer.size_y * 255);
                    }
                }
                r = std::min(std::max(r, 0u), 255u);
                g = std::min(std::max(g, 0u), 255u);
                b = std::min(std::max(b, 0u), 255u);
                std::cout << "\033[48;2;" << std::setw(3) << r << ";" << std::setw(3) << g << ";" << std::setw(3) << b << "m  ";
            }
            std::cout << "\033[0m\n";
        }
        std::cout << "\033[0m" << std::flush;
    }
};

auto GvoxImageParser::parse(GvoxParseCbInfo const &info) -> GvoxResult {
    stbi_io_callbacks callbacks;
    callbacks.read = [](void *user, char *data, int size) -> int {
        auto input_stream = static_cast<GvoxInputStream>(user);
        gvox_input_read(input_stream, reinterpret_cast<uint8_t *>(data), static_cast<size_t>(size));
        return size;
    };
    callbacks.skip = [](void *user, int n) {
        auto input_stream = static_cast<GvoxInputStream>(user);
        gvox_input_seek(input_stream, n, GVOX_SEEK_ORIGIN_CUR);
    };
    callbacks.eof = [](void *) -> int {
        // No need to do anything
        return 0;
    };
    int size_x, size_y, comp;
    auto *data = stbi_load_from_callbacks(&callbacks, info.input_stream, &size_x, &size_y, &comp, 4);
    if (data == nullptr) {
        // Failed to load the texture
        return GVOX_ERROR_UNKNOWN;
    }

    layers.emplace_back(
        static_cast<uint32_t>(size_x),
        static_cast<uint32_t>(size_y),
        std::vector(reinterpret_cast<Pixel *>(data), reinterpret_cast<Pixel *>(data) + static_cast<size_t>(size_x * size_y)),
        info.transform);

    stbi_image_free(data);

    auto const &newest_layer = layers.back();
    if (layers.size() == 1) {
        bounds = newest_layer.bounds;
    } else {
        bounds.min_x = std::min(bounds.min_x, newest_layer.bounds.min_x);
        bounds.min_y = std::min(bounds.min_y, newest_layer.bounds.min_y);
        bounds.max_x = std::max(bounds.max_x, newest_layer.bounds.max_x);
        bounds.max_y = std::max(bounds.max_y, newest_layer.bounds.max_y);
    }

    return GVOX_SUCCESS;
}

auto GvoxImageParser::serialize() -> GvoxResult {
    return GVOX_SUCCESS;
}

auto gvox_parser_image_create(void **self, void const *config_ptr) -> GvoxResult {
    GvoxImageParserConfig config;
    if (config_ptr) {
        config = *static_cast<GvoxImageParserConfig const *>(config_ptr);
    } else {
        config = {};
    }
    *self = new GvoxImageParser(config);
    return GVOX_SUCCESS;
}
auto gvox_parser_image_parse(void *self, GvoxParseCbInfo const *info) -> GvoxResult {
    return static_cast<GvoxImageParser *>(self)->parse(*info);
}
auto gvox_parser_image_serialize(void *self) -> GvoxResult {
    return static_cast<GvoxImageParser *>(self)->serialize();
}
void gvox_parser_image_destroy(void *self) {
    delete static_cast<GvoxImageParser *>(self);
}
