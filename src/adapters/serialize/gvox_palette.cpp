#include <gvox/gvox.h>
#include <gvox/adapters/serialize/gvox_palette.h>

#include "../shared/gvox_palette.hpp"
#include "../shared/thread_pool.hpp"

#include <cstdlib>
#include <cstdint>

#include <bit>
#include <array>
#include <vector>
#include <new>
#include <unordered_set>
#include <algorithm>
#include <memory>
#include <mutex>

using namespace gvox_detail::thread_pool;
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
using namespace std::chrono_literals;
#endif

struct PaletteRegion {
    std::unordered_set<uint32_t> palette{};
    std::unique_ptr<std::array<std::pair<uint32_t, bool>, REGION_SIZE * REGION_SIZE * REGION_SIZE>> data{};
    uint32_t accounted_for{};
};

using PaletteRegionChannels = std::vector<PaletteRegion>;
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
using PaletteRegionChannelsMutexes = std::vector<std::mutex>;
#endif

struct GvoxPaletteSerializeUserState {
    GvoxRegionRange range{};
    size_t offset{};
    size_t blobs_begin{};
    size_t blob_size_offset{};
    std::vector<uint8_t> data{};
    std::vector<uint8_t> channels{};
    uint32_t region_nx{};
    uint32_t region_ny{};
    uint32_t region_nz{};
    std::vector<PaletteRegionChannels> palette_region_channels{};
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
    std::unique_ptr<PaletteRegionChannelsMutexes> palette_region_channels_mutexes{};
#endif
    ThreadPool thread_pool{};
};

template <typename T>
static void write_data(uint8_t *&buffer_ptr, T const &data) {
    *reinterpret_cast<T *>(buffer_ptr) = data;
    buffer_ptr += sizeof(T);
}

// Base
extern "C" void gvox_serialize_adapter_gvox_palette_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(GvoxPaletteSerializeUserState));
    new (user_state_ptr) GvoxPaletteSerializeUserState();
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_serialize_adapter_gvox_palette_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GvoxPaletteSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~GvoxPaletteSerializeUserState();
    free(&user_state);
}

extern "C" void gvox_serialize_adapter_gvox_palette_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<GvoxPaletteSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto magic = std::bit_cast<uint32_t>(std::array<char, 4>{'g', 'v', 'p', '\0'});
    auto channel_n = static_cast<uint32_t>(std::popcount(channel_flags));
    gvox_output_write(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(magic);
    gvox_output_write(blit_ctx, user_state.offset, sizeof(*range), range);
    user_state.offset += sizeof(*range);
    user_state.blob_size_offset = user_state.offset;
    user_state.offset += sizeof(uint32_t);
    gvox_output_write(blit_ctx, user_state.offset, sizeof(channel_flags), &channel_flags);
    user_state.offset += sizeof(channel_flags);
    gvox_output_write(blit_ctx, user_state.offset, sizeof(channel_n), &channel_n);
    user_state.offset += sizeof(channel_n);
    user_state.channels.resize(static_cast<size_t>(channel_n));
    uint32_t next_channel = 0;
    for (uint8_t channel_i = 0; channel_i < 32; ++channel_i) {
        if ((channel_flags & (1u << channel_i)) != 0) {
            user_state.channels[next_channel] = channel_i;
            ++next_channel;
        }
    }
    user_state.range = *range;
    user_state.region_nx = (range->extent.x + REGION_SIZE - 1) / REGION_SIZE;
    user_state.region_ny = (range->extent.y + REGION_SIZE - 1) / REGION_SIZE;
    user_state.region_nz = (range->extent.z + REGION_SIZE - 1) / REGION_SIZE;
    auto size = (sizeof(ChannelHeader) * user_state.channels.size()) * user_state.region_nx * user_state.region_ny * user_state.region_nz;
    user_state.blobs_begin = size;
    user_state.palette_region_channels.resize(user_state.region_nx * user_state.region_ny * user_state.region_nz);
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
    user_state.palette_region_channels_mutexes = std::make_unique<PaletteRegionChannelsMutexes>(user_state.region_nx * user_state.region_ny * user_state.region_nz * user_state.channels.size());
#endif
    user_state.data.resize(size);
}

