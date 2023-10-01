#ifndef GVOX_FORMAT_H
#define GVOX_FORMAT_H

#include <gvox/core.h>

#define GVOX_CREATE_FORMAT(encoding, component_count, c0_bit_count, c1_bit_count, c2_bit_count, c3_bit_count) \
    ((((uint64_t)(encoding)) << 0) |                                                                          \
     (((uint64_t)(component_count)-1) << 10) |                                                                \
     (((uint64_t)(c0_bit_count)) << 12) |                                                                     \
     (((uint64_t)(c1_bit_count)) << 17) |                                                                     \
     (((uint64_t)(c2_bit_count)) << 22) |                                                                     \
     (((uint64_t)(c3_bit_count)) << 27))

GVOX_ENUM(GvoxFormatEncoding){
    GVOX_FORMAT_ENCODING_UNKNOWN,
    GVOX_FORMAT_ENCODING_RAW,
    GVOX_FORMAT_ENCODING_UNORM,
    GVOX_FORMAT_ENCODING_SNORM,
    GVOX_FORMAT_ENCODING_SRGB,
    GVOX_FORMAT_ENCODING_OCTAHEDRAL_UNIT,
    GVOX_FORMAT_ENCODING_SHARED_EXP_UFLOAT_BGR,
};

GVOX_ENUM(GvoxAttributeType){
    GVOX_ATTRIBUTE_TYPE_UNKNOWN,
    GVOX_ATTRIBUTE_TYPE_ALBEDO,
    GVOX_ATTRIBUTE_TYPE_NORMAL,
    GVOX_ATTRIBUTE_TYPE_ROUGHNESS,
    GVOX_ATTRIBUTE_TYPE_OPACITY,
};

GVOX_TYPE(GvoxFormat, uint64_t);

GVOX_CONSTANTS_BEGIN(GvoxStandardFormat)

// Integer formats
GVOX_CONSTANT(GVOX_FORMAT_R8_UINT, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_RAW, 1, 8, 0, 0, 0))
GVOX_CONSTANT(GVOX_FORMAT_R16_UINT, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_RAW, 1, 16, 0, 0, 0))
GVOX_CONSTANT(GVOX_FORMAT_R32_UINT, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_RAW, 1, 32, 0, 0, 0))
// Floating-point formats
GVOX_CONSTANT(GVOX_FORMAT_R8G8B8_SRGB, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_SRGB, 3, 8, 8, 8, 0))
GVOX_CONSTANT(GVOX_FORMAT_R8G8B8A8_SRGB, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_SRGB, 4, 8, 8, 8, 8))
GVOX_CONSTANT(GVOX_FORMAT_R8G8B8_UNORM, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_UNORM, 3, 8, 8, 8, 0))
GVOX_CONSTANT(GVOX_FORMAT_E5B9G9R9_UFLOAT, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_SHARED_EXP_UFLOAT_BGR, 4, 5, 9, 9, 9))
GVOX_CONSTANT(GVOX_FORMAT_R8G8B8_OCT_UNIT, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_UNIT, 3, 8, 8, 8, 0))
GVOX_CONSTANT(GVOX_FORMAT_R12G12B12_OCT_UNIT, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_UNIT, 3, 12, 12, 12, 0))
GVOX_CONSTANT(GVOX_FORMAT_R16G16B16_OCT_UNIT, GVOX_CREATE_FORMAT(GVOX_FORMAT_ENCODING_OCTAHEDRAL_UNIT, 3, 16, 16, 16, 0))

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

GVOX_EXPORT GvoxResult gvox_create_voxel_desc(GvoxVoxelDescCreateInfo const *info, GvoxVoxelDesc *handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT void gvox_destroy_voxel_desc(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT uint32_t gvox_voxel_desc_size_in_bits(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB;
GVOX_EXPORT uint32_t gvox_voxel_desc_attribute_count(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB;

GVOX_EXPORT uint8_t gvox_voxel_desc_compare(GvoxVoxelDesc desc_a, GvoxVoxelDesc desc_b) GVOX_FUNC_ATTRIB;

#endif
