#pragma once

#include "../common/math.hpp"

#include <numeric>
#include <vector>
#include <span>

union VoxelOctreeNode {
    struct Branch {
        uint32_t child_valid_mask : 8;
        uint32_t child_is_leaf_mask : 8;
        uint32_t children_ptr : 16;
    } branch;
    struct Leaf {
        uint32_t data;
    };
};

struct VoxelBvhChunkRoot {
    uint64_t aabb_min_x : 6;
    uint64_t aabb_min_y : 6;
    uint64_t aabb_min_z : 6;
    uint64_t aabb_max_x : 6;
    uint64_t aabb_max_y : 6;
    uint64_t aabb_max_z : 6;
    uint64_t left_node_index : 26;
    uint64_t child_is_leaf : 2;
};
struct VoxelBvhNode5 {
    uint64_t aabb_min_x : 5;
    uint64_t aabb_min_y : 5;
    uint64_t aabb_min_z : 5;
    uint64_t aabb_max_x : 5;
    uint64_t aabb_max_y : 5;
    uint64_t aabb_max_z : 5;
    uint64_t left_node_index : 16;
    uint64_t child_is_leaf : 2;
};
struct VoxelBvhNode4 {
    uint64_t aabb_min_x : 4;
    uint64_t aabb_min_y : 4;
    uint64_t aabb_min_z : 4;
    uint64_t aabb_max_x : 4;
    uint64_t aabb_max_y : 4;
    uint64_t aabb_max_z : 4;
    uint64_t left_node_index : 14;
    uint64_t child_is_leaf : 2;
};
struct VoxelBvhNode3 {
    uint32_t aabb_min_x : 3;
    uint32_t aabb_min_y : 3;
    uint32_t aabb_min_z : 3;
    uint32_t aabb_max_x : 3;
    uint32_t aabb_max_y : 3;
    uint32_t aabb_max_z : 3;
    uint32_t left_node_index : 12;
    uint32_t child_is_leaf : 2;
};
struct VoxelBvhNode2 {
    uint32_t aabb_min_x : 2;
    uint32_t aabb_min_y : 2;
    uint32_t aabb_min_z : 2;
    uint32_t aabb_max_x : 2;
    uint32_t aabb_max_y : 2;
    uint32_t aabb_max_z : 2;
    uint32_t left_node_index : 10;
    uint32_t child_is_leaf : 2;
};
struct VoxelBvhNode1 {
    uint16_t aabb_min_x : 1;
    uint16_t aabb_min_y : 1;
    uint16_t aabb_min_z : 1;
    uint16_t aabb_max_x : 1;
    uint16_t aabb_max_y : 1;
    uint16_t aabb_max_z : 1;
    uint16_t left_node_index : 8;
    uint16_t child_is_leaf : 2;
};

struct BvhNode {
    Vec3 aabbMin, aabbMax;
    uint32_t leftNode, firstTriIdx, triCount;
    [[nodiscard]] auto isLeaf() const -> bool { return triCount > 0; }
};

// auto const MAX_DEPTH_U = std::bit_cast<uint32_t>(MAX_DEPTH);
// auto const N = 128;

struct Bvh {
    std::vector<BvhNode> nodes;
    std::vector<uint32_t> tri_indices;
};

void update_node_bounds(Bvh &bvh, std::span<Triangle const> tris, uint32_t nodeIdx) {
    auto &node = bvh.nodes[nodeIdx];
    node.aabbMin = Vec3(1e30f);
    node.aabbMax = Vec3(-1e30f);
    for (auto first = node.firstTriIdx, i = 0u; i < node.triCount; i++) {
        auto leafTriIdx = bvh.tri_indices[first + i];
        auto const &leafTri = tris[leafTriIdx];
        node.aabbMin = min(node.aabbMin, leafTri[0]),
        node.aabbMin = min(node.aabbMin, leafTri[1]),
        node.aabbMin = min(node.aabbMin, leafTri[2]),
        node.aabbMax = max(node.aabbMax, leafTri[0]),
        node.aabbMax = max(node.aabbMax, leafTri[1]),
        node.aabbMax = max(node.aabbMax, leafTri[2]);
    }
}