extern "C" void gvox_serialize_adapter_gvox_palette_blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<GvoxPaletteSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto const two_percent_raw_size = static_cast<size_t>(user_state.range.extent.x * user_state.range.extent.y * user_state.range.extent.z) * sizeof(uint32_t) * user_state.channels.size() / 50;
    user_state.data.reserve(user_state.offset + two_percent_raw_size);
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
    std::mutex data_mutex;
#endif
    user_state.thread_pool.start();
    for (uint32_t rzi = 0; rzi < user_state.region_nz; ++rzi) {
        for (uint32_t ryi = 0; ryi < user_state.region_ny; ++ryi) {
            for (uint32_t rxi = 0; rxi < user_state.region_nx; ++rxi) {
                auto &palette_region_channel = user_state.palette_region_channels[rxi + ryi * user_state.region_nx + rzi * user_state.region_nx * user_state.region_ny];
                for (uint32_t ci = 0; ci < user_state.channels.size(); ++ci) {
                    auto region_header = ChannelHeader{.variant_n = 1u};
                    auto size = size_t{0};
                    auto local_data = std::vector<uint8_t>{};
                    auto alloc_region = [&]() {
                        {
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
                            auto lock = std::lock_guard{data_mutex};
#endif
                            auto const old_size = user_state.data.size();
                            region_header.blob_offset = static_cast<uint32_t>(old_size - user_state.blobs_begin);
                            user_state.data.resize(old_size + size);
                        }
                        local_data.resize(size);
                    };
                    if (palette_region_channel.size() == user_state.channels.size()) {
                        auto &palette_region = palette_region_channel.at(ci);
                        if (palette_region.accounted_for != 0 && palette_region.accounted_for != REGION_SIZE * REGION_SIZE * REGION_SIZE) {
                            palette_region.palette.insert(0u);
                            for (uint32_t zi = 0; zi < REGION_SIZE; ++zi) {
                                for (uint32_t yi = 0; yi < REGION_SIZE; ++yi) {
                                    for (uint32_t xi = 0; xi < REGION_SIZE; ++xi) {
                                        auto &[u32_voxel, is_present] = (*palette_region.data)[xi + yi * REGION_SIZE + zi * REGION_SIZE * REGION_SIZE];
                                        if (!is_present) {
                                            u32_voxel = 0u;
                                        }
                                    }
                                }
                            }
                        }
                        region_header.variant_n = static_cast<uint32_t>(palette_region.palette.size());
                        auto bits_per_variant = ceil_log2(region_header.variant_n);
                        if (region_header.variant_n > MAX_REGION_COMPRESSED_VARIANT_N) {
                            size = MAX_REGION_ALLOCATION_SIZE;
                            alloc_region();
                            uint8_t *output_buffer = local_data.data();
                            for (uint32_t zi = 0; zi < REGION_SIZE; ++zi) {
                                for (uint32_t yi = 0; yi < REGION_SIZE; ++yi) {
                                    for (uint32_t xi = 0; xi < REGION_SIZE; ++xi) {
                                        auto [u32_voxel, is_present] = (*palette_region.data)[xi + yi * REGION_SIZE + zi * REGION_SIZE * REGION_SIZE];
                                        write_data<uint32_t>(output_buffer, u32_voxel);
                                    }
                                }
                            }
                        } else if (region_header.variant_n > 1) {
                            // insert palette
                            size += sizeof(uint32_t) * region_header.variant_n;
                            // insert palette region
                            size += calc_palette_region_size(bits_per_variant);
                            alloc_region();
                            uint8_t *output_buffer = local_data.data();
                            auto *palette_begin = reinterpret_cast<uint32_t *>(output_buffer);
                            auto *palette_end = palette_begin + region_header.variant_n;
                            for (auto u32_voxel : palette_region.palette) {
                                write_data<uint32_t>(output_buffer, u32_voxel);
                            }
                            std::sort(palette_begin, palette_end);
                            for (uint32_t zi = 0; zi < REGION_SIZE; ++zi) {
                                for (uint32_t yi = 0; yi < REGION_SIZE; ++yi) {
                                    for (uint32_t xi = 0; xi < REGION_SIZE; ++xi) {
                                        auto const in_region_index = xi + yi * REGION_SIZE + zi * REGION_SIZE * REGION_SIZE;
                                        auto const [u32_voxel, is_present] = (*palette_region.data)[in_region_index];
                                        auto *iter = std::lower_bound(palette_begin, palette_end, u32_voxel);
                                        auto palette_id = static_cast<uint32_t>(iter - palette_begin);
                                        auto const bit_index = static_cast<size_t>(in_region_index) * bits_per_variant;
                                        auto const byte_index = bit_index / 8;
                                        auto const bit_offset = static_cast<uint32_t>(bit_index - byte_index * 8);
                                        auto const mask = get_mask(bits_per_variant);
                                        if (output_buffer + byte_index + 3 >= local_data.data() + local_data.size()) {
                                            gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "Trying to write past end of buffer, how did this happen?");
                                            return;
                                        }
#if 0
                                        // Note: This is technically UB, since I think it breaks the strict aliasing rules of C++.
                                        auto &output = *reinterpret_cast<uint32_t *>(output_buffer + byte_index);
                                        output = output & ~(mask << bit_offset);
                                        output = output | static_cast<uint32_t>(palette_id << bit_offset);
                                        // The "correct" solution is below.
#else
                                        auto prev_val = std::bit_cast<uint32_t>(*reinterpret_cast<std::array<uint8_t, 4> *>(output_buffer + byte_index));
                                        auto output = prev_val & ~(mask << bit_offset);
                                        output = output | static_cast<uint32_t>(palette_id << bit_offset);
                                        auto output_bytes = std::bit_cast<std::array<uint8_t, 4>>(output);
                                        std::copy(output_bytes.begin(), output_bytes.end(), output_buffer + byte_index);
#endif
                                    }
                                }
                            }
                        } else if (region_header.variant_n == 1) {
                            region_header.blob_offset = *palette_region.palette.begin();
                        } else {
                            region_header.blob_offset = 0;
                        }
                    } else {
                        region_header.blob_offset = 0;
                    }
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
                    auto lock = std::lock_guard{data_mutex};
#endif
                    auto *channel_header_ptr =
                        user_state.data.data() +
                        (rxi + ryi * user_state.region_nx + rzi * user_state.region_nx * user_state.region_ny) *
                            (sizeof(ChannelHeader) * user_state.channels.size()) +
                        (sizeof(ChannelHeader) * ci);
                    write_data<ChannelHeader>(channel_header_ptr, region_header);
                    if (region_header.variant_n > 1) {
                        std::copy(local_data.begin(), local_data.end(), user_state.data.data() + user_state.blobs_begin + region_header.blob_offset);
                    }
                }
            }
        }
    }
    while (user_state.thread_pool.busy()) {
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
        std::this_thread::sleep_for(10ms);
#endif
    }
    user_state.thread_pool.stop();
    auto blob_size = static_cast<uint32_t>(user_state.data.size() - user_state.blobs_begin);
    gvox_output_write(blit_ctx, user_state.blob_size_offset, sizeof(blob_size), &blob_size);
    gvox_output_write(blit_ctx, user_state.offset, user_state.data.size(), user_state.data.data());
}

