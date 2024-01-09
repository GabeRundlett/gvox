#ifndef GVOX_GVOX_H
#define GVOX_GVOX_H

#pragma region GVOX_CORE

#include <stddef.h>
#include <stdint.h>

#if !defined(GVOX_CMAKE_EXPORT)
#define GVOX_CMAKE_EXPORT
#endif

#define GVOX_DEFINE_HANDLE(NAME) typedef struct NAME##_ImplT *NAME

#ifdef __cplusplus
#define GVOX_EXPORT extern "C" GVOX_CMAKE_EXPORT
#define GVOX_FUNC_ATTRIB noexcept
#define GVOX_DEFAULT(x) = x
#define GVOX_FUNC(ret, name, ...) GVOX_EXPORT auto name(__VA_ARGS__) GVOX_FUNC_ATTRIB->ret
#define GVOX_ENUM(x) enum x : int32_t
#define GVOX_STRUCT(x) struct x
#define GVOX_UNION(x) union x
#define GVOX_TYPE(x, type) using x = type
#define GVOX_CONSTANTS_BEGIN(x)
#define GVOX_CONSTANT(x, value) static inline constexpr auto x = (value);
#define GVOX_CONSTANTS_END()
#else
#define GVOX_EXPORT GVOX_CMAKE_EXPORT
#define GVOX_FUNC_ATTRIB
#define GVOX_DEFAULT(x)
#define GVOX_FUNC(ret, name, ...) GVOX_EXPORT ret name(__VA_ARGS__) GVOX_FUNC_ATTRIB
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
    GVOX_ERROR_UNPARSABLE_INPUT = -12,
};

