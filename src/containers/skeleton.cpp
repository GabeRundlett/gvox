#include <gvox/gvox.h>

namespace {
    auto create(void **out_self, GvoxContainerCreateCbArgs const *args) -> GvoxResult {
        return GVOX_ERROR_UNKNOWN;
    }
    void destroy(void *self_ptr) {
    }
    auto fill(void *self_ptr, void const *single_voxel_data, GvoxVoxelDesc src_voxel_desc, GvoxRange range) -> GvoxResult {
        return GVOX_ERROR_UNKNOWN;
    }
    auto move(void *self_ptr, GvoxContainer *src_containers, GvoxRange *src_ranges, GvoxOffset *offsets, uint32_t src_container_n) -> GvoxResult {
        return GVOX_ERROR_UNKNOWN;
    }
    auto sample(void *self_ptr, GvoxSample const *samples, uint32_t sample_n) -> GvoxResult {
        return GVOX_ERROR_UNKNOWN;
    }
    void create_iterator(void * /*self_ptr*/, void **out_iterator_ptr) {
    }
    void destroy_iterator(void * /*self_ptr*/, void *iterator_ptr) {
    }
    void iterator_advance(void *self_ptr, void **iterator_ptr, GvoxIteratorAdvanceInfo const *info, GvoxIteratorValue *out) {
        out->tag = GVOX_ITERATOR_VALUE_TYPE_NULL;
    }
} // namespace

auto gvox_container_skeleton_description() GVOX_FUNC_ATTRIB->GvoxContainerDescription {
    return GvoxContainerDescription{
        .create = create,
        .destroy = destroy,
        .fill = fill,
        .move = move,
        .sample = sample,
        .create_iterator = create_iterator,
        .destroy_iterator = destroy_iterator,
        .iterator_advance = iterator_advance,
    };
}
