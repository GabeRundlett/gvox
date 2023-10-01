#include "types.hpp"
#include "utils/tracy.hpp"
#include "utils/handle.hpp"

#include <bit>

auto gvox_create_voxel_desc(GvoxVoxelDescCreateInfo const *info, GvoxVoxelDesc *handle) GVOX_FUNC_ATTRIB->GvoxResult {
    ZoneScoped;
    HANDLE_NEW(VoxelDesc, VOXEL_DESC);

    if (info->struct_type != GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    auto &result = *reinterpret_cast<IMPL_STRUCT_NAME(VoxelDesc) *>(*handle);

    for (size_t i = 0; i < info->attribute_count; ++i) {
        auto const &in_attrib = info->attributes[i];
        if (in_attrib.struct_type != GVOX_STRUCT_TYPE_ATTRIBUTE) {
            return GVOX_ERROR_BAD_STRUCT_TYPE;
        }
        auto const format_desc = std::bit_cast<FormatDescriptor>(in_attrib.format);
        auto attribute_size = static_cast<uint32_t>(format_desc.c0_bit_count) + format_desc.c1_bit_count + format_desc.c2_bit_count + format_desc.c3_bit_count;
        result.attributes.push_back({
            .bit_count = attribute_size,
            .bit_offset = result.bit_count, // currently accumulating the size and therefore is the offset!
            .type = in_attrib.type,
            .format_desc = format_desc,
        });
        result.bit_count += attribute_size;
    }

    return GVOX_SUCCESS;
}
void gvox_destroy_voxel_desc(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB {
    destroy_handle(handle);
}

auto gvox_voxel_desc_size_in_bits(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB->uint32_t {
    auto &voxel_desc = *reinterpret_cast<IMPL_STRUCT_NAME(VoxelDesc) *>(handle);
    return voxel_desc.bit_count;
}
auto gvox_voxel_desc_attribute_count(GvoxVoxelDesc handle) GVOX_FUNC_ATTRIB->uint32_t {
    auto &voxel_desc = *reinterpret_cast<IMPL_STRUCT_NAME(VoxelDesc) *>(handle);
    return static_cast<uint32_t>(voxel_desc.attributes.size());
}

auto gvox_voxel_desc_compare(GvoxVoxelDesc desc_a, GvoxVoxelDesc desc_b) GVOX_FUNC_ATTRIB->uint8_t {
    if (desc_a == desc_b) {
        return 2;
    }
    if (desc_a->attributes.size() != desc_b->attributes.size()) {
        return 0;
    }
    for (size_t i = 0; i < desc_a->attributes.size(); ++i) {
        if (memcmp(&desc_a->attributes[i], &desc_b->attributes[i], sizeof(Attribute)) != 0) {
            return 0;
        }
    }
    return 1;
}

#if 0
void gvox_translate_voxels(
    GvoxVoxelDesc src_desc, uint32_t src_n, void *src_data,
    GvoxVoxelDesc dst_desc, void *dst_data) {
    auto dst_sources = std::vector<uint32_t>(dst_desc->attributes.size());
    auto find_most_suitable_src_index = [](GvoxVoxelDesc src_desc, Attribute const &dst_attrib) -> uint32_t {
        for (size_t attrib_i = 0; attrib_i < src_desc->attributes.size(); ++attrib_i) {
            // TODO: Improve this metric!
            if (src_desc->attributes[attrib_i].type == dst_attrib.type) {
                return static_cast<uint32_t>(attrib_i);
            }
        }
        return 0xffffffff;
    };
    for (size_t attrib_i = 0; attrib_i < dst_sources.size(); ++attrib_i) {
        dst_sources[attrib_i] = find_most_suitable_src_index(src_desc, dst_desc->attributes[attrib_i]);
    }

    for (size_t voxel_i = 0; voxel_i < src_n; ++voxel_i) {
        auto src_voxel_bit_offset = voxel_i * src_desc->bit_count;
        auto dst_voxel_bit_offset = voxel_i * dst_desc->bit_count;
        for (size_t attrib_i = 0; attrib_i < dst_sources.size(); ++attrib_i) {
            auto src_attrib_i = dst_sources[attrib_i];
            if (src_attrib_i != 0xffffffff) {
                auto const &src_attrib = src_desc->attributes[dst_sources[attrib_i]];
                auto const &dst_attrib = dst_desc->attributes[attrib_i];
                auto src_attrib_bit_offset = src_voxel_bit_offset + src_attrib.bit_offset;
                auto dst_attrib_bit_offset = dst_voxel_bit_offset + dst_attrib.bit_offset;
                //
            }
        }
        static_cast<uint64_t *>(src_data);
    }
}
#endif