GVOX_ENUM(GvoxStructType){
    GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_CB_ARGS,
    GVOX_STRUCT_TYPE_OUTPUT_STREAM_CREATE_CB_ARGS,
    GVOX_STRUCT_TYPE_PARSER_CREATE_CB_ARGS,
    GVOX_STRUCT_TYPE_SERIALIZER_CREATE_CB_ARGS,
    GVOX_STRUCT_TYPE_CONTAINER_CREATE_CB_ARGS,

    GVOX_STRUCT_TYPE_INPUT_STREAM_CREATE_INFO,
    GVOX_STRUCT_TYPE_OUTPUT_STREAM_CREATE_INFO,
    GVOX_STRUCT_TYPE_PARSER_CREATE_INFO,
    GVOX_STRUCT_TYPE_SERIALIZER_CREATE_INFO,
    GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,
    GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
    GVOX_STRUCT_TYPE_SAMPLER_CREATE_INFO,
    GVOX_STRUCT_TYPE_ITERATOR_CREATE_INFO,
    GVOX_STRUCT_TYPE_PARSE_ITERATOR_CREATE_INFO,
    GVOX_STRUCT_TYPE_CONTAINER_ITERATOR_CREATE_INFO,

    GVOX_STRUCT_TYPE_FILL_INFO,
    GVOX_STRUCT_TYPE_MOVE_INFO,
    GVOX_STRUCT_TYPE_BLIT_INFO,
    GVOX_STRUCT_TYPE_SAMPLE_INFO,
    GVOX_STRUCT_TYPE_SERIALIZE_INFO,

    GVOX_STRUCT_TYPE_ATTRIBUTE,
    GVOX_STRUCT_TYPE_PARSER_DESCRIPTION_COLLECTION,
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
GVOX_DEFINE_HANDLE(GvoxIterator);

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

#define _GVOX_DECL_OFFSET_EXTENT_ND(N)                  \
    GVOX_STRUCT(GvoxOffset##N##D) { int64_t data[N]; }; \
    GVOX_STRUCT(GvoxExtent##N##D) { uint64_t data[N]; };

_GVOX_DECL_OFFSET_EXTENT_ND(1)
_GVOX_DECL_OFFSET_EXTENT_ND(2)
_GVOX_DECL_OFFSET_EXTENT_ND(3)
_GVOX_DECL_OFFSET_EXTENT_ND(4)
_GVOX_DECL_OFFSET_EXTENT_ND(5)
_GVOX_DECL_OFFSET_EXTENT_ND(6)

#undef _GVOX_DECL_OFFSET_EXTENT_ND

GVOX_STRUCT(GvoxRange) {
    GvoxOffset offset;
    GvoxExtent extent;
};

GVOX_STRUCT(GvoxRangeMut) {
    GvoxOffsetMut offset;
    GvoxExtentMut extent;

#ifdef __cplusplus
    explicit operator GvoxRange() const {
        return GvoxRange{.offset = static_cast<GvoxOffset>(this->offset), .extent = static_cast<GvoxExtent>(this->extent)};
    }
#endif
};

GVOX_ENUM(GvoxIteratorValueType){
    GVOX_ITERATOR_VALUE_TYPE_NULL,
    GVOX_ITERATOR_VALUE_TYPE_LEAF,
    GVOX_ITERATOR_VALUE_TYPE_NODE_BEGIN,
    GVOX_ITERATOR_VALUE_TYPE_NODE_END,
};

GVOX_STRUCT(GvoxIteratorValue) {
    GvoxIteratorValueType tag;
    void const *voxel_data;
    GvoxVoxelDesc voxel_desc;
    GvoxRange range;
};

#ifdef __cplusplus

#include <concepts>
template <typename T>
concept GvoxOffsetOrExtentType =
    std::same_as<T, GvoxOffset> ||
    std::same_as<T, GvoxOffsetMut> ||
    std::same_as<T, GvoxExtent> ||
    std::same_as<T, GvoxExtentMut>;

template <typename T>
concept GvoxOffsetType =
    std::same_as<T, GvoxOffset> ||
    std::same_as<T, GvoxOffsetMut>;

template <typename T>
concept GvoxExtentType =
    std::same_as<T, GvoxExtent> ||
    std::same_as<T, GvoxExtentMut>;

#endif

#undef GVOX_DEFINE_HANDLE

#pragma endregion GVOX_CORE

#pragma region GVOX_FORMAT

#define GVOX_CREATE_FORMAT(encoding, extra_state) \
    ((((uint32_t)(encoding)) << 0) |              \
     (((uint32_t)(extra_state)) << 10))

#define GVOX_SINGLE_CHANNEL_BIT_COUNT(bit_count) \
    (((uint32_t)(bit_count)) - 1)

#define GVOX_PACKED_ENCODING(component_count, swizzle_mode, d0_bit_count, d1_bit_count, d2_bit_count) \
    ((((uint32_t)(component_count)-1) << 0) |                                                         \
     (((uint32_t)(swizzle_mode)) << 2) |                                                              \
     ((((uint32_t)(d0_bit_count) == 0 ? 0 : (((uint32_t)(d0_bit_count)) - 1))) << 3) |                \
     ((((uint32_t)(d1_bit_count) == 0 ? 0 : (((uint32_t)(d1_bit_count)) - 1))) << 9) |                \
     ((((uint32_t)(d2_bit_count) == 0 ? 0 : (((uint32_t)(d2_bit_count)) - 1))) << 15))

GVOX_ENUM(GvoxFormatEncoding){
    GVOX_FORMAT_ENCODING_UNKNOWN,
    GVOX_FORMAT_ENCODING_RAW,
    GVOX_FORMAT_ENCODING_UNORM,
    GVOX_FORMAT_ENCODING_SNORM,
    GVOX_FORMAT_ENCODING_SRGB,
    GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT,
    // GVOX_FORMAT_ENCODING_SHARED_EXP_UFLOAT,
};

GVOX_ENUM(GvoxAttributeType){
    GVOX_ATTRIBUTE_TYPE_UNKNOWN,

    GVOX_ATTRIBUTE_TYPE_ARBITRARY_INTEGER,
    GVOX_ATTRIBUTE_TYPE_ARBITRARY_FLOAT,
    GVOX_ATTRIBUTE_TYPE_ALBEDO_R,
    GVOX_ATTRIBUTE_TYPE_ALBEDO_G,
    GVOX_ATTRIBUTE_TYPE_ALBEDO_B,
    GVOX_ATTRIBUTE_TYPE_NORMAL_X,
    GVOX_ATTRIBUTE_TYPE_NORMAL_Y,
    GVOX_ATTRIBUTE_TYPE_NORMAL_Z,
    GVOX_ATTRIBUTE_TYPE_ROUGHNESS,
    GVOX_ATTRIBUTE_TYPE_OPACITY,

    GVOX_ATTRIBUTE_TYPE_UNKNOWN_PACKED = (1 << 8),
    GVOX_ATTRIBUTE_TYPE_ALBEDO_PACKED,
    GVOX_ATTRIBUTE_TYPE_NORMAL_PACKED,
};

GVOX_ENUM(GvoxSwizzleMode){
    GVOX_SWIZZLE_MODE_NONE,
    GVOX_SWIZZLE_MODE_REVERSE,
};

GVOX_TYPE(GvoxFormat, uint32_t);

GVOX_CONSTANTS_BEGIN(GvoxStandardFormat)

GVOX_CONSTANT(GVOX_STANDARD_FORMAT_BITMASK, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_RAW, GVOX_PACKED_ENCODING(1, GVOX_SWIZZLE_MODE_NONE, 1, 0, 0)))
GVOX_CONSTANT(GVOX_STANDARD_FORMAT_R8G8B8_SRGB, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_SRGB, GVOX_PACKED_ENCODING(3, GVOX_SWIZZLE_MODE_NONE, 8, 8, 8)))
GVOX_CONSTANT(GVOX_STANDARD_FORMAT_B8G8R8_SRGB, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_SRGB, GVOX_PACKED_ENCODING(3, GVOX_SWIZZLE_MODE_REVERSE, 8, 8, 8)))
GVOX_CONSTANT(GVOX_STANDARD_FORMAT_R8G8B8_UNORM, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_UNORM, GVOX_PACKED_ENCODING(3, GVOX_SWIZZLE_MODE_NONE, 8, 8, 8)))
// GVOX_CONSTANT(GVOX_STANDARD_FORMAT_E5B9G9R9_UFLOAT, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_SHARED_EXP_UFLOAT, GVOX_PACKED_ENCODING(1, GVOX_SWIZZLE_MODE_REVERSE, 32, 9, 5)))
GVOX_CONSTANT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT_PACK08, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT, GVOX_PACKED_ENCODING(1, GVOX_SWIZZLE_MODE_NONE, 8, 0, 0)))
GVOX_CONSTANT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT_PACK12, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT, GVOX_PACKED_ENCODING(1, GVOX_SWIZZLE_MODE_NONE, 12, 0, 0)))
GVOX_CONSTANT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT_PACK16, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT, GVOX_PACKED_ENCODING(1, GVOX_SWIZZLE_MODE_NONE, 16, 0, 0)))
GVOX_CONSTANT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT_PACK24, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT, GVOX_PACKED_ENCODING(1, GVOX_SWIZZLE_MODE_NONE, 24, 0, 0)))
GVOX_CONSTANT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT_PACK32, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_3D_UNIT, GVOX_PACKED_ENCODING(1, GVOX_SWIZZLE_MODE_NONE, 32, 0, 0)))

