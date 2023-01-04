#include <gvox/gvox.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <unordered_set>
#include <array>
#include <vector>

#include <iostream>

#if GVOX_FORMAT_GVOX_U32_PALETTE_BUILT_STATIC
#define EXPORT
#else
#if __linux__
#define EXPORT
#elif _WIN32
#define EXPORT __declspec(dllexport)
#endif
#endif

static constexpr auto REGION_SIZE = 8;

static constexpr auto ceil_log2(uint32_t x) -> uint32_t {
    constexpr auto const t = std::array<uint32_t, 6>{
        0x00000000u,
        0xFFFF0000u,
        0x0000FF00u,
        0x000000F0u,
        0x0000000Cu,
        0x00000002u};

    uint32_t y = (((x & (x - 1)) == 0) ? 0 : 1);
    int j = 32;

    for (uint32_t const i : t) {
        int const k = (((x & i) == 0) ? 0 : j);
        y += static_cast<uint32_t>(k);
        x >>= k;
        j >>= 1;
    }

    return y;
}

template <typename T>
static void write_data(uint8_t *&buffer_ptr, T const &data) {
    *reinterpret_cast<T *>(buffer_ptr) = data;
    buffer_ptr += sizeof(T);
}

template <typename T>
static auto read_data(uint8_t *&buffer_ptr) -> T {
    auto &result = *reinterpret_cast<T *>(buffer_ptr);
    buffer_ptr += sizeof(T);
    return result;
}

constexpr auto get_mask(uint32_t bits_per_variant) -> uint32_t {
    return (~0u) >> (32 - bits_per_variant);
}

static constexpr auto calc_palette_region_size(size_t bits_per_variant) -> size_t {
    auto palette_region_size = (bits_per_variant * REGION_SIZE * REGION_SIZE * REGION_SIZE + 7) / 8;
    palette_region_size = (palette_region_size + 3) / 4;
    auto size = palette_region_size + 1;
    return size * 4;
}

struct RegionHeader {
    uint32_t variant_n;
    uint32_t blob_offset; // if variant_n == 1, this is just the voxel
};

struct NodeHeader {
    uint32_t node_full_size;

    uint32_t region_count_x;
    uint32_t region_count_y;
    uint32_t region_count_z;
};

struct PaletteBuffer {
    NodeHeader node_header;
    // RegionHeader *region_headers;
    // uint32_t *data;
};

struct PaletteCompressor {
    std::vector<uint8_t> data;

    uint32_t region_nx;
    uint32_t region_ny;
    uint32_t region_nz;