static void handle_single_palette(
    GvoxBlitContext *blit_ctx, GvoxPaletteSerializeUserState &user_state, PaletteRegion &palette_region,
    GvoxRegion *region_ptr, uint32_t channel_id, uint32_t ox, uint32_t oy, uint32_t oz) {
    if (!palette_region.data) {
        palette_region.data = std::make_unique<decltype(PaletteRegion::data)::element_type>(decltype(PaletteRegion::data)::element_type{});
    }
    for (uint32_t zi = 0; zi < REGION_SIZE; ++zi) {
        for (uint32_t yi = 0; yi < REGION_SIZE; ++yi) {
            for (uint32_t xi = 0; xi < REGION_SIZE; ++xi) {
                auto palette_region_index = xi + yi * REGION_SIZE + zi * REGION_SIZE * REGION_SIZE;
                auto const px = ox + xi;
                auto const py = oy + yi;
                auto const pz = oz + zi;
                auto pos = GvoxOffset3D{
                    .x = static_cast<int32_t>(px) + user_state.range.offset.x,
                    .y = static_cast<int32_t>(py) + user_state.range.offset.y,
                    .z = static_cast<int32_t>(pz) + user_state.range.offset.z,
                };
                if (px < user_state.range.extent.x && py < user_state.range.extent.y && pz < user_state.range.extent.z) {
                    auto sample = gvox_sample_region(blit_ctx, region_ptr, &pos, channel_id);
                    if (sample.is_present != 0u) {
                        palette_region.palette.insert(sample.data);
                        auto const [prev_u32_voxel, prev_present] = (*palette_region.data)[palette_region_index];
                        if (!prev_present) {
                            (*palette_region.data)[palette_region_index] = {sample.data, sample.is_present};
                            ++palette_region.accounted_for;
                        }
                    }
                }
            }
        }
    }
    if (palette_region.accounted_for == 0) {
        palette_region.data.reset();
    }
}