GVOX_CONSTANTS_END()

GVOX_STRUCT(GvoxAttribute) {
    GvoxStructType struct_type;
    void const *next;
    GvoxAttributeType type;
    GvoxFormat format;
};

GVOX_STRUCT(GvoxVoxelDescCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    uint32_t attribute_count;
    GvoxAttribute const *attributes;
};

GVOX_STRUCT(GvoxAttributeMapping) {
    uint8_t dst_index;
    uint8_t src_index;
    uint8_t src1_index;
    uint8_t src2_index;
};

#pragma endregion GVOX_FORMAT

#pragma region GVOX_STREAM

GVOX_ENUM(GvoxSeekOrigin){
    GVOX_SEEK_ORIGIN_BEG,
    GVOX_SEEK_ORIGIN_CUR,
    GVOX_SEEK_ORIGIN_END,
};

GVOX_ENUM(GvoxIteratorAdvanceMode){
    GVOX_ITERATOR_ADVANCE_MODE_NEXT,
    GVOX_ITERATOR_ADVANCE_MODE_SKIP_BRANCH,
};

GVOX_STRUCT(GvoxIteratorAdvanceInfo) {
    // TODO: Figure out if this makes sense
    GvoxInputStream input_stream;
    GvoxIteratorAdvanceMode mode;
};

GVOX_STRUCT(GvoxSample) {
    GvoxOffset offset;
    void *dst_voxel_data;
    GvoxVoxelDesc dst_voxel_desc;
};

GVOX_STRUCT(GvoxInputStreamCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxOutputStreamCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxParserCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    GvoxInputStream input_stream;
    void const *config;
};

GVOX_STRUCT(GvoxSerializerCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    GvoxOutputStream output_stream;
    void const *config;
};

GVOX_STRUCT(GvoxContainerCreateCbArgs) {
    GvoxStructType struct_type;
    void const *next;
    void const *config;
};

