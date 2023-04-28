#include <gvox/gvox.h>
// #include <gvox/adapters/serialize/gvox_octree.h>
#include "../shared/gvox_octree.hpp"

#include <bit>
#include <variant>

struct OctreeUserState {
    GvoxRegionRange range{};
    std::vector<uint32_t> voxels;
    size_t offset{};
    uint32_t max_depth{};
};

using OctreeTempNode = std::variant<OctreeNode::Parent, OctreeNode::Leaf>;

// Base
extern "C" void gvox_serialize_adapter_gvox_octree_create(GvoxAdapterContext *ctx, void const * /*unused*/) {
    auto *user_state_ptr = malloc(sizeof(OctreeUserState));
    [[maybe_unused]] auto &user_state = *(new (user_state_ptr) OctreeUserState());
    gvox_adapter_set_user_pointer(ctx, user_state_ptr);
}

extern "C" void gvox_serialize_adapter_gvox_octree_destroy(GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<OctreeUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.~OctreeUserState();
    free(&user_state);
}

extern "C" void gvox_serialize_adapter_gvox_octree_blit_begin(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t channel_flags) {
    if (std::popcount(range->extent.x) != 1 ||
        std::popcount(range->extent.y) != 1 ||
        std::popcount(range->extent.z) != 1 ||
        range->extent.x != range->extent.y ||
        range->extent.x != range->extent.z) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "In order to serialize as an OctreeNode, you must have a volume with equal power of 2 side lengths");
        return;
    }

    if ((channel_flags & ~static_cast<uint32_t>(GVOX_CHANNEL_BIT_COLOR)) != 0u) {
        gvox_adapter_push_error(ctx, GVOX_RESULT_ERROR_PARSE_ADAPTER_INVALID_INPUT, "The current OctreeNode impl can't support anything other than color");
        return;
    }

    auto &user_state = *static_cast<OctreeUserState *>(gvox_adapter_get_user_pointer(ctx));
    user_state.offset = 0;
    user_state.range = *range;
    auto magic = std::bit_cast<uint32_t>(std::array<char, 4>{'o', 'c', 't', '\0'});
    gvox_output_write(blit_ctx, user_state.offset, sizeof(uint32_t), &magic);
    user_state.offset += sizeof(magic);
    gvox_output_write(blit_ctx, user_state.offset, sizeof(*range), range);
    user_state.offset += sizeof(*range);

    user_state.max_depth = ceil_log2(range->extent.x);

    user_state.voxels.resize(range->extent.x * range->extent.y * range->extent.z);
}

void do_output(OctreeUserState &user_state, uint32_t my_output_index, std::vector<OctreeNode> &output, std::vector<std::vector<OctreeTempNode>> const &levels, uint32_t depth, uint32_t max_depth, uint32_t x, uint32_t y, uint32_t z) {
    if (depth == max_depth) {
        auto sample_child = [&user_state](uint32_t x, uint32_t y, uint32_t z) {
            auto grid_size = user_state.range.extent.x;
            auto index = x + y * grid_size + z * grid_size * grid_size;
            return user_state.voxels[index];
        };
        output[my_output_index] = OctreeNode{.leaf = {.color = sample_child(x, y, z)}};
        return;
    }
    auto level_i = max_depth - 1 - depth;
    auto node_size = 2u << level_i;
    auto grid_size = user_state.range.extent.x / node_size;
    auto const &my_gvox_octree = levels[level_i][x + y * grid_size + z * grid_size * grid_size];
    if (std::holds_alternative<OctreeNode::Parent>(my_gvox_octree)) {
        auto const &my_node = std::get<OctreeNode::Parent>(my_gvox_octree);
        auto children_index = static_cast<uint32_t>(output.size());
        output[my_output_index] = OctreeNode{.parent = my_node};
        output[my_output_index].parent.child_pointer = children_index;
        for (uint32_t child_i = 0; child_i < 8; ++child_i) {
            output.push_back(OctreeNode{});
        }
        for (uint32_t czi = 0; czi < 2; ++czi) {
            for (uint32_t cyi = 0; cyi < 2; ++cyi) {
                for (uint32_t cxi = 0; cxi < 2; ++cxi) {
                    auto child_i = cxi + cyi * 2 + czi * 4;
                    do_output(user_state, children_index + child_i, output, levels, depth + 1, max_depth, x * 2 + cxi, y * 2 + cyi, z * 2 + czi);
                }
            }
        }
    } else {
        output[my_output_index] = OctreeNode{.leaf = std::get<OctreeNode::Leaf>(my_gvox_octree)};
    }
}

