#include <gvox/gvox.h>
#include <gvox/containers/openvdb.h>

#include "../utils/tracy.hpp"
#include "gvox/core.h"

#include <cstdlib>
#include <cstdint>
#include <fstream>
#include <new>

#include <openvdb/openvdb.h>
#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/io/Stream.h>

struct OpenVDBContainer {
    openvdb::Int32Grid::Ptr grid_ptr;
    GvoxVoxelDesc voxel_desc;
};

auto gvox_container_openvdb_description() GVOX_FUNC_ATTRIB->GvoxContainerDescription {
    return GvoxContainerDescription{
        .create = [](void **out_self, GvoxContainerCreateCbArgs const *args) -> GvoxResult {
            openvdb::initialize();
            GvoxOpenvdbContainerConfig config;
            if (args->config != nullptr) {
                config = *static_cast<GvoxOpenvdbContainerConfig const *>(args->config);
            } else {
                config = {};
            }
            (*out_self) = new (std::nothrow) OpenVDBContainer({});
            auto &self = *static_cast<OpenVDBContainer *>(*out_self);
            auto const attribs = std::array{
                GvoxAttribute{
                    .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                    .next = nullptr,
                    .type = GVOX_ATTRIBUTE_TYPE_ALBEDO,
                    .format = GVOX_FORMAT_R8G8B8A8_SRGB,
                },
            };
            auto voxel_desc_info = GvoxVoxelDescCreateInfo{
                .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
                .next = nullptr,
                .attribute_count = static_cast<uint32_t>(attribs.size()),
                .attributes = attribs.data(),
            };
            auto res = gvox_create_voxel_desc(&voxel_desc_info, &self.voxel_desc);
            if (res != GVOX_SUCCESS) {
                delete &self;
                return res;
            }
            self.grid_ptr = openvdb::Int32Grid::create();
            return GVOX_SUCCESS;
        },
        .destroy = [](void *self_ptr) -> void {
            // auto &self = *static_cast<OpenVDBContainer *>(self_ptr);
            // auto ofile = std::ofstream("magica.vdb", std::ios_base::binary);
            // openvdb::GridPtrVecPtr grids(new openvdb::GridPtrVec);
            // grids->push_back(self.grid_ptr);
            // openvdb::io::Stream(ofile).write(*grids);
            delete static_cast<OpenVDBContainer *>(self_ptr);
        },
        .fill = [](void *self_ptr, void const *single_voxel_data, GvoxVoxelDesc src_voxel_desc, GvoxRange range) -> GvoxResult {
            auto &self = *static_cast<OpenVDBContainer *>(self_ptr);
            auto accessor = self.grid_ptr->getAccessor();

            // convert src data to be compatible with the dst_voxel_desc
            auto *converted_data = static_cast<void const *>(nullptr);
            // test to see if the input data is already compatible (basically if it's the same exact voxel desc)
            if (gvox_voxel_desc_compare(src_voxel_desc, self.voxel_desc) != 0) {
                converted_data = single_voxel_data;
            } else {
                // converted_data = convert_data(converted_data_stack);
            }

            auto uint_data = *static_cast<uint32_t const *>(converted_data);
            auto min_coord = openvdb::Coord(0, 0, 0);
            auto max_coord = openvdb::Coord(1, 1, 1);

            auto dim = std::min<uint32_t>(range.extent.axis_n, 3);

            // uint64_t total_voxel_i = 1;
            for (uint32_t i = 0; i < dim; ++i) {
                auto offset_i = static_cast<int32_t>(range.offset.axis[i]);
                auto extent_i = static_cast<int32_t>(range.extent.axis[i]);
                // total_voxel_i *= range.extent.axis[i];
                min_coord.asPointer()[i] = offset_i;
                max_coord.asPointer()[i] = offset_i + extent_i;
            }

            auto iter_coord = min_coord;

            // TODO: Investigate whether this access pattern is bad for openvdb
            //  - update: I looked for a little bit and it looks like createLevelSetBox
            //    is the intended way to construct a new grid with a bounding box,

            for (iter_coord.z() = min_coord.z(); iter_coord.z() < max_coord.z(); iter_coord.z()++) {
                for (iter_coord.y() = min_coord.y(); iter_coord.y() < max_coord.y(); iter_coord.y()++) {
                    for (iter_coord.x() = min_coord.x(); iter_coord.x() < max_coord.x(); iter_coord.x()++) {
                        accessor.setValue(iter_coord, std::bit_cast<int32_t>(uint_data));
                    }
                }
            }
            return GVOX_SUCCESS;
        },
        .sample = [](void *self_ptr, void *out_voxel_data, GvoxVoxelDesc out_voxel_desc, GvoxOffset offset) -> GvoxResult {
            auto &self = *static_cast<OpenVDBContainer *>(self_ptr);
            auto coord = openvdb::Coord(offset.axis[0], offset.axis[1], offset.axis[2]);
            auto accessor = self.grid_ptr->getAccessor();
            return gvox_translate_voxel(&accessor.getValue(coord), self.voxel_desc, out_voxel_data, out_voxel_desc);
        },
    };
}
