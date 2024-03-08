#include <gvox/gvox.h>

#include <cstdio>
#include <cstdlib>

#include <gvox/adapters/input/file.h>
#include <mutex>

// Called when creating the adapter context
void create(GvoxAdapterContext *ctx, void const *config) {
    // here you can create any resources you want to associate with your adapter.
    // you can tie your state to the adapter context `ctx` with this function:
    //   void *my_pointer = malloc(sizeof(int));
    //   gvox_adapter_set_user_pointer(ctx, my_pointer);
    // which can be retrieved at any time with the get variant of this function.
}
// Called when destroying the adapter context (for freeing any resources created by the adapter)
void destroy(GvoxAdapterContext *ctx) {
    // here we'd free `my_pointer`
    //   void *my_pointer = gvox_adapter_get_user_pointer(ctx);
    //   free(my_pointer);
}
// Called once, at the beginning of a blit operation.
void blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    // We get a minimal description of the volume we're blitting.
}
// Called once, at the end of a blit operation.
void blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
}
// Legacy... We don't care to implement.
void serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
}

// This function may be called in a parallel nature by the parse adapter.
void receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    // `GvoxRegion` description:
    //  `.range.offset` is the 3D location of the 3D array in world-space
    //  `.range.extent` is the dimensions of the 3D array.
    //  `.channels` is the channel flags, in this case it should just be `GVOX_CHANNEL_BIT_COLOR`.
    //  `.flags` is a set of GVOX_REGION_FLAG_ bits, which describe extra metadata about the region.

    GvoxOffset3D sample_position = region->range.offset;
    // In order to sample voxel data from the region, use `gvox_sample_region()`:
    //  - blit_ctx is necessary as the data is extracted from the parser's custom region data format.
    //  - region is the pointer to the region that has been acquired.
    //  - sample position is the coordinate from [ range.offset, range.offset + range.extent ) that the serializer
    //    would like to query. If a coordinate is specified outside this range, the resulting sample should have
    //    0 for `.is_present`.
    GvoxSample region_sample = gvox_sample_region(blit_ctx, region, &sample_position, GVOX_CHANNEL_ID_COLOR);
    // `GvoxSample` description:
    //  `.is_present` is either 0 or 1 depending on whether there is valid data at the specified coordinate.
    //  `.data` is a single uint32_t that holds the actual data. How this data is represented is defined by
    //     the channel in question. If, for example, one requested GVOX_CHANNEL_ID_COLOR, the 8bpc color data
    //     would be packed into the first 24 bits of the uint32_t.

    {
        static auto printf_mtx = std::mutex{};

        // If necessary in our application, we may need to synchronize. For example,
        // here we need to lock printf because we only want one thread writing to
        // the console at any time.
        auto lock = std::lock_guard{printf_mtx};

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        if (region_sample.is_present != 0) {
            r = (region_sample.data >> 0u) & 0xff;
            g = (region_sample.data >> 8u) & 0xff;
            b = (region_sample.data >> 16u) & 0xff;
        }
        // print-out the color of the voxel in the 0, 0, 0 corner of the region
        printf("\033[48;2;%03d;%03d;%03dm  \033[0m", r, g, b);
        printf("offset: (%d %d %d) extent: (%d %d %d) channels: %d flags: %d\n",
               region->range.offset.x,
               region->range.offset.y,
               region->range.offset.z,
               region->range.extent.x,
               region->range.extent.y,
               region->range.extent.z,
               region->channels,
               region->flags);
    }
}

void handle_gvox_error(GvoxContext *gvox_ctx) {
    GvoxResult res = gvox_get_result(gvox_ctx);
    int error_count = 0;
    while (res != GVOX_RESULT_SUCCESS) {
        size_t size = 0;
        gvox_get_result_message(gvox_ctx, nullptr, &size);
        char *str = new char[size + 1];
        gvox_get_result_message(gvox_ctx, str, nullptr);
        str[size] = '\0';
        printf("ERROR: %s\n", str);
        gvox_pop_result(gvox_ctx);
        delete[] str;
        res = gvox_get_result(gvox_ctx);
        ++error_count;
    }
    if (error_count != 0) {
        exit(-error_count);
    }
}

auto const my_adapter_info = GvoxSerializeAdapterInfo{
    .base_info = {
        .name_str = "my_adapter",
        .create = create,
        .destroy = destroy,
        .blit_begin = blit_begin,
        .blit_end = blit_end,
    },
    .serialize_region = serialize_region,
    .receive_region = receive_region,
};

auto main() -> int {
    auto *gvox_ctx = gvox_create_context();

    // register our custom adapter that'll receive all the model data
    gvox_register_serialize_adapter(gvox_ctx, &my_adapter_info);

    auto const *model_filepath = "tests/simple/nuke.vox";

    auto i_config = GvoxFileInputAdapterConfig{.filepath = model_filepath, .byte_offset = 0};
    auto *i_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_input_adapter(gvox_ctx, "file"), &i_config);
    auto *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "magicavoxel"), nullptr);
    // we don't need an output adapter, as we're not using it in our serialize adapter.
    auto *o_ctx = (GvoxAdapterContext *)nullptr;
    auto *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "my_adapter"), nullptr);

#if 0 // we optionally can provide a specified range of the file to parse
    auto region_range = GvoxRegionRange{.offset = {-4, -4, -4}, .extent = {8, 8, 8}};
    auto *region_range_ptr = &region_range;
#else // otherwise, we're going to parse the entire region of the file
    auto *region_range_ptr = (GvoxRegionRange *)nullptr;
#endif

    // lets parse only color data
    gvox_blit_region_parse_driven(i_ctx, o_ctx, p_ctx, s_ctx, region_range_ptr, GVOX_CHANNEL_BIT_COLOR);
    gvox_destroy_adapter_context(i_ctx);
    gvox_destroy_adapter_context(p_ctx);
    gvox_destroy_adapter_context(s_ctx);

    handle_gvox_error(gvox_ctx);

    gvox_destroy_context(gvox_ctx);
}
