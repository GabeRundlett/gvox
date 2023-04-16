#include <gvox/gvox.h>
#include <gvox_standard_functions.hpp>

#include "types.hpp"

#define IMPL_STRUCT_NAME(NAME) Gvox##NAME##_ImplT

struct IMPL_STRUCT_NAME(InputStream) {
    void *self = nullptr;
    GvoxInputStreamDescription desc;
    ~IMPL_STRUCT_NAME(InputStream)() {
        if (self) {
            desc.destroy(self);
        }
    }
};
struct IMPL_STRUCT_NAME(OutputStream) {
    void *self = nullptr;
    GvoxOutputStreamDescription desc;
    ~IMPL_STRUCT_NAME(OutputStream)() {
        if (self) {
            desc.destroy(self);
        }
    }
};
struct IMPL_STRUCT_NAME(ParseAdapter) {
    void *self = nullptr;
    GvoxParseAdapterDescription desc;
    ~IMPL_STRUCT_NAME(ParseAdapter)() {
        if (self) {
            desc.destroy(self);
        }
    }
};
struct IMPL_STRUCT_NAME(SerializeAdapter) {
    void *self = nullptr;
    GvoxSerializeAdapterDescription desc;
    ~IMPL_STRUCT_NAME(SerializeAdapter)() {
        if (self) {
            desc.destroy(self);
        }
    }
};
struct IMPL_STRUCT_NAME(Parser) {
    void *self = nullptr;
    GvoxParserDescription desc;
    std::vector<GvoxParseAdapter> parse_adapters{};
    ~IMPL_STRUCT_NAME(Parser)() {
        if (self) {
            desc.destroy(self);
        }
    }
};
struct IMPL_STRUCT_NAME(Serializer) {
    void *self = nullptr;
    GvoxSerializerDescription desc;
    ~IMPL_STRUCT_NAME(Serializer)() {
        if (self) {
            desc.destroy(self);
        }
    }
};
struct IMPL_STRUCT_NAME(Container) {
    void *self = nullptr;
    GvoxContainerDescription desc;
    ~IMPL_STRUCT_NAME(Container)() {
        if (self) {
            desc.destroy(self);
        }
    }
};

struct GvoxChainStruct {
    GvoxStructType type;
    void const *next;
};

#define HANDLE_CREATE(Type, TYPE)                                                    \
    if (info == nullptr) {                                                           \
        return GVOX_ERROR_INVALID_ARGUMENT;                                          \
    }                                                                                \
    if (handle == nullptr) {                                                         \
        return GVOX_ERROR_INVALID_ARGUMENT;                                          \
    }                                                                                \
    if (info->type != GVOX_STRUCT_TYPE_##TYPE##_CREATE_INFO) {                       \
        return GVOX_ERROR_BAD_STRUCT_TYPE;                                           \
    }                                                                                \
    *handle = new IMPL_STRUCT_NAME(Type){};                                          \
    (*handle)->desc = info->description;                                             \
    {                                                                                \
        auto create_result = (*handle)->desc.create(&(*handle)->self, info->config); \
        if (create_result != GVOX_SUCCESS) {                                         \
            delete *handle;                                                          \
            return create_result;                                                    \
        }                                                                            \
    }

GvoxResult gvox_create_input_stream(GvoxInputStreamCreateInfo const *info, GvoxInputStream *handle) {
    HANDLE_CREATE(InputStream, INPUT_STREAM)
    return GVOX_SUCCESS;
}
GvoxResult gvox_create_output_stream(GvoxOutputStreamCreateInfo const *info, GvoxOutputStream *handle) {
    HANDLE_CREATE(OutputStream, OUTPUT_STREAM)
    return GVOX_SUCCESS;
}
GvoxResult gvox_create_parse_adapter(GvoxParseAdapterCreateInfo const *info, GvoxParseAdapter *handle) {
    HANDLE_CREATE(ParseAdapter, PARSE_ADAPTER)
    return GVOX_SUCCESS;
}
GvoxResult gvox_create_serialize_adapter(GvoxSerializeAdapterCreateInfo const *info, GvoxSerializeAdapter *handle) {
    HANDLE_CREATE(SerializeAdapter, SERIALIZE_ADAPTER)
    return GVOX_SUCCESS;
}
GvoxResult gvox_create_parser(GvoxParserCreateInfo const *info, GvoxParser *handle) {
    HANDLE_CREATE(Parser, PARSER)

    // if (!info->adapter_chain) {
    //     return GVOX_ERROR_BAD_ADAPTER_CHAIN;
    // }
    // auto adapter_chain = static_cast<GvoxChainStruct const *>(info->adapter_chain);
    // while (adapter_chain != nullptr) {
    //     switch (adapter_chain->type) {
    //     case GVOX_STRUCT_TYPE_PARSE_ADAPTER_CREATE_INFO: {
    //         // auto new_parse_adapter = IMPL_STRUCT_NAME(ParseAdapter){};
    //         // auto result = gvox_create_parse_adapter(reinterpret_cast<GvoxParseAdapterCreateInfo *>(adapter_chain), new_parse_adapter);
    //         // (*adapter)->parse_adapters.push_back(new_parse_adapter);
    //     } break;
    //     default:
    //         delete adapter_chain;
    //         return GVOX_ERROR_BAD_ADAPTER_CHAIN;
    //     }
    // }

    return GVOX_SUCCESS;
}
GvoxResult gvox_create_serializer(GvoxSerializerCreateInfo const *info, GvoxSerializer *handle) {
    HANDLE_CREATE(Serializer, SERIALIZER)
    return GVOX_SUCCESS;
}
GvoxResult gvox_create_container(GvoxContainerCreateInfo const *info, GvoxContainer *handle) {
    HANDLE_CREATE(Container, CONTAINER)
    return GVOX_SUCCESS;
}