    auto region_variance(GVoxSceneNode const &node, size_t region_x, size_t region_y, size_t region_z) -> size_t {
        auto tile_set = std::unordered_set<uint32_t>{};
        auto const ox = region_x * REGION_SIZE;
        auto const oy = region_y * REGION_SIZE;
        auto const oz = region_z * REGION_SIZE;
        // std::cout << "o: " << ox << " " << oy << " " << oz << "\t";

        for (size_t zi = 0; zi < REGION_SIZE; ++zi) {
            for (size_t yi = 0; yi < REGION_SIZE; ++yi) {
                for (size_t xi = 0; xi < REGION_SIZE; ++xi) {
                    // TODO: fix for non-multiple sizes of REGION_SIZE
                    auto const px = ox + xi;
                    auto const py = oy + yi;
                    auto const pz = oz + zi;

                    auto u32_voxel = 0u;

                    if (px < node.size_x && py < node.size_y && pz < node.size_z) {
                        auto const index = px + py * node.size_x + pz * node.size_x * node.size_y;
                        auto const &i_vox = node.voxels[index];
                        auto const r = static_cast<uint32_t>(std::max(std::min(i_vox.color.x, 1.0f), 0.0f) * 255.0f);
                        auto const g = static_cast<uint32_t>(std::max(std::min(i_vox.color.y, 1.0f), 0.0f) * 255.0f);
                        auto const b = static_cast<uint32_t>(std::max(std::min(i_vox.color.z, 1.0f), 0.0f) * 255.0f);
                        auto const i = i_vox.id;
                        u32_voxel = u32_voxel | (r << 0x00);
                        u32_voxel = u32_voxel | (g << 0x08);
                        u32_voxel = u32_voxel | (b << 0x10);
                        u32_voxel = u32_voxel | (i << 0x18);
                    }

                    tile_set.insert(u32_voxel);
                }
            }
        }

        auto const variants = static_cast<uint32_t>(tile_set.size());
        auto const bits_per_variant = ceil_log2(variants);

        size_t size = 0;

        RegionHeader region_header{.variant_n = variants};

        if (variants > 1) {
            // insert palette
            size += sizeof(uint32_t) * variants;
            // insert palette region
            size += calc_palette_region_size(bits_per_variant);

            auto const old_size = data.size();
            region_header.blob_offset = static_cast<uint32_t>(old_size - (sizeof(NodeHeader) + (region_nx * region_ny * region_nz) * sizeof(RegionHeader)));

            data.reserve(old_size + size);
            for (size_t i = 0; i < size; ++i) {
                data.push_back(0);
            }
            uint8_t *output_buffer = data.data() + old_size;

            auto *palette_begin = (uint32_t *)output_buffer;
            uint32_t *palette_end = palette_begin + variants;

            for (auto u32_voxel : tile_set) {
                write_data<uint32_t>(output_buffer, u32_voxel);
            }
            for (size_t zi = 0; zi < REGION_SIZE; ++zi) {
                for (size_t yi = 0; yi < REGION_SIZE; ++yi) {
                    for (size_t xi = 0; xi < REGION_SIZE; ++xi) {
                        auto const px = ox + xi;
                        auto const py = oy + yi;
                        auto const pz = oz + zi;
                        auto const in_region_index = xi + yi * REGION_SIZE + zi * REGION_SIZE * REGION_SIZE;
                        auto u32_voxel = 0u;
                        if (px < node.size_x && py < node.size_y && pz < node.size_z) {
                            auto const index = px + py * node.size_x + pz * node.size_x * node.size_y;
                            auto const &i_vox = node.voxels[index];
                            auto const r = static_cast<uint32_t>(std::max(std::min(i_vox.color.x, 1.0f), 0.0f) * 255.0f);
                            auto const g = static_cast<uint32_t>(std::max(std::min(i_vox.color.y, 1.0f), 0.0f) * 255.0f);
                            auto const b = static_cast<uint32_t>(std::max(std::min(i_vox.color.z, 1.0f), 0.0f) * 255.0f);
                            auto const i = i_vox.id;
                            u32_voxel = u32_voxel | (r << 0x00);
                            u32_voxel = u32_voxel | (g << 0x08);
                            u32_voxel = u32_voxel | (b << 0x10);
                            u32_voxel = u32_voxel | (i << 0x18);
                        }
                        auto *palette_iter = std::find(palette_begin, palette_end, u32_voxel);
                        assert(palette_iter != palette_end);
                        auto const palette_id = static_cast<size_t>(palette_iter - palette_begin);
                        auto const bit_index = static_cast<size_t>(in_region_index * bits_per_variant);
                        auto const byte_index = bit_index / 8;
                        auto const bit_offset = static_cast<uint32_t>(bit_index - byte_index * 8);
                        auto const mask = get_mask(bits_per_variant);
                        assert(output_buffer + byte_index + 3 < data.data() + data.size());
                        auto &output = *reinterpret_cast<uint32_t *>(output_buffer + byte_index);
                        output = output & ~(mask << bit_offset);
                        output = output | static_cast<uint32_t>(palette_id << bit_offset);
                    }
                }
            }
        } else {
            region_header.blob_offset = *tile_set.begin();
        }

        auto region_header_ptr = data.data() + sizeof(NodeHeader) + (region_x + region_y * region_nx + region_z * region_nx * region_ny) * sizeof(RegionHeader);
        write_data<RegionHeader>(region_header_ptr, region_header);

        return size;
    }

