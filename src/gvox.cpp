#include <gvox/gvox.h>
#include <gvox_standard_functions.hpp>

#include "types.hpp"

#define IMPL_STRUCT_NAME(NAME) Gvox##NAME##_ImplT

struct IMPL_STRUCT_NAME(Container) {
    void *self;
    GvoxContainerDescription desc;
};

struct IMPL_STRUCT_NAME(IoAdapter) {
    void *self;
    GvoxIoAdapterDescription desc;
};

GvoxResult gvox_create_io_adapter(GvoxIoAdapterCreateInfo const *info, GvoxIoAdapter *adapter) {
    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (adapter == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->type != GVOX_STRUCT_TYPE_IO_ADAPTER_CREATE_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    *adapter = new IMPL_STRUCT_NAME(IoAdapter){};
    (*adapter)->desc = info->description;
    return (*adapter)->desc.create(&(*adapter)->self, info->config);
}
void gvox_destroy_io_adapter(GvoxIoAdapter adapter) {
    if (adapter != nullptr) {
        adapter->desc.destroy(adapter->self);
        delete adapter;
    }
}

GvoxResult gvox_create_container(GvoxContainerCreateInfo const *info, GvoxContainer *container) {
    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (container == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->type != GVOX_STRUCT_TYPE_CONTAINER_CREATE_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    *container = new IMPL_STRUCT_NAME(Container){};
    (*container)->desc = info->description;
    return (*container)->desc.create(&(*container)->self, info->config);
}
void gvox_destroy_container(GvoxContainer container) {
    if (container != nullptr) {
        container->desc.destroy(container->self);
        delete container;
    }
}

GvoxResult gvox_get_standard_io_adapter_description(char const *name, GvoxIoAdapterDescription *desc) {
    auto name_str = std::string_view{name};
    auto iter = std::find_if(standard_io_adapters.begin(), standard_io_adapters.end(), [name_str](StdIoAdapterInfo const &info) { return info.first == name_str; });
    if (iter == standard_io_adapters.end()) {
        return GVOX_ERROR_UNKNOWN_STANDARD_IO_ADAPTER;
    }
    *desc = iter->second;
    return GVOX_SUCCESS;
}

GvoxResult gvox_get_standard_container_description(char const *name, GvoxContainerDescription *desc) {
    auto name_str = std::string_view{name};
    auto iter = std::find_if(standard_containers.begin(), standard_containers.end(), [name_str](StdContainerInfo const &info) { return info.first == name_str; });
    if (iter == standard_containers.end()) {
        return GVOX_ERROR_UNKNOWN_STANDARD_CONTAINER;
    }
    *desc = iter->second;
    return GVOX_SUCCESS;
}

GvoxResult gvox_parse(GvoxParseInfo const *info) {
    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->type != GVOX_STRUCT_TYPE_PARSE_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    auto const cb_info = GvoxParseCbInfo{
        .type = GVOX_STRUCT_TYPE_PARSE_CB_INFO,
        .next = NULL,
        .input_adapter = info->src,
        .transform = info->transform,
    };
    auto &dst = *(info->dst);
    return dst.desc.parse(dst.self, &cb_info);
}

GvoxResult gvox_blit(GvoxBlitInfo const *info) {
    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->type != GVOX_STRUCT_TYPE_BLIT_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    return GVOX_ERROR_UNKNOWN;
}

GvoxResult gvox_serialize(GvoxSerializeInfo const *info) {
    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->type != GVOX_STRUCT_TYPE_SERIALIZE_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    return GVOX_ERROR_UNKNOWN;
}

// Adapter API

GvoxResult gvox_input_read(GvoxIoAdapter input_adapter, uint8_t *data, size_t size) {
    return input_adapter->desc.input_read(input_adapter->self, data, size);
}

GvoxResult gvox_input_seek(GvoxIoAdapter input_adapter, long offset, GvoxSeekOrigin origin) {
    return input_adapter->desc.input_seek(input_adapter->self, offset, origin);
}

// Utilities

using namespace gvox::types;

GvoxTransform gvox_identity_transform(uint32_t dimension) {
    auto result = GvoxTransform{.dimension_n = dimension};
    switch (dimension) {
    case 2: {
        auto &mat = *reinterpret_cast<f32mat2x3 *>(&result.data);
        mat = f32mat2x3{
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        };
    } break;
    case 3: {
        auto &mat = *reinterpret_cast<f32mat3x4 *>(&result.data);
        mat = f32mat3x4{
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
        };
    } break;
    case 4: {
        auto &mat = *reinterpret_cast<f32mat4x5 *>(&result.data);
        mat = f32mat4x5{
            {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
        };
    } break;
    }
    return result;
}

GvoxTransform gvox_inverse_transform(GvoxTransform const *transform) {
    auto result = GvoxTransform{.dimension_n = transform->dimension_n};
    switch (transform->dimension_n) {
    case 2: {
        auto &mat = *reinterpret_cast<f32mat2x3 const *>(transform->data);
        auto &ret = *reinterpret_cast<f32mat2x3 *>(&result.data);
        auto m = gvox::extend<float, 3, 3>(mat);
        m(2, 2) = 1;
        ret = gvox::truncate<float, 2, 3>(f32mat3x3(m.inverse()));
    } break;
    case 3: {
        auto &mat = *reinterpret_cast<f32mat3x4 const *>(transform->data);
        auto &ret = *reinterpret_cast<f32mat3x4 *>(&result.data);
        auto m = gvox::extend<float, 4, 4>(mat);
        m(3, 3) = 1;
        ret = gvox::truncate<float, 3, 4>(f32mat4x4(m.inverse()));
    } break;
    case 4: {
        auto &mat = *reinterpret_cast<f32mat4x5 const *>(transform->data);
        auto &ret = *reinterpret_cast<f32mat4x5 *>(&result.data);
        auto m = gvox::extend<float, 5, 5>(mat);
        m(4, 4) = 1;
        ret = gvox::truncate<float, 4, 5>(f32mat5x5(m.inverse()));
    } break;
    }
    return result;
}

GvoxTranslation gvox_apply_transform(GvoxTransform const *transform, GvoxTranslation point) {
    GvoxTranslation result = point;

    switch (transform->dimension_n) {
    case 2: {
        auto &vec = *reinterpret_cast<f32vec2 *>(&result);
        auto &mat = *reinterpret_cast<f32mat2x3 const *>(transform->data);
        auto m = gvox::extend<float, 3, 3>(mat);
        m(2, 2) = 1;
        auto v = m *f32vec3{vec.x(), vec.y(), 1.0f};
        vec = f32vec2{v.x(), v.y()};
    } break;
    case 3: {
        auto &vec = *reinterpret_cast<f32vec3 *>(&result);
        auto &mat = *reinterpret_cast<f32mat3x4 const *>(transform->data);
        auto m = gvox::extend<float, 4, 4>(mat);
        m(3, 3) = 1;
        auto v = m *f32vec4{vec.x(), vec.y(), vec.z(), 1.0f};
        vec = f32vec3{v.x(), v.y(), v.z()};
    } break;
    case 4: {
        auto &vec = *reinterpret_cast<f32vec4 *>(&result);
        auto &mat = *reinterpret_cast<f32mat4x5 const *>(transform->data);
        auto m = gvox::extend<float, 5, 5>(mat);
        m(4, 4) = 1;
        auto v = m *f32vec5{vec.x(), vec.y(), vec.z(), vec.w(), 1.0f};
        vec = f32vec4{v.x(), v.y(), v.z(), v.w()};
    } break;
    }
    return result;
}

void gvox_translate(GvoxTransform *transform, GvoxTranslation t) {
    switch (transform->dimension_n) {
    case 2: {
        auto &mat = *reinterpret_cast<f32mat2x3 *>(transform->data);
        auto m = gvox::extend<float, 3, 3>(mat);
        m(2, 2) = 1;
        auto translation = f32mat3x3{
            // {1.0f, 0.0f, 0.0f},
            // {0.0f, 1.0f, 0.0f},
            // {t.data[0], t.data[1], 1.0f},
            {1.0f, 0.0f, t.data[0]},
            {0.0f, 1.0f, t.data[1]},
            {0.0f, 0.0f, 1.0f},
        };
        mat = gvox::truncate<float, 2, 3>(f32mat3x3(m * translation));
    } break;
    case 3: {
        auto &mat = *reinterpret_cast<f32mat3x4 *>(transform->data);
        auto m = gvox::extend<float, 4, 4>(mat);
        m(3, 3) = 1;
        auto translation = f32mat4x4{
            {1.0f, 0.0f, 0.0f, t.data[0]},
            {0.0f, 1.0f, 0.0f, t.data[1]},
            {0.0f, 0.0f, 1.0f, t.data[2]},
            {0.0f, 0.0f, 0.0f, 1.0f},
        };
        mat = gvox::truncate<float, 3, 4>(f32mat4x4(m * translation));
    } break;
    case 4: {
        auto &mat = *reinterpret_cast<f32mat4x5 *>(transform->data);
        auto m = gvox::extend<float, 5, 5>(mat);
        m(4, 4) = 1;
        auto translation = f32mat5x5{
            {1.0f, 0.0f, 0.0f, 0.0f, t.data[0]},
            {0.0f, 1.0f, 0.0f, 0.0f, t.data[1]},
            {0.0f, 0.0f, 1.0f, 0.0f, t.data[2]},
            {0.0f, 0.0f, 0.0f, 1.0f, t.data[3]},
            {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        };
        mat = gvox::truncate<float, 4, 5>(f32mat5x5(m * translation));
    } break;
    }
}

void gvox_rotate(GvoxTransform *transform, GvoxRotation t) {
    switch (transform->dimension_n) {
    case 2: {
        auto &mat = *reinterpret_cast<f32mat2x3 *>(transform->data);
        auto m = gvox::extend<float, 3, 3>(mat);
        m(2, 2) = 1;
        auto cosx = std::cos(t.data[0]);
        auto sinx = std::sin(t.data[0]);
        auto translation = f32mat3x3{
            {cosx, -sinx, 0.0f},
            {sinx, cosx, 0.0f},
            {0.0f, 0.0f, 1.0f},
        };
        mat = gvox::truncate<float, 2, 3>(f32mat3x3(m * translation));
    } break;
    case 3: {
        auto &mat = *reinterpret_cast<f32mat3x4 *>(transform->data);
        auto m = gvox::extend<float, 4, 4>(mat);
        m(3, 3) = 1;
        auto translation = f32mat4x4{
            {1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
        };
        mat = gvox::truncate<float, 3, 4>(f32mat4x4(m * translation));
    } break;
    case 4: {
        auto &mat = *reinterpret_cast<f32mat4x5 *>(transform->data);
        auto m = gvox::extend<float, 5, 5>(mat);
        m(4, 4) = 1;
        auto translation = f32mat5x5{
            {1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 1.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        };
        mat = gvox::truncate<float, 4, 5>(f32mat5x5(m * translation));
    } break;
    }
}

void gvox_scale(GvoxTransform *transform, GvoxScale t) {
    switch (transform->dimension_n) {
    case 2: {
        auto &mat = *reinterpret_cast<f32mat2x3 *>(transform->data);
        auto m = gvox::extend<float, 3, 3>(mat);
        m(2, 2) = 1;
        auto translation = f32mat3x3{
            {t.data[0], 0.0f, 0.0f},
            {0.0f, t.data[1], 0.0f},
            {0.0f, 0.0f, 1.0f},
        };
        mat = gvox::truncate<float, 2, 3>(f32mat3x3(m * translation));
    } break;
    case 3: {
        auto &mat = *reinterpret_cast<f32mat3x4 *>(transform->data);
        auto m = gvox::extend<float, 4, 4>(mat);
        m(3, 3) = 1;
        auto translation = f32mat4x4{
            {t.data[0], 0.0f, 0.0f, 0.0f},
            {0.0f, t.data[1], 0.0f, 0.0f},
            {0.0f, 0.0f, t.data[2], 0.0f},
            {0.0f, 0.0f, 0.0f, 1.0f},
        };
        mat = gvox::truncate<float, 3, 4>(f32mat4x4(m * translation));
    } break;
    case 4: {
        auto &mat = *reinterpret_cast<f32mat4x5 *>(transform->data);
        auto m = gvox::extend<float, 5, 5>(mat);
        m(4, 4) = 1;
        auto translation = f32mat5x5{
            {t.data[0], 0.0f, 0.0f, 0.0f, 0.0f},
            {0.0f, t.data[1], 0.0f, 0.0f, 0.0f},
            {0.0f, 0.0f, t.data[2], 0.0f, 0.0f},
            {0.0f, 0.0f, 0.0f, t.data[3], 0.0f},
            {0.0f, 0.0f, 0.0f, 0.0f, 1.0f},
        };
        mat = gvox::truncate<float, 4, 5>(f32mat5x5(m * translation));
    } break;
    }
}
