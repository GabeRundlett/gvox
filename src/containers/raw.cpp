#include <gvox/adapter.h>
#include <gvox/containers/raw.h>

#include <cstdlib>
#include <cstdint>

#include <array>
#include <vector>
#include <new>
#include <map>

static constexpr auto CHUNK_SIZE = size_t{64};
static constexpr auto VOXELS_PER_CHUNK = CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE;

struct Chunk {
    std::vector<uint64_t> voxels{};
};

struct GvoxRawContainer {
    // GvoxRegionRange range{};
    std::map<std::array<int64_t, 3>, Chunk> chunks{};
};

auto gvox_container_raw_create(void **self, GvoxContainerCreateCbArgs const *args) -> GvoxResult {
    GvoxRawContainerConfig config;
    if (args->config != nullptr) {
        config = *static_cast<GvoxRawContainerConfig const *>(args->config);
    } else {
        config = {};
    }
    *self = new GvoxRawContainer();
    return GVOX_SUCCESS;
}
void gvox_container_raw_destroy(void *self) {
    delete static_cast<GvoxRawContainer *>(self);
}
