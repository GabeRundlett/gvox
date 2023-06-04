#include <gvox/gvox.h>
#include <gvox/adapters/serialize/random_sample.h>

#include <cstdlib>

#include <bit>
#include <array>
#include <vector>
#include <random>
#include <chrono>

struct RandomSampleUserState {
    uint32_t value{};
    std::chrono::nanoseconds duration{};
    GvoxRandomSampleSerializeAdapterConfig config{};
};

// Base
extern "C" void gvox_serialize_adapter_random_sample_create(GvoxAdapterContext *ctx, void const *config) {
    auto *user_state_ptr = malloc(sizeof(RandomSampleUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) RandomSampleUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
    if (config != nullptr) {
        user_state.config = *static_cast<GvoxRandomSampleSerializeAdapterConfig const *>(config);
    } else {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_OUTPUT_ADAPTER, "Can't use this 'bytes' output adapter without a config");
    }
}

extern "C" void gvox_serialize_adapter_random_sample_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<RandomSampleUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~RandomSampleUserState();
    free(&user_state);
}

extern "C" void gvox_serialize_adapter_random_sample_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
}

extern "C" void gvox_serialize_adapter_random_sample_blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<RandomSampleUserState *>(gvox_adapter_get_user_pointer(ctx));
    auto duration = std::chrono::duration<float>(user_state.duration).count();
    gvox_output_write(blit_ctx, 0, sizeof(duration), &duration);
}

// Serialize Driven
extern "C" void gvox_serialize_adapter_random_sample_serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    auto &user_state = *static_cast<RandomSampleUserState *>(gvox_adapter_get_user_pointer(ctx));

    std::vector<uint8_t> channels;
    channels.resize(static_cast<size_t>(std::popcount(channel_flags)));
    uint32_t next_channel = 0;
    for (uint8_t channel_i = 0; channel_i < 32; ++channel_i) {
        if ((channel_flags & (1u << channel_i)) != 0) {
            channels[next_channel] = channel_i;
            ++next_channel;
        }
    }

    auto dev = std::random_device{};
    auto rng = std::mt19937(dev());
    auto x_dist = std::uniform_int_distribution<std::mt19937::result_type>(0, range->extent.x - 1);
    auto y_dist = std::uniform_int_distribution<std::mt19937::result_type>(0, range->extent.y - 1);
    auto z_dist = std::uniform_int_distribution<std::mt19937::result_type>(0, range->extent.z - 1);
    auto c_dist = std::uniform_int_distribution<std::mt19937::result_type>(0, channels.size() - 1);

    for (size_t i = 0; i < user_state.config.sample_count; ++i) {
        // get random pos
        auto pos = GvoxOffset3D{
            .x = range->offset.x + static_cast<int32_t>(x_dist(rng)),
            .y = range->offset.y + static_cast<int32_t>(y_dist(rng)),
            .z = range->offset.z + static_cast<int32_t>(z_dist(rng)),
        };
        // get random channel
        auto channel_i = c_dist(rng);

        auto const sample_range = GvoxRegionRange{
            .offset = pos,
            .extent = GvoxExtent3D{1, 1, 1},
        };
        using Clock = std::chrono::high_resolution_clock;
        auto t0 = Clock::now();
        auto region = gvox_load_region_range(blit_ctx, &sample_range, 1u << channels[channel_i]);
        auto sample = gvox_sample_region(blit_ctx, &region, &pos, channels[channel_i]);
        auto t1 = Clock::now();
        if (sample.is_present == 0u) {
            sample.data = 0u;
        }
        user_state.value += sample.data;
        user_state.duration += t1 - t0;
        gvox_unload_region_range(blit_ctx, &region, &sample_range);
    }
}

// Parse Driven
extern "C" void gvox_serialize_adapter_random_sample_receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    // Shouldn't be called
}
