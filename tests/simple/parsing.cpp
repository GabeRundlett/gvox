#include <gvox/gvox.h>

#include <cstdio>
#include <cstdlib>

#include <gvox/adapters/input/file.h>
#include <mutex>

void create(GvoxAdapterContext *ctx, void const *config) {
}
void destroy(GvoxAdapterContext *ctx) {
}
void blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
}
void blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
}
void serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
}

// This function may be called in a parallel nature by the parse adapter.
// If necessary in our application, we may need to synchronize. For example,
// here we need to lock printf because we only want one thread writing to
// the console at any time.
auto printf_mtx = std::mutex{};
void receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    auto lock = std::lock_guard{printf_mtx};
    printf("offset: (%d %d %d) extent: (%d %d %d) channels: %d flags: %d\n",
           region->range.offset.x,
           region->range.offset.y,
           region->range.offset.z,
           region->range.extent.x,
           region->range.extent.y,
           region->range.extent.z,
           region->channels,
           region->flags);

    // `region` description:
    //  `.data` is a pointer to the raw data, in a 3D array described by the rest of the region struct.
    //  `.range.offset` is the 3D location of the 3D array in world-space
    //  `.range.extent` is the dimensions of the 3D array.
    //  `.channels` is the channel flags, in this case it should just be `GVOX_CHANNEL_BIT_COLOR`.
    //      This means the voxel data is 1 extent.x * extent.y * extent.z uint32_ts long. If there
    //      were multiple channel flags, then the data would be interleaved uint32_ts.
    //  `.flags` is a set of GVOX_REGION_FLAG_ bits, which describe extra metadata about the region.s
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
    auto *p_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_parse_adapter(gvox_ctx, "magicavoxel"), NULL);
    // we don't need an output adapter, as we're not using it in our serialize adapter.
    auto *o_ctx = (GvoxAdapterContext *)nullptr;
    auto *s_ctx = gvox_create_adapter_context(gvox_ctx, gvox_get_serialize_adapter(gvox_ctx, "my_adapter"), nullptr);

#if 0 // we optionally can provide a specified range of the file to parse
    auto region_range = GvoxRegionRange{.offset = {-4, -4, -4}, .extent = {8, 8, 8}};
    auto *region_range_ptr = &region_range;
#else
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