    auto node_precomp(GVoxSceneNode const &node) -> size_t {
        // allocate at least 5% of the original node
        size_t const old_size = data.size();
        data.reserve(old_size + (node.size_x * node.size_y * node.size_z * sizeof(uint32_t)) / 20);

        region_nx = static_cast<uint32_t>((node.size_x + REGION_SIZE - 1) / REGION_SIZE);
        region_ny = static_cast<uint32_t>((node.size_y + REGION_SIZE - 1) / REGION_SIZE);
        region_nz = static_cast<uint32_t>((node.size_z + REGION_SIZE - 1) / REGION_SIZE);

        size_t pre_size =
            sizeof(NodeHeader) +
            sizeof(RegionHeader) * (region_nx * region_ny * region_nz);
        size_t size = pre_size;

        data.reserve(old_size + size);
        for (size_t i = 0; i < size; ++i) {
            data.push_back(0);
        }

        {
            uint8_t *output_buffer = data.data() + old_size;
            write_data<NodeHeader>(output_buffer, NodeHeader{0u, region_nx, region_ny, region_nz});
        }

        for (size_t zi = 0; zi < region_nz; ++zi) {
            for (size_t yi = 0; yi < region_ny; ++yi) {
                for (size_t xi = 0; xi < region_nx; ++xi) {
                    size += region_variance(node, xi, yi, zi);
                }
            }
        }

        {
            uint8_t *output_buffer = data.data() + old_size;
            write_data<uint32_t>(output_buffer, static_cast<uint32_t>(size - pre_size));
        }

        return size;
    }

    auto create(GVoxScene const &scene) -> GVoxPayload {
        GVoxPayload result = {};
        result.size += sizeof(uint32_t);
        auto pre_node_size = result.size;
        for (uint32_t node_i = 0; node_i < scene.node_n; ++node_i) {
            if (scene.nodes[node_i].voxels == nullptr) {
                continue;
            }
            result.size += node_precomp(scene.nodes[node_i]);
        }
        result.data = new uint8_t[result.size];
        auto *buffer_ptr = result.data;
        write_data<uint32_t>(buffer_ptr, static_cast<uint32_t>(scene.node_n));
        std::memcpy(buffer_ptr, data.data(), result.size - pre_node_size);
        return result;
    }
};

struct GVoxU32PaletteContext {
    GVoxU32PaletteContext();
    ~GVoxU32PaletteContext() = default;

    static auto create_payload(GVoxScene scene) -> GVoxPayload;
    static void destroy_payload(GVoxPayload payload);
    static auto parse_payload(GVoxPayload payload) -> GVoxScene;
};

GVoxU32PaletteContext::GVoxU32PaletteContext() = default;

auto GVoxU32PaletteContext::create_payload(GVoxScene scene) -> GVoxPayload {
    PaletteCompressor compressor;
    return compressor.create(scene);
}

void GVoxU32PaletteContext::destroy_payload(GVoxPayload payload) {
    delete[] payload.data;
}

auto u32_to_voxel(uint32_t u32_voxel) -> GVoxVoxel {
    float r = static_cast<float>((u32_voxel >> 0x00) & 0xff) / 255.0f;
    float g = static_cast<float>((u32_voxel >> 0x08) & 0xff) / 255.0f;
    float b = static_cast<float>((u32_voxel >> 0x10) & 0xff) / 255.0f;
    uint32_t const i = (u32_voxel >> 0x18) & 0xff;
    return GVoxVoxel{
        .color = {r, g, b},
        .id = i,
    };
}

