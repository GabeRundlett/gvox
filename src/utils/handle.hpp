#pragma once

#define IMPL_STRUCT_NAME(Name) Gvox##Name##_ImplT

#define HANDLE_NEW(Type, TYPE)                                        \
    if (info == nullptr) {                                            \
        return GVOX_ERROR_INVALID_ARGUMENT;                           \
    }                                                                 \
    if (handle == nullptr) {                                          \
        return GVOX_ERROR_INVALID_ARGUMENT;                           \
    }                                                                 \
    if (info->struct_type != GVOX_STRUCT_TYPE_##TYPE##_CREATE_INFO) { \
        return GVOX_ERROR_BAD_STRUCT_TYPE;                            \
    }                                                                 \
    *handle = new (std::nothrow) IMPL_STRUCT_NAME(Type){};            \
    if (*handle == nullptr) {                                         \
        return GVOX_ERROR_UNKNOWN;                                    \
    }

namespace {
    inline void destroy_handle(auto *handle) {
        ZoneScoped;
        delete handle;
    }
} // namespace
