#include <gvox/gvox.h>
#include <gvox/adapters/serialize/colored_text.h>

#include <vector>
#include <array>
#include <new>

struct ColoredTextSerializeUserState {
    GvoxColoredTextSerializeAdapterConfig config;
};

extern "C" void gvox_serialize_adapter_colored_text_begin([[maybe_unused]] GvoxAdapterContext *ctx, [[maybe_unused]] void *config) {
    auto *user_state_ptr = gvox_adapter_malloc(ctx, sizeof(ColoredTextSerializeUserState));
    auto &user_state = *(new (user_state_ptr) ColoredTextSerializeUserState());
    gvox_serialize_adapter_set_user_pointer(ctx, user_state_ptr);

    if (config != nullptr) {
        user_state.config = *reinterpret_cast<GvoxColoredTextSerializeAdapterConfig *>(config);
    } else {
        user_state.config = {
            .downscale_factor = 1,
            .downscale_mode = GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_DOWNSCALE_MODE_NEAREST,
            .channel_id = GVOX_CHANNEL_ID_COLOR,
            .non_color_max_value = 0,
        };
    }
}

extern "C" void gvox_serialize_adapter_colored_text_end([[maybe_unused]] GvoxAdapterContext *ctx) {
}

extern "C" void gvox_serialize_adapter_colored_text_serialize_region(GvoxAdapterContext *ctx, GvoxRegionRange const *range, [[maybe_unused]] uint32_t channels) {
    auto &user_state = *reinterpret_cast<ColoredTextSerializeUserState *>(gvox_serialize_adapter_get_user_pointer(ctx));
    std::vector<char> data;
    constexpr auto pixel = std::to_array("\033[48;2;000;000;000m  ");
    constexpr auto line_terminator = std::to_array("\033[0m ");
    constexpr auto newline_terminator = std::to_array("\033[0m\n");
    data.resize(range->extent.x * range->extent.y * range->extent.z * (pixel.size() - 1) + range->extent.y * range->extent.z * (line_terminator.size() - 1) + range->extent.z * (newline_terminator.size() - 1));
    size_t output_index = 0;
    bool const is_3channel =
        (user_state.config.channel_id == GVOX_CHANNEL_ID_COLOR) ||
        (user_state.config.channel_id == GVOX_CHANNEL_ID_NORMAL);
    for (uint32_t zi = 0; zi < range->extent.z; zi += user_state.config.downscale_factor) {
        for (uint32_t yi = 0; yi < range->extent.y; yi += user_state.config.downscale_factor) {
            for (uint32_t xi = 0; xi < range->extent.x; xi += user_state.config.downscale_factor) {
                uint8_t r = 0;
                uint8_t g = 0;
                uint8_t b = 0;
                if (user_state.config.downscale_mode == GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_DOWNSCALE_MODE_LINEAR) {
                    float avg_r = 0.0f;
                    float avg_g = 0.0f;
                    float avg_b = 0.0f;
                    float sample_n = 0.0f;
                    uint32_t sub_n = user_state.config.downscale_factor;
                    GvoxRegionRange const sub_range = {
                        .offset = {
                            static_cast<int32_t>(xi) + range->offset.x,
                            static_cast<int32_t>(yi) + range->offset.y,
                            static_cast<int32_t>(zi) + range->offset.z,
                        },
                        .extent = {
                            user_state.config.downscale_factor,
                            user_state.config.downscale_factor,
                            user_state.config.downscale_factor,
                        },
                    };
                    if (gvox_query_region_flags(ctx, &sub_range, user_state.config.channel_id) != 0u) {
                        sub_n = 1;
                    }
                    for (uint32_t sub_zi = 0; sub_zi < sub_n && (zi + sub_zi < range->extent.z); ++sub_zi) {
                        for (uint32_t sub_yi = 0; sub_yi < sub_n && (yi + sub_yi < range->extent.y); ++sub_yi) {
                            for (uint32_t sub_xi = 0; sub_xi < sub_n && (xi + sub_xi < range->extent.x); ++sub_xi) {
                                auto const pos = GvoxOffset3D{
                                    static_cast<int32_t>(xi + sub_xi) + range->offset.x,
                                    static_cast<int32_t>(yi + sub_yi) + range->offset.y,
                                    static_cast<int32_t>(range->extent.z) + range->offset.z - static_cast<int32_t>(zi + sub_zi) - 1,
                                };
                                auto region = gvox_load_region(ctx, &pos, user_state.config.channel_id);
                                auto voxel = gvox_sample_region(ctx, &region, &pos, user_state.config.channel_id);
                                gvox_unload_region(ctx, &region);
                                if (is_3channel) {
                                    avg_r += static_cast<float>((voxel >> 0x00) & 0xff) * (1.0f / 255.0f);
                                    avg_g += static_cast<float>((voxel >> 0x08) & 0xff) * (1.0f / 255.0f);
                                    avg_b += static_cast<float>((voxel >> 0x10) & 0xff) * (1.0f / 255.0f);
                                } else {
                                    avg_r += static_cast<float>(voxel) / static_cast<float>(user_state.config.non_color_max_value);
                                }
                                sample_n += 1.0f;
                            }
                        }
                    }
                    if (!is_3channel) {
                        avg_g = avg_r;
                        avg_b = avg_r;
                    }
                    r = static_cast<uint8_t>(avg_r / sample_n * 255.0f);
                    g = static_cast<uint8_t>(avg_g / sample_n * 255.0f);
                    b = static_cast<uint8_t>(avg_b / sample_n * 255.0f);
                } else {
                    auto const pos = GvoxOffset3D{
                        static_cast<int32_t>(xi) + range->offset.x,
                        static_cast<int32_t>(yi) + range->offset.y,
                        static_cast<int32_t>(range->extent.z) + range->offset.z - static_cast<int32_t>(zi) - 1,
                    };
                    auto region = gvox_load_region(ctx, &pos, user_state.config.channel_id);
                    auto voxel = gvox_sample_region(ctx, &region, &pos, user_state.config.channel_id);
                    gvox_unload_region(ctx, &region);
                    if (is_3channel) {
                        r = (voxel >> 0x00) & 0xff;
                        g = (voxel >> 0x08) & 0xff;
                        b = (voxel >> 0x10) & 0xff;
                    } else {
                        r = static_cast<uint8_t>(static_cast<float>(voxel) * 255.0f / static_cast<float>(user_state.config.non_color_max_value));
                        g = r;
                        b = r;
                    }
                }
                auto next_pixel = pixel;
                next_pixel[7 + 0 * 4 + 0] = '0' + (r / 100);
                next_pixel[7 + 0 * 4 + 1] = '0' + (r % 100) / 10;
                next_pixel[7 + 0 * 4 + 2] = '0' + (r % 10);
                next_pixel[7 + 1 * 4 + 0] = '0' + (g / 100);
                next_pixel[7 + 1 * 4 + 1] = '0' + (g % 100) / 10;
                next_pixel[7 + 1 * 4 + 2] = '0' + (g % 10);
                next_pixel[7 + 2 * 4 + 0] = '0' + (b / 100);
                next_pixel[7 + 2 * 4 + 1] = '0' + (b % 100) / 10;
                next_pixel[7 + 2 * 4 + 2] = '0' + (b % 10);
                std::copy(next_pixel.begin(), next_pixel.end() - 1, data.data() + output_index);
                output_index += pixel.size() - 1;
            }
            std::copy(line_terminator.begin(), line_terminator.end() - 1, data.data() + output_index);
            output_index += line_terminator.size() - 1;
        }
        std::copy(newline_terminator.begin(), newline_terminator.end() - 1, data.data() + output_index);
        output_index += newline_terminator.size() - 1;
    }
    gvox_output_write(ctx, 0, output_index, data.data());
}
