#include <gvox/gvox.h>

namespace {
    auto create(void **out_self, GvoxParserCreateCbArgs const *args) -> GvoxResult {
        return GVOX_ERROR_UNKNOWN;
    }
    auto create_from_input(GvoxInputStream input_stream, GvoxParser *user_parser) -> GvoxResult {
        return GVOX_ERROR_UNKNOWN;
    }
    void destroy(void *self_ptr) {
    }
    void create_iterator(void * /*self_ptr*/, void **out_iterator_ptr) {
    }
    void destroy_iterator(void * /*self_ptr*/, void *iterator_ptr) {
    }
    void iterator_advance(void *self_ptr, void **iterator_ptr, GvoxIteratorAdvanceInfo const *info, GvoxIteratorValue *out) {
        out->tag = GVOX_ITERATOR_VALUE_TYPE_NULL;
    }
} // namespace

auto gvox_parser_skeleton_description() GVOX_FUNC_ATTRIB->GvoxParserDescription {
    return GvoxParserDescription{
        .create = create,
        .destroy = destroy,
        .create_from_input = create_from_input,
        .create_iterator = create_iterator,
        .destroy_iterator = destroy_iterator,
        .iterator_advance = iterator_advance,
    };
}
