#include <gvox/gvox.h>
#include <gvox/adapters/serialize/gvox_raw.h>

#include <cstdlib>

#include <bit>
#include <array>
#include <vector>

extern "C" void gvox_serialize_adapter_gvox_raw_create(GvoxAdapterContext *, void *) {
}

extern "C" void gvox_serialize_adapter_gvox_raw_destroy(GvoxAdapterContext *) {
}

extern "C" void gvox_serialize_adapter_gvox_raw_blit_begin(GvoxBlitContext *, GvoxAdapterContext *) {
}

extern "C" void gvox_serialize_adapter_gvox_raw_blit_end(GvoxBlitContext *, GvoxAdapterContext *) {
}

extern "C" void gvox_serialize_adapter_gvox_raw_serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *, GvoxRegionRange const *range, uint32_t channel_flags) {
    size_t offset = 0;
    auto magic = std::bit_cast<uint32_t>(std::array<char, 4>{'g', 'v', 'r', '\0'});
    gvox_output_write(blit_ctx, offset, sizeof(uint32_t), &magic);
    offset += sizeof(magic);
    gvox_output_write(blit_ctx, offset, sizeof(*range), range);
    offset += sizeof(*range);
    gvox_output_write(blit_ctx, offset, sizeof(channel_flags), &channel_flags);
    offset += sizeof(channel_flags);
    std::vector<uint8_t> channels;
    channels.resize(static_cast<size_t>(std::popcount(channel_flags)));
    uint32_t next_channel = 0;
    for (uint8_t channel_i = 0; channel_i < 32; ++channel_i) {
        if ((channel_flags & (1u << channel_i)) != 0) {
            channels[next_channel] = channel_i;
            ++next_channel;
        }
    }
    size_t const voxel_size = sizeof(uint32_t) * channels.size();
    auto temp_voxel = std::vector<uint32_t>{};
    temp_voxel.resize(channels.size());
    for (uint32_t zi = 0; zi < range->extent.z; ++zi) {
        for (uint32_t yi = 0; yi < range->extent.y; ++yi) {
            for (uint32_t xi = 0; xi < range->extent.x; ++xi) {
                auto const pos = GvoxOffset3D{
                    static_cast<int32_t>(xi) + range->offset.x,
                    static_cast<int32_t>(yi) + range->offset.y,
                    static_cast<int32_t>(zi) + range->offset.z,
                };
                for (uint32_t channel_i = 0; channel_i < channels.size(); ++channel_i) {
                    auto region = gvox_load_region(blit_ctx, &pos, channels[channel_i]);
                    temp_voxel[channel_i] = gvox_sample_region(blit_ctx, &region, &pos, channels[channel_i]);
                    gvox_unload_region(blit_ctx, &region);
                }
                gvox_output_write(blit_ctx, offset, voxel_size, temp_voxel.data());
                offset += voxel_size;
            }
        }
    }
}