GVOX_STRUCT(GvoxInputStreamDescription) {
    GvoxResult (*create)(void **, GvoxInputStreamCreateCbArgs const *);
    GvoxResult (*read)(void *, GvoxInputStream, void *, size_t);
    GvoxResult (*seek)(void *, GvoxInputStream, int64_t, GvoxSeekOrigin);
    int64_t (*tell)(void *, GvoxInputStream);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxOutputStreamDescription) {
    GvoxResult (*create)(void **, GvoxOutputStreamCreateCbArgs const *);
    GvoxResult (*write)(void *, GvoxOutputStream, void *, size_t);
    GvoxResult (*seek)(void *, GvoxOutputStream, int64_t, GvoxSeekOrigin);
    int64_t (*tell)(void *, GvoxOutputStream);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxParserDescription) {
    GvoxResult (*create)(void **, GvoxParserCreateCbArgs const *);
    void (*destroy)(void *);
    GvoxResult (*create_from_input)(GvoxInputStream, GvoxParser *);

    void (*create_iterator)(void *, void **);
    void (*destroy_iterator)(void *, void *);
    void (*iterator_advance)(void *, void **, GvoxIteratorAdvanceInfo const *, GvoxIteratorValue *);
};

GVOX_STRUCT(GvoxSerializerDescription) {
    GvoxResult (*create)(void **, GvoxSerializerCreateCbArgs const *);
    void (*destroy)(void *);
};

GVOX_STRUCT(GvoxContainerDescription) {
    GvoxResult (*create)(void **, GvoxContainerCreateCbArgs const *);
    void (*destroy)(void *);
    GvoxResult (*fill)(void *, void const *, GvoxVoxelDesc, GvoxRange);
    GvoxResult (*move)(void *, GvoxContainer *, GvoxRange *, GvoxOffset *, uint32_t);
    GvoxResult (*sample)(void *, GvoxSample const *, uint32_t);

    void (*create_iterator)(void *, void **);
    void (*destroy_iterator)(void *, void *);
    void (*iterator_advance)(void *, void **, GvoxIteratorAdvanceInfo const *, GvoxIteratorValue *);
};

GVOX_STRUCT(GvoxInputStreamCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    struct GvoxInputStreamCreateInfo const *stream_chain;
    GvoxInputStreamDescription description;
    GvoxInputStreamCreateCbArgs cb_args;
};

GVOX_STRUCT(GvoxOutputStreamCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    void const *stream_chain;
    GvoxOutputStreamDescription description;
    GvoxOutputStreamCreateCbArgs cb_args;
};

GVOX_STRUCT(GvoxParserCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxParserDescription description;
    GvoxParserCreateCbArgs cb_args;
};

GVOX_STRUCT(GvoxSerializerCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxSerializerDescription description;
    GvoxSerializerCreateCbArgs cb_args;
};

GVOX_STRUCT(GvoxContainerCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainerDescription description;
    GvoxContainerCreateCbArgs cb_args;
};

GVOX_ENUM(GvoxIteratorType){
    GVOX_ITERATOR_TYPE_INPUT,
    GVOX_ITERATOR_TYPE_OUTPUT,
};

GVOX_STRUCT(GvoxIteratorCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
};

GVOX_STRUCT(GvoxParseIteratorCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxParser parser;
};

GVOX_STRUCT(GvoxContainerIteratorCreateInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainer container;
};

GVOX_STRUCT(GvoxParserDescriptionCollection) {
    GvoxStructType struct_type;
    void const *next;
    GvoxParserDescription const *descriptions;
    uint32_t description_n;
};

#pragma endregion GVOX_STREAM

GVOX_STRUCT(GvoxVolume) {
    uint32_t n_dimensions;
    void *data;
    uint32_t *extent;
    float *transform;
};

GVOX_STRUCT(GvoxFillInfo) {
    GvoxStructType struct_type;
    void const *next;
    void const *src_data;
    GvoxVoxelDesc src_desc;
    GvoxContainer dst;
    GvoxRange range;
};

GVOX_STRUCT(GvoxMoveInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainer *src_containers;
    GvoxRange *src_container_ranges;
    GvoxOffset *src_dst_offsets;
    uint32_t src_container_n;
    GvoxContainer dst;
};

GVOX_STRUCT(GvoxBlitInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxParser src;
    GvoxSerializer dst;
};

GVOX_STRUCT(GvoxSampleInfo) {
    GvoxStructType struct_type;
    void const *next;
    GvoxContainer src;
    GvoxSample const *samples;
    uint32_t sample_n;
};

