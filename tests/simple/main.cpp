#include <gvox/gvox.h>

#include <gvox/containers/raw.h>

#include <iostream>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>

#define R96_ARGB(alpha, red, green, blue) (uint32_t)(((uint8_t)(alpha) << 24) | ((uint8_t)(blue) << 0) | ((uint8_t)(green) << 8) | (uint8_t)(red))

#define HANDLE_RES(x)          \
    if ((x) != GVOX_SUCCESS) { \
        return -1;             \
    }
auto main() -> int {
    using namespace std::literals;
    // std::this_thread::sleep_for(5s);

    auto *rgb_voxel_desc = GvoxVoxelDesc{};
    {
        auto const attribs = std::array{
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_ALBEDO_PACKED,
                .format = GVOX_STANDARD_FORMAT_R8G8B8_SRGB,
            },
            // GvoxAttribute{
            //     .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
            //     .next = nullptr,
            //     .type = GVOX_ATTRIBUTE_TYPE_NORMAL,
            //     .format = GVOX_STANDARD_FORMAT_R12G12B12_OCT_UNIT,
            // },
        };
        auto voxel_desc_info = GvoxVoxelDescCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
            .next = nullptr,
            .attribute_count = static_cast<uint32_t>(attribs.size()),
            .attributes = attribs.data(),
        };
        HANDLE_RES(gvox_create_voxel_desc(&voxel_desc_info, &rgb_voxel_desc))
    }

    auto *u32_color_voxel_desc = GvoxVoxelDesc{};
    {
        auto const attribs = std::array{
            GvoxAttribute{
                .struct_type = GVOX_STRUCT_TYPE_ATTRIBUTE,
                .next = nullptr,
                .type = GVOX_ATTRIBUTE_TYPE_ALBEDO_PACKED,
                .format = GVOX_STANDARD_FORMAT_R8G8B8_SRGB,
            },
        };
        auto voxel_desc_info = GvoxVoxelDescCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_VOXEL_DESC_CREATE_INFO,
            .next = nullptr,
            .attribute_count = static_cast<uint32_t>(attribs.size()),
            .attributes = attribs.data(),
        };
        HANDLE_RES(gvox_create_voxel_desc(&voxel_desc_info, &u32_color_voxel_desc))
    }

    auto *raw_container = GvoxContainer{};
    {
        auto raw_container_conf = GvoxRawContainerConfig{
            .voxel_desc = rgb_voxel_desc,
        };

        auto cont_info = GvoxContainerCreateInfo{
            .struct_type = GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO,
            .next = nullptr,
            .description = gvox_container_raw_description(),
            .cb_args = {
                .struct_type = {}, // ?
                .next = nullptr,
                .config = &raw_container_conf,
            },
        };

        HANDLE_RES(gvox_create_container(&cont_info, &raw_container))
    }

    auto speed_test_3d = [&]() {
        uint64_t voxel_data{};
        GvoxOffset3D offset{};
        GvoxExtent3D extent{};
        auto fill_info = GvoxFillInfo{
            .struct_type = GVOX_STRUCT_TYPE_FILL_INFO,
            .next = nullptr,
            .src_data = &voxel_data,
            .src_desc = rgb_voxel_desc,
            .dst = raw_container,
            .range = {
                {3, offset.data},
                {3, extent.data},
            },
        };
        voxel_data = 0x0000'0000'00ff'40aa;
        HANDLE_RES(gvox_fill(&fill_info));

        voxel_data = 0x0000'0000'00ff'20aa;
        offset = GvoxOffset3D{0, 0, 0};
        extent = GvoxExtent3D{64, 64, 64};
        auto const N_ITER = uint64_t{2000};
        using Clock = std::chrono::high_resolution_clock;
        auto t0 = Clock::now();
        for (uint64_t i = 0; i < N_ITER; i++) {
            voxel_data = R96_ARGB(255, rand() % 255, rand() % 255, rand() % 255);
            // offset.data[0] = rand() % 64;
            // offset.data[1] = rand() % 64;
            // offset.data[2] = rand() % 64;
            // extent.data[0] = std::min<uint64_t>(rand() % 16, 64 - offset.data[0]);
            // extent.data[1] = std::min<uint64_t>(rand() % 16, 64 - offset.data[1]);
            // extent.data[2] = std::min<uint64_t>(rand() % 16, 64 - offset.data[1]);
            HANDLE_RES(gvox_fill(&fill_info));
        }
        auto t1 = Clock::now();
        auto seconds = std::chrono::duration<double>(t1 - t0).count();
        auto voxel_n = N_ITER * extent.data[0] * extent.data[1] * extent.data[2];
        auto voxel_size = (gvox_voxel_desc_size_in_bits(rgb_voxel_desc) + 7) / 8;
        std::cout << seconds << "s, aka about " << static_cast<double>(voxel_n) / 1'000'000'000.0 / seconds << " GVx/s, or about " << static_cast<double>(voxel_n * voxel_size / 1000) / 1'000'000.0 / seconds << " GB/s" << '\n';

        return 0;
    };

    auto test_6d = [&]() {
        uint64_t voxel_data{};
        GvoxOffset6D offset{};
        GvoxExtent6D extent{};
        auto fill_info = GvoxFillInfo{
            .struct_type = GVOX_STRUCT_TYPE_FILL_INFO,
            .next = nullptr,
            .src_data = &voxel_data,
            .src_desc = rgb_voxel_desc,
            .dst = raw_container,
            .range = {
                {6, offset.data},
                {6, extent.data},
            },
        };

        // background
        voxel_data = 0x0000'0000'0020'2020;
        offset = GvoxOffset6D{0, 0, 0, 0, 0, 0};
        extent = GvoxExtent6D{3, 3, 3, 3, 3, 3};
        HANDLE_RES(gvox_fill(&fill_info));
        // x-axis
        voxel_data = 0x0000'0000'0000'00ff;
        offset = GvoxOffset6D{1, 0, 0, 0, 0, 0};
        extent = GvoxExtent6D{2, 1, 1, 1, 1, 1};
        HANDLE_RES(gvox_fill(&fill_info));
        // y-axis
        voxel_data = 0x0000'0000'0000'ff00;
        offset = GvoxOffset6D{0, 1, 0, 0, 0, 0};
        extent = GvoxExtent6D{1, 2, 1, 1, 1, 1};
        HANDLE_RES(gvox_fill(&fill_info));
        // z-axis
        voxel_data = 0x0000'0000'00ff'0000;
        offset = GvoxOffset6D{0, 0, 1, 0, 0, 0};
        extent = GvoxExtent6D{1, 1, 2, 1, 1, 1};
        HANDLE_RES(gvox_fill(&fill_info));
        // w-axis
        voxel_data = 0x0000'0000'0000'ffff;
        offset = GvoxOffset6D{0, 0, 0, 1, 0, 0};
        extent = GvoxExtent6D{1, 1, 1, 2, 1, 1};
        HANDLE_RES(gvox_fill(&fill_info));
        // v-axis
        voxel_data = 0x0000'0000'00ff'00ff;
        offset = GvoxOffset6D{0, 0, 0, 0, 1, 0};
        extent = GvoxExtent6D{1, 1, 1, 1, 2, 1};
        HANDLE_RES(gvox_fill(&fill_info));
        // u-axis
        voxel_data = 0x0000'0000'00ff'ff00;
        offset = GvoxOffset6D{0, 0, 0, 0, 0, 1};
        extent = GvoxExtent6D{1, 1, 1, 1, 1, 2};
        HANDLE_RES(gvox_fill(&fill_info));

        uint32_t const size_x = 3;
        uint32_t const size_y = 3;
        uint32_t const size_z = 3;
        uint32_t const size_w = 3;
        uint32_t const size_v = 3;
        uint32_t const size_u = 3;
        std::cout.fill('0');
        for (uint32_t vi = 0; vi < size_v; ++vi) {
            for (uint32_t zi = 0; zi < size_z; ++zi) {
                for (uint32_t yi = 0; yi < size_y; ++yi) {
                    for (uint32_t ui = 0; ui < size_u; ++ui) {
                        for (uint32_t wi = 0; wi < size_w; ++wi) {
                            for (uint32_t xi = 0; xi < size_x; ++xi) {
                                auto sample_offset = GvoxOffset6D{
                                    static_cast<int32_t>(xi),
                                    static_cast<int32_t>(size_y - yi - 1),
                                    static_cast<int32_t>(size_z - zi - 1),
                                    static_cast<int32_t>(wi),
                                    static_cast<int32_t>(size_v - vi - 1),
                                    static_cast<int32_t>(ui),
                                };
                                auto sampled_voxel_data = std::array<uint8_t, 4>{};
                                auto sample = GvoxSample{
                                    .offset = {6, sample_offset.data},
                                    .dst_voxel_data = &sampled_voxel_data,
                                    .dst_voxel_desc = u32_color_voxel_desc,
                                };
                                auto sample_info = GvoxSampleInfo{
                                    .struct_type = GVOX_STRUCT_TYPE_SAMPLE_INFO,
                                    .next = nullptr,
                                    .src = raw_container,
                                    .samples = &sample,
                                    .sample_n = 1,
                                };
                                HANDLE_RES(gvox_sample(&sample_info));
                                auto r = static_cast<uint32_t>(sampled_voxel_data[0]);
                                auto g = static_cast<uint32_t>(sampled_voxel_data[1]);
                                auto b = static_cast<uint32_t>(sampled_voxel_data[2]);
                                std::cout << "\033[48;2;" << std::setw(3) << r << ";" << std::setw(3) << g << ";" << std::setw(3) << b << "m  ";
                            }
                            std::cout << "\033[0m ";
                        }
                        std::cout << "\033[0m ";
                    }
                    std::cout << "\033[0m\n";
                }
                std::cout << "\033[0m\n";
            }
            std::cout << "\033[0m\n";
        }
        std::cout << "\033[0m" << std::flush;

        return 0;
    };

    {
        auto result = test_6d();
        if (result != 0) {
            return result;
        }
    }
    // {
    //     auto result = speed_test_3d();
    //     if (result != 0) {
    //         return result;
    //     }
    // }

    gvox_destroy_container(raw_container);
    gvox_destroy_voxel_desc(rgb_voxel_desc);
    gvox_destroy_voxel_desc(u32_color_voxel_desc);
}
