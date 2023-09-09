#include <gvox/gvox.h>

#include <vector>
#include <thread>

#include "utils/tracy.hpp"

struct GvoxChainStruct {
    GvoxStructType struct_type;
    void const *next;
};

void gvox_init(void) GVOX_FUNC_ATTRIB {
    // TODO: REMOVE gvox_init
    // ZoneScoped;
    using namespace std::literals;
    std::this_thread::sleep_for(5s);
}

auto gvox_fill(GvoxFillInfo const *info) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;

    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->struct_type != GVOX_STRUCT_TYPE_FILL_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    // TODO: Formalize with adapters
    info->dst;

    return GVOX_ERROR_UNKNOWN;
}

auto gvox_blit_prepare(GvoxParser parser) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;

    if (parser == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }

    return GVOX_ERROR_UNKNOWN;
}

auto gvox_blit(GvoxBlitInfo const *info) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;

    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->struct_type != GVOX_STRUCT_TYPE_BLIT_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    return GVOX_ERROR_UNKNOWN;
}

// Adapter API
