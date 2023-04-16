#include <gvox/gvox.h>
#include <gvox/containers/raw.h>

#include <cstdlib>
#include <cstdint>

#include <array>
#include <vector>
#include <new>

static constexpr auto CHUNK_SIZE = size_t{64};
static constexpr auto VOXELS_PER_CHUNK = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

struct Chunk {
    std::array<uint32_t, VOXELS_PER_CHUNK> voxels{};
};

struct GvoxRawContainer {
    // GvoxRegionRange range{};
    // std::map<std::array<int32_t, 3>, Chunk> chunks{};
};

auto gvox_container_raw_create(void **self, void const *config_ptr) -> GvoxResult {
    GvoxRawContainerConfig config;
    if (config_ptr) {
        config = *static_cast<GvoxRawContainerConfig const *>(config_ptr);
    } else {
        config = {};
    }
    *self = new GvoxRawContainer();
    return GVOX_SUCCESS;
}
void gvox_container_raw_destroy(void *self) {
    delete static_cast<GvoxRawContainer *>(self);
}
