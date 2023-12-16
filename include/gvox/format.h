#ifndef GVOX_FORMAT_H
#define GVOX_FORMAT_H

#include <gvox/core.h>

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

GVOX_EXPORT GvoxResult gvox_create_voxel_desc(GvoxVoxelDescCreateInfo const *info, GvoxVoxelDesc *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_voxel_desc(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT uint32_t gvox_voxel_desc_size_in_bits(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT uint32_t gvox_voxel_desc_attribute_count(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT uint8_t gvox_voxel_desc_compare(GvoxVoxelDesc desc_a, GvoxVoxelDesc desc_b) GVOX_FUNC_ATTRIB;

// GVOX_EXPORT GvoxResult gvox_translate_format(void const *src_data, GvoxFormat src_format, void *dst_data, GvoxFormat dst_format) GVOX_FUNC_ATTRIB;
GVOX_EXPORT GvoxResult gvox_translate_voxel(void const *src_data, GvoxVoxelDesc src_desc, void *dst_data, GvoxVoxelDesc dst_desc, GvoxAttributeMapping const *attrib_mapping, uint32_t attrib_mapping_n) GVOX_FUNC_ATTRIB;

#endif
