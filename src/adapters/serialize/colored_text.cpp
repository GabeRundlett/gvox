#include <gvox/gvox.h>
#include <gvox/adapters/serialize/colored_text.h>

#include <cstdlib>

#include <bit>
#include <vector>
#include <array>
#include <new>

struct ColoredTextSerializeUserState {
    GvoxColoredTextSerializeAdapterConfig config{};
    GvoxRegionRange range{};
    std::array<char, 6> line_terminator{};
    std::vector<char> data;
    std::vector<uint8_t> channels;
};

static constexpr auto pixel = std::to_array("\033[48;2;000;000;000m  ");
static constexpr auto newline_terminator = std::to_array("\033[0m\n");
static constexpr auto channel_terminator = std::to_array("\n\n");

static constexpr auto pixel_stride = pixel.size() - 1;

// Base
extern "C" void gvox_serialize_adapter_colored_text_create(GvoxAdapterContext *ctx, void const *config) {
    auto *user_state_ptr = malloc(sizeof(ColoredTextSerializeUserState));
    auto &user_state = *(new (user_state_ptr) ColoredTextSerializeUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
    if (config != nullptr) {
        user_state.config = *static_cast<GvoxColoredTextSerializeAdapterConfig const *>(config);
        user_state.config.downscale_factor = std::max(user_state.config.downscale_factor, 1u);
    } else {
        user_state.config = {
            .downscale_factor = 1,
            .downscale_mode = GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_DOWNSCALE_MODE_NEAREST,
            .non_color_max_value = 0,
            .vertical = 0,
        };
    }
}

extern "C" void gvox_serialize_adapter_colored_text_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<ColoredTextSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~ColoredTextSerializeUserState();
    free(&user_state);
}

extern "C" void gvox_serialize_adapter_colored_text_blit_begin(GvoxBlitContext * /*unused*/, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<ColoredTextSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));

    user_state.range = *range;
    user_state.channels.resize(static_cast<size_t>(std::popcount(channel_flags)));
    uint32_t next_channel = 0;
    for (uint8_t channel_i = 0; channel_i < 32; ++channel_i) {
        if ((channel_flags & (1u << channel_i)) != 0) {
            user_state.channels[next_channel] = channel_i;
            ++next_channel;
        }
    }

    user_state.line_terminator = std::to_array("\033[0m ");
    if (user_state.config.vertical != 0u) {
        user_state.line_terminator.at(user_state.line_terminator.size() - 2) = '\n';
    }
    user_state.data.resize(
        static_cast<size_t>(range->extent.x) * range->extent.y * range->extent.z * user_state.channels.size() * (pixel.size() - 1) +
        static_cast<size_t>(range->extent.y) * range->extent.z * user_state.channels.size() * (user_state.line_terminator.size() - 1) +
        static_cast<size_t>(range->extent.z) * user_state.channels.size() * (newline_terminator.size() - 1) +
        user_state.channels.size() * (channel_terminator.size() - 1));

    size_t output_index = 0;
    for ([[maybe_unused]] auto channel_id : user_state.channels) {
        for (uint32_t zi = 0; zi < user_state.range.extent.z; zi += user_state.config.downscale_factor) {
            for (uint32_t yi = 0; yi < user_state.range.extent.y; yi += user_state.config.downscale_factor) {
                for (uint32_t xi = 0; xi < range->extent.x; xi += user_state.config.downscale_factor) {
                    std::copy(pixel.begin(), pixel.end() - 1, user_state.data.data() + output_index);
                    output_index += pixel.size() - 1;
                }
                std::copy(user_state.line_terminator.begin(), user_state.line_terminator.end() - 1, user_state.data.data() + output_index);
                output_index += user_state.line_terminator.size() - 1;
            }
            std::copy(newline_terminator.begin(), newline_terminator.end() - 1, user_state.data.data() + output_index);
            output_index += newline_terminator.size() - 1;
        }
        std::copy(channel_terminator.begin(), channel_terminator.end() - 1, user_state.data.data() + output_index);
        output_index += channel_terminator.size() - 1;
    }
}