#pragma region GVOX_STREAM
GVOX_FUNC(GvoxResult, gvox_create_input_stream, GvoxInputStreamCreateInfo const *info, GvoxInputStream *handle);
GVOX_FUNC(GvoxResult, gvox_create_output_stream, GvoxOutputStreamCreateInfo const *info, GvoxOutputStream *handle);
GVOX_FUNC(GvoxResult, gvox_create_parser, GvoxParserCreateInfo const *info, GvoxParser *handle);
GVOX_FUNC(GvoxResult, gvox_create_serializer, GvoxSerializerCreateInfo const *info, GvoxSerializer *handle);
GVOX_FUNC(GvoxResult, gvox_create_container, GvoxContainerCreateInfo const *info, GvoxContainer *handle);
GVOX_FUNC(GvoxResult, gvox_create_iterator, GvoxIteratorCreateInfo const *info, GvoxIterator *handle);

GVOX_FUNC(void, gvox_destroy_input_stream, GvoxInputStream handle);
GVOX_FUNC(void, gvox_destroy_output_stream, GvoxOutputStream handle);
GVOX_FUNC(void, gvox_destroy_parser, GvoxParser handle);
GVOX_FUNC(void, gvox_destroy_serializer, GvoxSerializer handle);
GVOX_FUNC(void, gvox_destroy_container, GvoxContainer handle);
GVOX_FUNC(void, gvox_destroy_iterator, GvoxIterator handle);

GVOX_FUNC(GvoxResult, gvox_input_read, GvoxInputStream handle, void *data, size_t size);
GVOX_FUNC(GvoxResult, gvox_input_seek, GvoxInputStream handle, int64_t offset, GvoxSeekOrigin origin);
GVOX_FUNC(int64_t, gvox_input_tell, GvoxInputStream handle);

GVOX_FUNC(GvoxResult, gvox_output_write, GvoxOutputStream handle, void *data, size_t size);
GVOX_FUNC(GvoxResult, gvox_output_seek, GvoxOutputStream handle, int64_t offset, GvoxSeekOrigin origin);
GVOX_FUNC(int64_t, gvox_output_tell, GvoxOutputStream handle);

GVOX_FUNC(GvoxResult, gvox_enumerate_standard_parser_descriptions, GvoxParserDescription const **out_descriptions, uint32_t *description_n);
GVOX_FUNC(GvoxResult, gvox_create_parser_from_input, GvoxParserDescriptionCollection const *parsers, GvoxInputStream input_stream, GvoxParser *user_parser);

GVOX_EXPORT void gvox_iterator_advance(GvoxIterator handle, GvoxIteratorAdvanceInfo const *info, GvoxIteratorValue *value);
#pragma endregion GVOX_STREAM

#pragma region GVOX_FORMAT
GVOX_FUNC(GvoxResult, gvox_create_voxel_desc, GvoxVoxelDescCreateInfo const *info, GvoxVoxelDesc *handle);
GVOX_FUNC(void, gvox_destroy_voxel_desc, GvoxVoxelDesc handle);

GVOX_FUNC(uint32_t, gvox_voxel_desc_size_in_bits, GvoxVoxelDesc handle);
GVOX_FUNC(uint32_t, gvox_voxel_desc_attribute_count, GvoxVoxelDesc handle);

GVOX_FUNC(uint8_t, gvox_voxel_desc_compare, GvoxVoxelDesc desc_a, GvoxVoxelDesc desc_b);

// GVOX_FUNC(GvoxResult, gvox_translate_format, void const *src_data, GvoxFormat src_format, void *dst_data, GvoxFormat dst_format);
GVOX_FUNC(GvoxResult, gvox_translate_voxel, void const *src_data, GvoxVoxelDesc src_desc, void *dst_data, GvoxVoxelDesc dst_desc, GvoxAttributeMapping const *attrib_mapping, uint32_t attrib_mapping_n);
#pragma region GVOX_FORMAT

GVOX_FUNC(GvoxResult, gvox_fill, GvoxFillInfo const *info);
GVOX_FUNC(GvoxResult, gvox_move, GvoxMoveInfo const *info);
GVOX_FUNC(GvoxResult, gvox_blit, GvoxBlitInfo const *info);
GVOX_FUNC(GvoxResult, gvox_sample, GvoxSampleInfo const *info);

#endif