extern "C" void gvox_serialize_adapter_gvox_octree_blit_end(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx) {
    auto &user_state = *static_cast<OctreeUserState *>(gvox_adapter_get_user_pointer(ctx));

    auto levels = std::vector<std::vector<OctreeTempNode>>{};
    levels.resize(user_state.max_depth);

    for (uint32_t level_i = 0; level_i < user_state.max_depth; ++level_i) {
        auto node_size = 2u << level_i;
        auto grid_size = user_state.range.extent.x / node_size;
        levels[level_i].resize(grid_size * grid_size * grid_size);
        for (uint32_t zi = 0; zi < grid_size; ++zi) {
            for (uint32_t yi = 0; yi < grid_size; ++yi) {
                for (uint32_t xi = 0; xi < grid_size; ++xi) {
                    if (level_i == 0) {
                        std::array<uint32_t, 8> children{};
                        auto sample_child = [&user_state, grid_size](uint32_t x, uint32_t y, uint32_t z) {
                            auto index = x + y * grid_size * 2 + z * grid_size * 2 * grid_size * 2;
                            return user_state.voxels[index];
                        };
                        for (uint32_t child_i = 0; child_i < 8; ++child_i) {
                            children[child_i] = sample_child(xi * 2 + (child_i & 1), yi * 2 + ((child_i / 2) & 1), zi * 2 + ((child_i / 4) & 1));
                        }
                        bool is_uniform = true;
                        for (uint32_t child_i = 1; child_i < 8; ++child_i) {
                            if (children[child_i] != children[0]) {
                                is_uniform = false;
                                break;
                            }
                        }
                        if (is_uniform) {
                            levels[level_i][xi + yi * grid_size + zi * grid_size * grid_size] = OctreeNode::Leaf{
                                .color = children[0],
                            };
                        } else {
                            levels[level_i][xi + yi * grid_size + zi * grid_size * grid_size] = OctreeNode::Parent{
                                .child_pointer = 0,
                                .leaf_mask = 0xff,
                            };
                        }
                    } else {
                        std::array<OctreeTempNode, 8> children{};
                        auto sample_child = [&levels, level_i, grid_size](uint32_t x, uint32_t y, uint32_t z) {
                            auto index = x + y * grid_size * 2 + z * grid_size * 2 * grid_size * 2;
                            return levels[level_i - 1][index];
                        };
                        for (uint32_t child_i = 0; child_i < 8; ++child_i) {
                            children[child_i] = sample_child(xi * 2 + (child_i & 1), yi * 2 + ((child_i / 2) & 1), zi * 2 + ((child_i / 4) & 1));
                        }
                        bool is_uniform = true;
                        uint32_t leaf_mask = 0;
                        for (uint32_t child_i = 0; child_i < 8; ++child_i) {
                            if (std::holds_alternative<OctreeNode::Parent>(children[child_i])) {
                                is_uniform = false;
                            } else {
                                leaf_mask |= 1 << child_i;
                                if (children[child_i] != children[0]) {
                                    is_uniform = false;
                                }
                            }
                        }
                        if (is_uniform) {
                            levels[level_i][xi + yi * grid_size + zi * grid_size * grid_size] = children[0];
                        } else {
                            levels[level_i][xi + yi * grid_size + zi * grid_size * grid_size] = OctreeNode::Parent{
                                .child_pointer = 0,
                                .leaf_mask = leaf_mask,
                            };
                        }
                    }
                }
            }
        }
    }

    auto output = std::vector<OctreeNode>{};
    output.push_back(OctreeNode{});
    do_output(user_state, 0, output, levels, 0, user_state.max_depth, 0, 0, 0);

    auto node_count = static_cast<uint32_t>(output.size());
    gvox_output_write(blit_ctx, user_state.offset, sizeof(node_count), &node_count);
    user_state.offset += sizeof(node_count);

    gvox_output_write(blit_ctx, user_state.offset, output.size() * sizeof(output[0]), output.data());
}

