#ifndef GVOX_CORE_H
#define GVOX_CORE_H

#include <stddef.h>
#include <stdint.h>

#if !defined(GVOX_EXPORT)
#define GVOX_EXPORT
#endif

#define GVOX_DEFINE_HANDLE(NAME) typedef struct NAME##_ImplT *NAME

#ifdef __cplusplus
#define GVOX_DEFAULT(x) = x
#define GVOX_ENUM(x) enum x : int32_t
#define GVOX_STRUCT(x) struct x
#define GVOX_UNION(x) union x
#define GVOX_TYPE(x, type) using x = type
#define GVOX_CONSTANTS_BEGIN(x)
#define GVOX_CONSTANT(x, value) static inline constexpr auto x = (value);
#define GVOX_CONSTANTS_END()
#else
#define GVOX_DEFAULT(x)
#define GVOX_ENUM(x)   \
    typedef int32_t x; \
    enum x
#define GVOX_STRUCT(x)  \
    typedef struct x x; \
    struct x
#define GVOX_UNION(x)  \
    typedef union x x; \
    union x
#define GVOX_TYPE(x, type) typedef type x
#define GVOX_CONSTANTS_BEGIN(x) \
    typedef uint32_t x;         \
    enum x {
#define GVOX_CONSTANT(x, value) x = (value),
#define GVOX_CONSTANTS_END() \
    }                        \
    ;
#endif

GVOX_ENUM(GvoxResult){
    GVOX_SUCCESS = 0,
    GVOX_ERROR_UNKNOWN = -1,
    GVOX_ERROR_UNKNOWN_STANDARD_INPUT_STREAM = -2,
    GVOX_ERROR_UNKNOWN_STANDARD_OUTPUT_STREAM = -3,
    GVOX_ERROR_UNKNOWN_STANDARD_PARSER = -6,
    GVOX_ERROR_UNKNOWN_STANDARD_SERIALIZER = -7,
    GVOX_ERROR_UNKNOWN_STANDARD_CONTAINER = -8,
    GVOX_ERROR_INVALID_ARGUMENT = -9,
    GVOX_ERROR_BAD_STRUCT_TYPE = -10,
    GVOX_ERROR_BAD_STREAM_CHAIN = -11,
};

GVOX_ENUM(GvoxStructType){
    GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_INFO,
    GVOX_STRUCT_TYPE_OUTPUT_STREAM_CREATE_INFO,
    GVOX_STRUCT_TYPE_PARSER_CREATE_INFO,
    GVOX_STRUCT_TYPE_SERIALIZER_CREATE_INFO,
    GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,

    GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
    GVOX_STRUCT_TYPE_SAMPLER_CREATE_INFO,

    GVOX_STRUCT_TYPE_PARSE_INFO,
    GVOX_STRUCT_TYPE_FILL_INFO,
    GVOX_STRUCT_TYPE_BLIT_INFO,
    GVOX_STRUCT_TYPE_SAMPLE_INFO,
    GVOX_STRUCT_TYPE_SERIALIZE_INFO,

    GVOX_STRUCT_TYPE_ATTRIBUTE,
};

GVOX_DEFINE_HANDLE(GvoxInputStream);
GVOX_DEFINE_HANDLE(GvoxOutputStream);
GVOX_DEFINE_HANDLE(GvoxInputStream);
GVOX_DEFINE_HANDLE(GvoxOutputStream);
GVOX_DEFINE_HANDLE(GvoxParser);
GVOX_DEFINE_HANDLE(GvoxSerializer);
GVOX_DEFINE_HANDLE(GvoxContainer);
GVOX_DEFINE_HANDLE(GvoxVoxelDesc);
GVOX_DEFINE_HANDLE(GvoxSampler);

GVOX_STRUCT(GvoxOffset) {
    uint32_t axis_n;
    int64_t const *axis;
};
GVOX_STRUCT(GvoxExtent) {
    uint32_t axis_n;
    uint64_t const *axis;
};

GVOX_STRUCT(GvoxOffsetMut) {
    uint32_t axis_n;
    int64_t *axis;

#ifdef __cplusplus
    explicit operator GvoxOffset() const {
        return GvoxOffset{.axis_n = this->axis_n, .axis = this->axis};
    }
#endif
};
GVOX_STRUCT(GvoxExtentMut) {
    uint32_t axis_n;
    uint64_t *axis;

#ifdef __cplusplus
    explicit operator GvoxExtent() const {
        return GvoxExtent{.axis_n = this->axis_n, .axis = this->axis};
    }
#endif
};

GVOX_STRUCT(GvoxRange) {
    GvoxOffset offset;
    GvoxExtent extent;
};

#ifdef __cplusplus
#define GVOX_FUNC_ATTRIB noexcept
#else
#define GVOX_FUNC_ATTRIB
#endif

#undef GVOX_DEFINE_HANDLE

#endif
