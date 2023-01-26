#include <gvox/adapter.h>
#include <gvox/adapters/serialize/colored_text.h>

#include <vector>
#include <array>

extern "C" void gvox_serialize_adapter_colored_text_begin([[maybe_unused]] GVoxAdapterContext *ctx, [[maybe_unused]] void *config) {
}

extern "C" void gvox_serialize_adapter_colored_text_end([[maybe_unused]] GVoxAdapterContext *ctx) {
}

extern "C" void gvox_serialize_adapter_colored_text_serialize_region(GVoxAdapterContext *ctx, GVoxRegionRange const *range) {
    std::vector<char> data;
    constexpr auto pixel = std::to_array("\033[48;2;000;000;000m  ");
    constexpr auto line_terminator = std::to_array("\033[0m ");
    constexpr auto newline_terminator = std::to_array("\033[0m\n");
    data.resize(range->extent.x * range->extent.y * range->extent.z * (pixel.size() - 1) + range->extent.y * range->extent.z * (line_terminator.size() - 1) + range->extent.z * (newline_terminator.size() - 1));
    size_t output_index = 0;
    for (uint32_t zi = 0; zi < range->extent.z; zi += 1) {
        for (uint32_t yi = 0; yi < range->extent.y; yi += 1) {
            for (uint32_t xi = 0; xi < range->extent.x; xi += 1) {
                GVoxOffset3D const pos = {
                    static_cast<int32_t>(xi + range->offset.x),
                    static_cast<int32_t>(yi + range->offset.y),
                    static_cast<int32_t>(range->extent.z + range->offset.z - zi - 1),
                };
                auto voxel = gvox_sample_data(ctx, &pos);
                uint8_t r = (voxel >> 0x00) & 0xff;
                uint8_t g = (voxel >> 0x08) & 0xff;
                uint8_t b = (voxel >> 0x10) & 0xff;
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