static void handle_region(OctreeUserState &user_state, GvoxRegionRange const *range, auto user_func) {
    for (uint32_t zi = 0; zi < range->extent.z; ++zi) {
        for (uint32_t yi = 0; yi < range->extent.y; ++yi) {
            for (uint32_t xi = 0; xi < range->extent.x; ++xi) {
                auto const pos = GvoxOffset3D{
                    static_cast<int32_t>(xi) + range->offset.x,
                    static_cast<int32_t>(yi) + range->offset.y,
                    static_cast<int32_t>(zi) + range->offset.z,
                };
                if (pos.x < user_state.range.offset.x ||
                    pos.y < user_state.range.offset.y ||
                    pos.z < user_state.range.offset.z ||
                    pos.x >= user_state.range.offset.x + static_cast<int32_t>(user_state.range.extent.x) ||
                    pos.y >= user_state.range.offset.y + static_cast<int32_t>(user_state.range.extent.y) ||
                    pos.z >= user_state.range.offset.z + static_cast<int32_t>(user_state.range.extent.z)) {
                    continue;
                }
                auto output_rel_x = static_cast<size_t>(pos.x - user_state.range.offset.x);
                auto output_rel_y = static_cast<size_t>(pos.y - user_state.range.offset.y);
                auto output_rel_z = static_cast<size_t>(pos.z - user_state.range.offset.z);
                auto output_index = static_cast<size_t>(output_rel_x + output_rel_y * user_state.range.extent.x + output_rel_z * user_state.range.extent.x * user_state.range.extent.y);
                user_func(output_index, pos);
            }
        }
    }
}

// Serialize Driven
extern "C" void gvox_serialize_adapter_gvox_octree_serialize_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegionRange const *range, uint32_t /* channel_flags */) {
    auto &user_state = *static_cast<OctreeUserState *>(gvox_adapter_get_user_pointer(ctx));
    handle_region(
        user_state, range,
        [blit_ctx, &user_state](size_t output_index, GvoxOffset3D const &pos) {
            auto const sample_range = GvoxRegionRange{
                .offset = pos,
                .extent = GvoxExtent3D{1, 1, 1},
            };
            auto region = gvox_load_region_range(blit_ctx, &sample_range, GVOX_CHANNEL_BIT_COLOR);
            auto sample = gvox_sample_region(blit_ctx, &region, &pos, GVOX_CHANNEL_ID_COLOR);
            if (sample.is_present == 0u) {
                sample.data = 0u;
            }
            user_state.voxels[output_index] = sample.data;
            gvox_unload_region_range(blit_ctx, &region, &sample_range);
        });
}

// Parse Driven
extern "C" void gvox_serialize_adapter_gvox_octree_receive_region(GvoxBlitContext *blit_ctx, GvoxAdapterContext *ctx, GvoxRegion const *region) {
    auto &user_state = *static_cast<OctreeUserState *>(gvox_adapter_get_user_pointer(ctx));
    handle_region(
        user_state, &region->range,
        [blit_ctx, region, &user_state](size_t output_index, GvoxOffset3D const &pos) {
            auto sample = gvox_sample_region(blit_ctx, region, &pos, GVOX_CHANNEL_ID_COLOR);
            if (sample.is_present != 0u) {
                user_state.voxels[output_index] = sample.data;
            }
        });
}