static void handle_region(GvoxBlitContext *blit_ctx, GvoxPaletteSerializeUserState &user_state, GvoxRegionRange const *range, GvoxRegion *region_ptr) {
    auto temp_region = GvoxRegion{};
    if (region_ptr != nullptr) {
        temp_region = *region_ptr;
    }
    auto range_min = GvoxOffset3D{
        std::max(range->offset.x, user_state.range.offset.x),
        std::max(range->offset.y, user_state.range.offset.y),
        std::max(range->offset.z, user_state.range.offset.z),
    };
    auto range_max = GvoxOffset3D{
        std::min(std::max(range->offset.x + static_cast<int32_t>(range->extent.x), user_state.range.offset.x), user_state.range.offset.x + static_cast<int32_t>(user_state.range.extent.x)),
        std::min(std::max(range->offset.y + static_cast<int32_t>(range->extent.y), user_state.range.offset.y), user_state.range.offset.y + static_cast<int32_t>(user_state.range.extent.y)),
        std::min(std::max(range->offset.z + static_cast<int32_t>(range->extent.z), user_state.range.offset.z), user_state.range.offset.z + static_cast<int32_t>(user_state.range.extent.z)),
    };
    auto rx_min = static_cast<uint32_t>(range_min.x - user_state.range.offset.x) / static_cast<uint32_t>(REGION_SIZE);
    auto ry_min = static_cast<uint32_t>(range_min.y - user_state.range.offset.y) / static_cast<uint32_t>(REGION_SIZE);
    auto rz_min = static_cast<uint32_t>(range_min.z - user_state.range.offset.z) / static_cast<uint32_t>(REGION_SIZE);
    auto rx_max = static_cast<uint32_t>(range_max.x - user_state.range.offset.x + static_cast<int32_t>(REGION_SIZE) - 1) / static_cast<uint32_t>(REGION_SIZE);
    auto ry_max = static_cast<uint32_t>(range_max.y - user_state.range.offset.y + static_cast<int32_t>(REGION_SIZE) - 1) / static_cast<uint32_t>(REGION_SIZE);
    auto rz_max = static_cast<uint32_t>(range_max.z - user_state.range.offset.z + static_cast<int32_t>(REGION_SIZE) - 1) / static_cast<uint32_t>(REGION_SIZE);
    for (uint32_t rzi = rz_min; rzi < rz_max; ++rzi) {
        for (uint32_t ryi = ry_min; ryi < ry_max; ++ryi) {
            for (uint32_t rxi = rx_min; rxi < rx_max; ++rxi) {
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
                auto &mutex = (*user_state.palette_region_channels_mutexes)[rxi + ryi * user_state.region_nx + rzi * user_state.region_nx * user_state.region_ny];
                auto lock = std::lock_guard{mutex};
#endif
                auto &palette_region_channel = user_state.palette_region_channels[rxi + ryi * user_state.region_nx + rzi * user_state.region_nx * user_state.region_ny];
                palette_region_channel.resize(user_state.channels.size());
                for (uint32_t ci = 0; ci < palette_region_channel.size(); ++ci) {
                    auto &palette_region = palette_region_channel.at(ci);
                    auto const ox = rxi * static_cast<uint32_t>(REGION_SIZE);
                    auto const oy = ryi * static_cast<uint32_t>(REGION_SIZE);
                    auto const oz = rzi * static_cast<uint32_t>(REGION_SIZE);
                    auto channel_id = user_state.channels[ci];
                    auto const sample_range = GvoxRegionRange{
                        .offset = GvoxOffset3D{
                            .x = static_cast<int32_t>(ox) + user_state.range.offset.x,
                            .y = static_cast<int32_t>(oy) + user_state.range.offset.y,
                            .z = static_cast<int32_t>(oz) + user_state.range.offset.z,
                        },
                        .extent = GvoxExtent3D{REGION_SIZE, REGION_SIZE, REGION_SIZE},
                    };
                    if (region_ptr == nullptr) {
                        temp_region = gvox_load_region_range(blit_ctx, &sample_range, 1u << channel_id);
                    }
                    handle_single_palette(
                        blit_ctx, user_state, palette_region,
                        &temp_region, channel_id, ox, oy, oz);
                    if (region_ptr == nullptr) {
                        gvox_unload_region_range(blit_ctx, &temp_region, &sample_range);
                    }
                }
            }
        }
    }
}