void subdivide(Bvh &bvh, std::span<Triangle const> tris, std::span<Vec3 const> centroids, uint32_t nodeIdx, uint32_t &nodes_used) {
    // terminate recursion
    auto &node = bvh.nodes[nodeIdx];
    if (node.triCount <= 2)
        return;
    // determine split axis and position
    Vec3 extent = node.aabbMax - node.aabbMin;
    int axis = 0;
    if (extent.y > extent.x)
        axis = 1;
    if (extent.z > extent[axis])
        axis = 2;
    float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
    // in-place partition
    int i = node.firstTriIdx;
    int j = i + node.triCount - 1;
    while (i <= j) {
        if (centroids[bvh.tri_indices[i]][axis] < splitPos)
            i++;
        else
            std::swap(bvh.tri_indices[i], bvh.tri_indices[j--]);
    }
    // abort split if one of the sides is empty
    int leftCount = i - node.firstTriIdx;
    if (leftCount == 0 || leftCount == node.triCount)
        return;
    // create child nodes
    int leftChildIdx = nodes_used++;
    int rightChildIdx = nodes_used++;
    bvh.nodes[leftChildIdx].firstTriIdx = node.firstTriIdx;
    bvh.nodes[leftChildIdx].triCount = leftCount;
    bvh.nodes[rightChildIdx].firstTriIdx = i;
    bvh.nodes[rightChildIdx].triCount = node.triCount - leftCount;
    node.leftNode = leftChildIdx;
    node.triCount = 0;
    update_node_bounds(bvh, tris, leftChildIdx);
    update_node_bounds(bvh, tris, rightChildIdx);
    // recurse
    subdivide(bvh, tris, centroids, leftChildIdx, nodes_used);
    subdivide(bvh, tris, centroids, rightChildIdx, nodes_used);
}

auto build_bvh(std::span<Triangle const> tris) -> Bvh {
    auto const N = tris.size();
    Bvh bvh;
    bvh.nodes.resize(N * 2 - 1);
    bvh.tri_indices.resize(N);
    std::iota(bvh.tri_indices.begin(), bvh.tri_indices.end(), 0);
    uint32_t rootNodeIdx = 0, nodes_used = 1;
    std::vector<Vec3> centroids;
    centroids.resize(N);
    for (int i = 0; i < N; i++) {
        centroids[i] = (tris[i][0] + tris[i][1] + tris[i][2]) * 0.3333f;
    }
    // assign all triangles to root node
    auto &root = bvh.nodes[rootNodeIdx];
    root.leftNode = 0;
    root.firstTriIdx = 0, root.triCount = N;
    update_node_bounds(bvh, tris, rootNodeIdx);
    // subdivide recursively
    subdivide(bvh, tris, centroids, rootNodeIdx, nodes_used);
    return bvh;
}

void intersect_bvh(Bvh const &bvh, std::span<Triangle const> tris, Ray const &ray, IntersectionRecord &ir, const uint32_t nodeIdx) {
    auto const &node = bvh.nodes[nodeIdx];
    auto temp_ir = IntersectionRecord{};
    intersect_aabb(ray, temp_ir, node.aabbMin, node.aabbMax);
    if (temp_ir.t >= ir.t)
        return;
    if (node.isLeaf()) {
        for (uint32_t i = 0; i < node.triCount; i++)
            intersect_tri(ray, ir, tris[bvh.tri_indices[node.firstTriIdx + i]]);
    } else {
        intersect_bvh(bvh, tris, ray, ir, node.leftNode);
        intersect_bvh(bvh, tris, ray, ir, node.leftNode + 1);
    }
}