extern "C" void gvox_serialize_adapter_colored_text_blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<ColoredTextSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    gvox_output_write(blit_ctx, 0, user_state.data.size(), user_state.data.data());
}

namespace {
    void handle_region(ColoredTextSerializeUserState &user_state, GvoxRegionRange const *range, auto user_func) {
        for (uint32_t channel_i = 0; channel_i < user_state.channels.size(); ++channel_i) {
            auto channel_id = user_state.channels[channel_i];
            bool const is_3channel =
                (channel_id == GVOX_CHANNEL_ID_COLOR) ||
                (channel_id == GVOX_CHANNEL_ID_EMISSIVITY) ||
                (channel_id == GVOX_CHANNEL_ID_NORMAL);
            bool const is_normalized_float =
                (channel_i == GVOX_CHANNEL_ID_ROUGHNESS) ||
                (channel_i == GVOX_CHANNEL_ID_METALNESS) ||
                (channel_i == GVOX_CHANNEL_ID_TRANSPARENCY) ||
                (channel_i == GVOX_CHANNEL_ID_REFLECTIVITY) ||
                (channel_id == GVOX_CHANNEL_ID_DENSITY) ||
                (channel_id == GVOX_CHANNEL_ID_PHASE);
            float const normalized_range_min = channel_i == GVOX_CHANNEL_ID_PHASE ? -0.9f : 0.0f;
            float const normalized_range_max = channel_i == GVOX_CHANNEL_ID_PHASE ? 0.9f : channel_i == GVOX_CHANNEL_ID_REFLECTIVITY ? 2.0f : 1.0f;
            for (uint32_t zi = 0; zi < range->extent.z; zi += user_state.config.downscale_factor) {
                for (uint32_t yi = 0; yi < range->extent.y; yi += user_state.config.downscale_factor) {
                    for (uint32_t xi = 0; xi < range->extent.x; xi += user_state.config.downscale_factor) {
                        auto const out_pos = GvoxOffset3D{
                            static_cast<int32_t>(xi) + range->offset.x,
                            static_cast<int32_t>(yi) + range->offset.y,
                            static_cast<int32_t>(zi) + range->offset.z,
                        };
                        if (out_pos.x < user_state.range.offset.x ||
                            out_pos.y < user_state.range.offset.y ||
                            out_pos.z < user_state.range.offset.z ||
                            out_pos.x >= user_state.range.offset.x + static_cast<int32_t>(user_state.range.extent.x) ||
                            out_pos.y >= user_state.range.offset.y + static_cast<int32_t>(user_state.range.extent.y) ||
                            out_pos.z >= user_state.range.offset.z + static_cast<int32_t>(user_state.range.extent.z)) {
                            continue;
                        }
                        auto output_rel_x = static_cast<size_t>(out_pos.x - user_state.range.offset.x);
                        auto output_rel_y = static_cast<size_t>(out_pos.y - user_state.range.offset.y);
                        auto output_rel_z = static_cast<size_t>(out_pos.z - user_state.range.offset.z);
                        if (user_state.config.vertical != 0u) {
                            std::swap(output_rel_y, output_rel_z);
                        }
                        auto line_stride = pixel_stride * user_state.range.extent.x + user_state.line_terminator.size() - 1;
                        auto newline_stride = line_stride * user_state.range.extent.y + newline_terminator.size() - 1;
                        auto channel_stride = newline_stride * user_state.range.extent.z + channel_terminator.size() - 1;
                        auto output_index =
                            output_rel_x * pixel_stride +
                            output_rel_y * line_stride +
                            output_rel_z * newline_stride +
                            channel_i * channel_stride;
                        uint8_t r = 0;
                        uint8_t g = 0;
                        uint8_t b = 0;
                        if (user_state.config.downscale_mode == GVOX_COLORED_TEXT_SERIALIZE_ADAPTER_DOWNSCALE_MODE_LINEAR) {
                            float avg_r = 0.0f;
                            float avg_g = 0.0f;
                            float avg_b = 0.0f;
                            float sample_n = 0.0f;
                            uint32_t const sub_n = user_state.config.downscale_factor;
                            for (uint32_t sub_zi = 0; sub_zi < sub_n && (zi + sub_zi < range->extent.z); ++sub_zi) {
                                for (uint32_t sub_yi = 0; sub_yi < sub_n && (yi + sub_yi < range->extent.y); ++sub_yi) {
                                    for (uint32_t sub_xi = 0; sub_xi < sub_n && (xi + sub_xi < range->extent.x); ++sub_xi) {
                                        auto const pos = GvoxOffset3D{
                                            static_cast<int32_t>(xi + sub_xi) + range->offset.x,
                                            static_cast<int32_t>(range->extent.y) + range->offset.y - static_cast<int32_t>(yi + sub_yi) - 1,
                                            static_cast<int32_t>(range->extent.z) + range->offset.z - static_cast<int32_t>(zi + sub_zi) - 1,
                                        };
                                        auto sample = user_func(channel_id, pos);
                                        auto voxel = sample.data;
                                        if (is_3channel) {
                                            avg_r += static_cast<float>((voxel >> 0x00) & 0xff) * (1.0f / 255.0f);
                                            avg_g += static_cast<float>((voxel >> 0x08) & 0xff) * (1.0f / 255.0f);
                                            avg_b += static_cast<float>((voxel >> 0x10) & 0xff) * (1.0f / 255.0f);
                                        } else if (is_normalized_float) {
                                            avg_r += std::bit_cast<float>(voxel);
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
                                static_cast<int32_t>(range->extent.y) + range->offset.y - static_cast<int32_t>(yi) - 1,
                                static_cast<int32_t>(range->extent.z) + range->offset.z - static_cast<int32_t>(zi) - 1,
                            };
                            auto sample = user_func(channel_id, pos);
                            auto voxel = sample.data;
                            if (is_3channel) {
                                r = (voxel >> 0x00) & 0xff;
                                g = (voxel >> 0x08) & 0xff;
                                b = (voxel >> 0x10) & 0xff;
                            } else if (is_normalized_float) {
                                r = static_cast<uint8_t>(std::min(std::max((std::bit_cast<float>(voxel) - normalized_range_min) / (normalized_range_max - normalized_range_min), 0.0f), 1.0f) * 255.0f);
                                g = r;
                                b = r;
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
                        std::copy(next_pixel.begin(), next_pixel.end() - 1, user_state.data.data() + output_index);
                    }
                }
            }
        }
    }
} // namespace

// Serialize Driven
extern "C" void gvox_serialize_adapter_colored_text_serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t /* channel_flags */) {
    auto &user_state = *static_cast<ColoredTextSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    handle_region(
        user_state, range,
        [blit_ctx](uint32_t channel_id, GvoxOffset3D const &pos) {
            auto const sample_range = GvoxRegionRange{
                .offset = pos,
                .extent = GvoxExtent3D{1, 1, 1},
            };
            auto region = gvox_load_region_range(blit_ctx, &sample_range, 1u << channel_id);
            auto voxel = gvox_sample_region(blit_ctx, &region, &sample_range.offset, channel_id);
            gvox_unload_region_range(blit_ctx, &region, &sample_range);
            return voxel;
        });
}

// Parse Driven
extern "C" void gvox_serialize_adapter_colored_text_receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    auto &user_state = *static_cast<ColoredTextSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    handle_region(
        user_state, &region->range,
        [blit_ctx, region](uint32_t channel_id, GvoxOffset3D const &pos) {
            auto const sample_range = GvoxRegionRange{
                .offset = pos,
                .extent = GvoxExtent3D{1, 1, 1},
            };
            auto voxel = gvox_sample_region(blit_ctx, region, &sample_range.offset, channel_id);
            return voxel;
        });
}
