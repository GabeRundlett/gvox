#include <gvox/gvox.h>

#include <gvox/gvox.h>
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

    GvoxColoredTextSerializer(GvoxColoredTextSerializerConfig const &a_config);

    auto parse(GvoxParseCbInfo const &info) -> GvoxResult;
    auto serialize() -> GvoxResult;
};

GvoxColoredTextSerializer::GvoxColoredTextSerializer(GvoxColoredTextSerializerConfig const &a_config) : config{a_config} {
}

auto GvoxColoredTextSerializer::parse(GvoxParseCbInfo const & /*info*/) -> GvoxResult {
    return GVOX_ERROR_UNKNOWN;
}

auto GvoxColoredTextSerializer::serialize() -> GvoxResult {
    // std::cout.fill('0');
    // for (int yi = floor(bounds.min_y); yi < ceil(bounds.max_y); ++yi) {
    //     for (int xi = floor(bounds.min_x); xi < ceil(bounds.max_x); ++xi) {
    //         auto r = 0;
    //         auto g = 0;
    //         auto b = 0;
    //         for (auto const &layer : layers) {
    //             auto inv_transform = gvox_inverse_transform(&layer.transform);
    //             auto pt = gvox_apply_transform(&inv_transform, GvoxTranslation{static_cast<float>(xi), static_cast<float>(yi)});
    //             if (pt.axis.x >= 0 && pt.axis.x < layer.size_x &&
    //                 pt.axis.y >= 0 && pt.axis.y < layer.size_y) {
    //                 auto x = static_cast<uint32_t>(pt.axis.x);
    //                 auto y = static_cast<uint32_t>(pt.axis.y);
    //                 auto index = static_cast<size_t>(x + y * layer.size_x);
    //                 r = static_cast<uint32_t>(layer.pixels[index].r);
    //                 g = static_cast<uint32_t>(layer.pixels[index].g);
    //                 b = static_cast<uint32_t>(layer.pixels[index].b);
    //             }
    //         }
    //         r = std::min(std::max(r, 0), 255);
    //         g = std::min(std::max(g, 0), 255);
    //         b = std::min(std::max(b, 0), 255);
    //         std::cout << "\033[48;2;" << std::setw(3) << r << ";" << std::setw(3) << g << ";" << std::setw(3) << b << "m  ";
    //     }
    //     std::cout << "\033[0m\n";
    // }
    // std::cout << "\033[0m" << std::flush;
    return GVOX_ERROR_UNKNOWN;
}

auto gvox_serializer_colored_text_create(void **self, void const *config_ptr) -> GvoxResult {
    GvoxColoredTextSerializerConfig config;
    if (config_ptr) {
        config = *static_cast<GvoxColoredTextSerializerConfig const *>(config_ptr);
    } else {
        config = {};
    }
    *self = new GvoxColoredTextSerializer(config);
    return GVOX_SUCCESS;
}
auto gvox_serializer_colored_text_parse(void *self, GvoxParseCbInfo const *info) -> GvoxResult {
    return static_cast<GvoxColoredTextSerializer *>(self)->parse(*info);
}
auto gvox_serializer_colored_text_serialize(void *self) -> GvoxResult {
    return static_cast<GvoxColoredTextSerializer *>(self)->serialize();
}
void gvox_serializer_colored_text_destroy(void *self) {
    delete static_cast<GvoxColoredTextSerializer *>(self);
}

// // Serialize Driven
// extern "C" void gvox_container_colored_text_blit_translate_regions(void *self, GvoxRegionRange const *range, uint32_t /* channel_flags */) {
//     auto &user_state = *static_cast<ColoredTextSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
//     handle_region(
//         user_state, range,
//         [blit_ctx](uint32_t channel_id, GvoxOffset3D const &pos) {
//             auto const sample_range = GvoxRegionRange{
//                 .offset = pos,
//                 .extent = GvoxExtent3D{1, 1, 1},
//             };
//             auto region = gvox_load_region_range(blit_ctx, &sample_range, 1u << channel_id);
//             auto voxel = gvox_sample_region(blit_ctx, &region, &sample_range.offset, channel_id);
//             gvox_unload_region_range(blit_ctx, &region, &sample_range);
//             return voxel;
//         });
// }

// // Parse Driven
// extern "C" void gvox_container_colored_text_blit_receive_region(void *self, GvoxRegion const *region) {
//     auto &user_state = *static_cast<ColoredTextSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
//     handle_region(
//         user_state, &region->range,
//         [blit_ctx, region](uint32_t channel_id, GvoxOffset3D const &pos) {
//             auto const sample_range = GvoxRegionRange{
//                 .offset = pos,
//                 .extent = GvoxExtent3D{1, 1, 1},
//             };
//             auto voxel = gvox_sample_region(blit_ctx, region, &sample_range.offset, channel_id);
//             return voxel;
//         });
// }
