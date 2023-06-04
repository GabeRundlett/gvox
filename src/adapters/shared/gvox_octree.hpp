#pragma once

#include "math_helpers.hpp"
#include <vector>
#include <algorithm>

union OctreeNode {
    struct Parent {
        uint32_t child_pointer : 24;
        uint32_t leaf_mask : 8;

        auto operator==(Parent const &other) const -> bool = default;
    } parent;

    struct Leaf {
        uint32_t color;

        auto operator==(Leaf const &other) const -> bool = default;
    } leaf;
};

struct Octree {
    GvoxRegionRange range{};
    std::vector<OctreeNode> nodes{};

    uint32_t sample(OctreeNode::Parent const &self, uint32_t x, uint32_t y, uint32_t z, uint32_t depth = 0) const {
        auto child_size = (1u << ceil_log2(std::max({range.extent.x, range.extent.y, range.extent.z}))) / (2u << depth);
        if (child_size == 0) {
            return 0;
        }
        auto cx = x / child_size;
        auto cy = y / child_size;
        auto cz = z / child_size;
        auto child_offset = cx + cy * 2 + cz * 4;
        auto const &child = nodes[self.child_pointer + child_offset];
        bool is_leaf = (self.leaf_mask >> child_offset) & 1;
        if (is_leaf) {
            return child.leaf.color;
        } else {
            return sample(child.parent, x - cx * child_size, y - cy * child_size, z - cz * child_size, depth + 1);
        }
    }
};