// Serialize Driven
extern "C" void gvox_serialize_adapter_gvox_palette_serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t /*channel_flags*/) {
    auto &user_state = *static_cast<GvoxPaletteSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.thread_pool.start();
#define IMPL(axis)                                                                                                                                   \
    auto per_thread_size = range->extent.axis / 32;                                                                                                  \
    if (per_thread_size > 0) {                                                                                                                       \
        for (uint32_t i = 0; i < 32; ++i) {                                                                                                          \
            user_state.thread_pool.enqueue([blit_ctx, &user_state, range, per_thread_size, i]() {                                                    \
                auto temp_range = *range;                                                                                                            \
                temp_range.extent.axis = per_thread_size;                                                                                            \
                temp_range.offset.axis = range->offset.axis + static_cast<int32_t>(per_thread_size * i);                                             \
                handle_region(blit_ctx, user_state, &temp_range, nullptr);                                                                           \
            });                                                                                                                                      \
        }                                                                                                                                            \
    }                                                                                                                                                \
    auto temp_range = *range;                                                                                                                        \
    temp_range.extent.axis = range->extent.axis % 32;                                                                                                \
    temp_range.offset.axis = range->offset.axis + static_cast<int32_t>(user_state.range.extent.axis) - static_cast<int32_t>(temp_range.extent.axis); \
    if (range->extent.axis > 0) {                                                                                                                    \
        handle_region(blit_ctx, user_state, &temp_range, nullptr);                                                                                   \
    }
    auto max_axis = std::max({range->extent.x, range->extent.y, range->extent.z});
    if (max_axis == range->extent.x) {
        IMPL(x)
    } else if (max_axis == range->extent.y) {
        IMPL(y)
    } else {
        IMPL(z)
    }
    while (user_state.thread_pool.busy()) {
#if GVOX_ENABLE_MULTITHREADED_ADAPTERS && GVOX_ENABLE_THREADSAFETY
        std::this_thread::sleep_for(10ms);
#endif
    }
    user_state.thread_pool.stop();
}

// Parse Driven
extern "C" void gvox_serialize_adapter_gvox_palette_receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    auto &user_state = *static_cast<GvoxPaletteSerializeUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto temp_region = *region;
    handle_region(blit_ctx, user_state, &region->range, &temp_region);
}