#undef HANDLE_CREATE

#define HANDLE_DESTROY()   \
    if (handle != nullptr) \
    delete handle

void gvox_destroy_input_stream(GvoxInputStream handle) { HANDLE_DESTROY(); }
void gvox_destroy_output_stream(GvoxOutputStream handle) { HANDLE_DESTROY(); }
void gvox_destroy_parse_adapter(GvoxParseAdapter handle) { HANDLE_DESTROY(); }
void gvox_destroy_serialize_adapter(GvoxSerializeAdapter handle) { HANDLE_DESTROY(); }
void gvox_destroy_parser(GvoxParser handle) { HANDLE_DESTROY(); }
void gvox_destroy_serializer(GvoxSerializer handle) { HANDLE_DESTROY(); }
void gvox_destroy_container(GvoxContainer handle) { HANDLE_DESTROY(); }

#undef HANDLE_DESTROY

#define HANDLE_GET_DESC(type, Type, TYPE)                                                                                                                       \
    auto name_str = std::string_view{name};                                                                                                                     \
    auto iter = std::find_if(standard_##type##s.begin(), standard_##type##s.end(), [name_str](Std##Type##Info const &info) { return info.first == name_str; }); \
    if (iter == standard_##type##s.end()) {                                                                                                                     \
        return GVOX_ERROR_UNKNOWN_STANDARD_##TYPE;                                                                                                              \
    }                                                                                                                                                           \
    *desc = iter->second;                                                                                                                                       \
    return GVOX_SUCCESS

GvoxResult gvox_get_standard_input_stream_description(char const *name, GvoxInputStreamDescription *desc) { HANDLE_GET_DESC(input_stream, InputStream, INPUT_STREAM); }
GvoxResult gvox_get_standard_output_stream_description(char const *name, GvoxOutputStreamDescription *desc) { HANDLE_GET_DESC(output_stream, OutputStream, OUTPUT_STREAM); }
GvoxResult gvox_get_standard_parse_adapter_description(char const *name, GvoxParseAdapterDescription *desc) { HANDLE_GET_DESC(parse_adapter, ParseAdapter, PARSE_ADAPTER); }
GvoxResult gvox_get_standard_serialize_adapter_description(char const *name, GvoxSerializeAdapterDescription *desc) { HANDLE_GET_DESC(serialize_adapter, SerializeAdapter, SERIALIZE_ADAPTER); }
GvoxResult gvox_get_standard_parser_description(char const *name, GvoxParserDescription *desc) { HANDLE_GET_DESC(parser, Parser, PARSER); }
GvoxResult gvox_get_standard_serializer_description(char const *name, GvoxSerializerDescription *desc) { HANDLE_GET_DESC(serializer, Serializer, SERIALIZER); }
GvoxResult gvox_get_standard_container_description(char const *name, GvoxContainerDescription *desc) { HANDLE_GET_DESC(container, Container, CONTAINER); }

#undef HANDLE_GET_DESC

GvoxResult gvox_parse(GvoxParseInfo const *info) {
    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->type != GVOX_STRUCT_TYPE_PARSE_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    // auto const cb_info = GvoxParseCbInfo{
    //     .type = GVOX_STRUCT_TYPE_PARSE_CB_INFO,
    //     .next = NULL,
    //     .input_stream = info->src,
    //     .transform = info->transform,
    // };
    // auto &dst = *(info->dst);
    // return dst.desc.parse(dst.self, &cb_info);
    return GVOX_ERROR_UNKNOWN;
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

GvoxResult gvox_input_read(GvoxInputStream handle, uint8_t *data, size_t size) {
    return handle->desc.read(handle->self, data, size);
}
GvoxResult gvox_input_seek(GvoxInputStream handle, long offset, GvoxSeekOrigin origin) {
    return handle->desc.seek(handle->self, offset, origin);
}

GvoxResult gvox_output_write(GvoxOutputStream handle, uint8_t *data, size_t size) {
    return handle->desc.write(handle->self, data, size);
}
GvoxResult gvox_output_seek(GvoxOutputStream handle, long offset, GvoxSeekOrigin origin) {
    return handle->desc.seek(handle->self, offset, origin);
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