void print_voxel(GVoxVoxel const &vox) {
    printf("\033[38;2;%03d;%03d;%03dm", (uint32_t)(vox.color.x * 255), (uint32_t)(vox.color.y * 255), (uint32_t)(vox.color.z * 255));
    printf("\033[48;2;%03d;%03d;%03dm", (uint32_t)(vox.color.x * 255), (uint32_t)(vox.color.y * 255), (uint32_t)(vox.color.z * 255));
    char const c = '_';
    fputc(c, stdout);
    fputc(c, stdout);
    printf("\033[0m");
}

auto GVoxU32PaletteContext::parse_payload(GVoxPayload payload) -> GVoxScene {
    GVoxScene result = {};
    auto *buffer_ptr = static_cast<uint8_t *>(payload.data);
    result.node_n = read_data<uint32_t>(buffer_ptr);

    result.nodes = (GVoxSceneNode *)std::malloc(sizeof(GVoxSceneNode) * result.node_n);
    for (size_t node_i = 0; node_i < result.node_n; ++node_i) {
        auto &node = result.nodes[node_i];
        auto const node_header = read_data<NodeHeader>(buffer_ptr);
        auto *region_headers_begin = buffer_ptr;
        node.size_x = node_header.region_count_x * REGION_SIZE;
        node.size_y = node_header.region_count_y * REGION_SIZE;
        node.size_z = node_header.region_count_z * REGION_SIZE;
        auto const region_n = node_header.region_count_x * node_header.region_count_y * node_header.region_count_z;
        buffer_ptr += region_n * sizeof(RegionHeader);
        auto *data_begin = buffer_ptr;

        // for (size_t region_i = 0; region_i < region_n; ++region_i) {
        //     auto const &region_header = reinterpret_cast<RegionHeader *>(region_headers_begin)[region_i];
        //     std::cout << region_header.variant_n << ", " << region_header.blob_offset << std::endl;
        // }
        // std::cout << std::flush;

        auto *next_node = buffer_ptr + node_header.node_full_size;
        size_t const voxels_n = node.size_x * node.size_y * node.size_z;
        size_t const voxels_size = voxels_n * sizeof(GVoxVoxel);
        node.voxels = (GVoxVoxel *)std::malloc(voxels_size);
        for (size_t region_z = 0; region_z < node_header.region_count_z; ++region_z) {
            for (size_t region_y = 0; region_y < node_header.region_count_y; ++region_y) {
                for (size_t region_x = 0; region_x < node_header.region_count_x; ++region_x) {
                    size_t const ox = region_x * REGION_SIZE;
                    size_t const oy = region_y * REGION_SIZE;
                    size_t const oz = region_z * REGION_SIZE;
                    auto *region_header_ptr = region_headers_begin + (region_x + region_y * node_header.region_count_x + region_z * node_header.region_count_x * node_header.region_count_y) * sizeof(RegionHeader);
                    auto const &region_header = read_data<RegionHeader>(region_header_ptr);
                    auto variants = region_header.variant_n;
                    if (variants == 1) {
                        auto const u32_voxel = region_header.blob_offset;
                        auto const r = static_cast<float>((u32_voxel >> 0x00) & 0xff) * 1.0f / 255.0f;
                        auto const g = static_cast<float>((u32_voxel >> 0x08) & 0xff) * 1.0f / 255.0f;
                        auto const b = static_cast<float>((u32_voxel >> 0x10) & 0xff) * 1.0f / 255.0f;
                        auto const i = (u32_voxel >> 0x18) & 0xff;
                        auto region_voxel = GVoxVoxel{
                            .color = {r, g, b},
                            .id = i,
                        };
                        for (size_t zi = 0; zi < REGION_SIZE; ++zi) {
                            for (size_t yi = 0; yi < REGION_SIZE; ++yi) {
                                for (size_t xi = 0; xi < REGION_SIZE; ++xi) {
                                    auto const px = ox + xi;
                                    auto const py = oy + yi;
                                    auto const pz = oz + zi;
                                    auto const index = px + py * node.size_x + pz * node.size_x * node.size_y;
                                    node.voxels[index] = region_voxel;
                                }
                            }
                        }
                    } else {
                        buffer_ptr = data_begin + region_header.blob_offset;
                        auto *palette_begin = reinterpret_cast<uint32_t *>(buffer_ptr);
                        auto const bits_per_variant = ceil_log2(variants);
                        assert(bits_per_variant <= 9);
                        buffer_ptr += variants * sizeof(uint32_t);

                        // std::cout << variants << " variants\n";
                        // for (size_t tile_i = 0; tile_i < static_cast<size_t>(variants); ++tile_i) {
                        //     auto u32_voxel = palette_begin[tile_i];
                        //     print_voxel(u32_to_voxel(u32_voxel));
                        //     std::cout << " (" << u32_voxel << ")\n";
                        // }
                        // std::cout << std::flush;

                        for (size_t zi = 0; zi < REGION_SIZE; ++zi) {
                            for (size_t yi = 0; yi < REGION_SIZE; ++yi) {
                                for (size_t xi = 0; xi < REGION_SIZE; ++xi) {
                                    auto const px = ox + xi;
                                    auto const py = oy + yi;
                                    auto const pz = oz + zi;
                                    auto const index = px + py * node.size_x + pz * node.size_x * node.size_y;
                                    auto const in_region_index = xi + yi * REGION_SIZE + zi * REGION_SIZE * REGION_SIZE;
                                    auto const bit_index = static_cast<uint32_t>(in_region_index * bits_per_variant);
                                    auto const byte_index = bit_index / 8;
                                    auto const bit_offset = static_cast<uint32_t>(bit_index - byte_index * 8);
                                    auto const mask = get_mask(bits_per_variant);
                                    auto &input = *reinterpret_cast<uint32_t *>(buffer_ptr + byte_index);
                                    auto const palette_id = (input >> bit_offset) & mask;
                                    assert(palette_id < variants);
                                    // if (palette_id >= variants) {
                                    //     std::cout << palette_id << "\n";
                                    //     continue;
                                    // }
                                    auto const u32_voxel = palette_begin[palette_id];
                                    auto r = static_cast<float>((u32_voxel >> 0x00) & 0xff) / 255.0f;
                                    auto g = static_cast<float>((u32_voxel >> 0x08) & 0xff) / 255.0f;
                                    auto b = static_cast<float>((u32_voxel >> 0x10) & 0xff) / 255.0f;
                                    auto const i = (u32_voxel >> 0x18) & 0xff;
                                    node.voxels[index] = GVoxVoxel{
                                        .color = {r, g, b},
                                        .id = i,
                                    };
                                }
                            }
                        }
                        // buffer_ptr += calc_palette_region_size(bits_per_variant);
                    }
                }
            }
        }
        buffer_ptr = next_node;
    }

    return result;
}

extern "C" EXPORT auto gvox_format_gvox_u32_palette_create_context() -> void * {
    auto *result = new GVoxU32PaletteContext{};
    return result;
}

extern "C" EXPORT void gvox_format_gvox_u32_palette_destroy_context(void *context_ptr) {
    auto *self = reinterpret_cast<GVoxU32PaletteContext *>(context_ptr);
    delete self;
}

extern "C" EXPORT auto gvox_format_gvox_u32_palette_create_payload(void *context_ptr, GVoxScene scene) -> GVoxPayload {
    auto *self = reinterpret_cast<GVoxU32PaletteContext *>(context_ptr);
    return self->create_payload(scene);
}

extern "C" EXPORT void gvox_format_gvox_u32_palette_destroy_payload(void *context_ptr, GVoxPayload payload) {
    auto *self = reinterpret_cast<GVoxU32PaletteContext *>(context_ptr);
    self->destroy_payload(payload);
}

extern "C" EXPORT auto gvox_format_gvox_u32_palette_parse_payload(void *context_ptr, GVoxPayload payload) -> GVoxScene {
    auto *self = reinterpret_cast<GVoxU32PaletteContext *>(context_ptr);
    return self->parse_payload(payload);
}
