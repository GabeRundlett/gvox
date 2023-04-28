#include <gvox/gvox.h>
#include <gvox_standard_functions.hpp>

#include <vector>

#define IMPL_STRUCT_NAME(Name) Gvox##Name##_ImplT

#define IMPL_STRUCT_DEFAULTS(Name)                                                     \
    void *self{};                                                                      \
    Gvox##Name##Description desc{};                                                    \
    ~IMPL_STRUCT_NAME(Name)() {                                                        \
        if (self != nullptr) {                                                         \
            desc.destroy(self);                                                        \
        }                                                                              \
    }                                                                                  \
    IMPL_STRUCT_NAME(Name)                                                             \
    () = default;                                                                      \
    IMPL_STRUCT_NAME(Name)                                                             \
    (IMPL_STRUCT_NAME(Name) const &) = delete;                                         \
    IMPL_STRUCT_NAME(Name)                                                             \
    (IMPL_STRUCT_NAME(Name) &&) = default;                                             \
    auto operator=(IMPL_STRUCT_NAME(Name) const &)->IMPL_STRUCT_NAME(Name) & = delete; \
    auto operator=(IMPL_STRUCT_NAME(Name) &&)->IMPL_STRUCT_NAME(Name) & = default;

struct IMPL_STRUCT_NAME(InputStream) {
    IMPL_STRUCT_DEFAULTS(InputStream)
};
struct IMPL_STRUCT_NAME(OutputStream) {
    IMPL_STRUCT_DEFAULTS(OutputStream)
};
struct IMPL_STRUCT_NAME(InputAdapter) {
    IMPL_STRUCT_DEFAULTS(InputAdapter)
};
struct IMPL_STRUCT_NAME(OutputAdapter) {
    IMPL_STRUCT_DEFAULTS(OutputAdapter)
};
struct IMPL_STRUCT_NAME(Parser) {
    IMPL_STRUCT_DEFAULTS(Parser)
    std::vector<GvoxInputAdapter> input_adapters{};
};
struct IMPL_STRUCT_NAME(Serializer) {
    IMPL_STRUCT_DEFAULTS(Serializer)
};
struct IMPL_STRUCT_NAME(Container) {
    IMPL_STRUCT_DEFAULTS(Container)
};

struct GvoxChainStruct {
    GvoxStructType struct_type;
    void const *next;
};

#define HANDLE_CREATE(Type, TYPE)                                                      \
    if (info == nullptr) {                                                             \
        return GVOX_ERROR_INVALID_ARGUMENT;                                            \
    }                                                                                  \
    if (handle == nullptr) {                                                           \
        return GVOX_ERROR_INVALID_ARGUMENT;                                            \
    }                                                                                  \
    if (info->struct_type != GVOX_STRUCT_TYPE_##TYPE##_CREATE_INFO) {                  \
        return GVOX_ERROR_BAD_STRUCT_TYPE;                                             \
    }                                                                                  \
    *handle = new IMPL_STRUCT_NAME(Type){};                                            \
    (*handle)->desc = info->description;                                               \
    {                                                                                  \
        auto create_result = (*handle)->desc.create(&(*handle)->self, &info->cb_args); \
        if (create_result != GVOX_SUCCESS) {                                           \
            delete *handle;                                                            \
            return create_result;                                                      \
        }                                                                              \
    }

auto gvox_create_input_stream(GvoxInputStreamCreateInfo const *info, GvoxInputStream *handle) -> GvoxResult {
    HANDLE_CREATE(InputStream, INPUT_STREAM)

    // if (!info->adapter_chain) {
    //     return GVOX_ERROR_BAD_ADAPTER_CHAIN;
    // }
    // auto adapter_chain = static_cast<GvoxChainStruct const *>(info->adapter_chain);
    // while (adapter_chain != nullptr) {
    //     switch (adapter_chain->struct_type) {
    //     case GVOX_STRUCT_TYPE_INPUT_ADAPTER_CREATE_INFO: {
    //         // auto new_input_adapter = IMPL_STRUCT_NAME(InputAdapter){};
    //         // auto result = gvox_create_input_adapter(reinterpret_cast<GvoxInputAdapterCreateInfo *>(adapter_chain), new_input_adapter);
    //         // (*adapter)->input_adapters.push_back(new_input_adapter);
    //     } break;
    //     default:
    //         delete adapter_chain;
    //         return GVOX_ERROR_BAD_ADAPTER_CHAIN;
    //     }
    // }

    return GVOX_SUCCESS;
}
auto gvox_create_output_stream(GvoxOutputStreamCreateInfo const *info, GvoxOutputStream *handle) -> GvoxResult {
    HANDLE_CREATE(OutputStream, OUTPUT_STREAM)
    return GVOX_SUCCESS;
}
auto gvox_create_input_adapter(GvoxInputAdapterCreateInfo const *info, GvoxInputAdapter *handle) -> GvoxResult {
    HANDLE_CREATE(InputAdapter, INPUT_ADAPTER)
    return GVOX_SUCCESS;
}
auto gvox_create_output_adapter(GvoxOutputAdapterCreateInfo const *info, GvoxOutputAdapter *handle) -> GvoxResult {
    HANDLE_CREATE(OutputAdapter, OUTPUT_ADAPTER)
    return GVOX_SUCCESS;
}
auto gvox_create_parser(GvoxParserCreateInfo const *info, GvoxParser *handle) -> GvoxResult {
    HANDLE_CREATE(Parser, PARSER)
    return GVOX_SUCCESS;
}
auto gvox_create_serializer(GvoxSerializerCreateInfo const *info, GvoxSerializer *handle) -> GvoxResult {
    HANDLE_CREATE(Serializer, SERIALIZER)
    return GVOX_SUCCESS;
}
auto gvox_create_container(GvoxContainerCreateInfo const *info, GvoxContainer *handle) -> GvoxResult {
    HANDLE_CREATE(Container, CONTAINER)
    return GVOX_SUCCESS;
}

#undef HANDLE_CREATE

namespace {
    inline void destroy_handle(auto *handle) {
        delete handle;
    }
} // namespace

void gvox_destroy_input_stream(GvoxInputStream handle) { destroy_handle(handle); }
void gvox_destroy_output_stream(GvoxOutputStream handle) { destroy_handle(handle); }
void gvox_destroy_input_adapter(GvoxInputAdapter handle) { destroy_handle(handle); }
void gvox_destroy_output_adapter(GvoxOutputAdapter handle) { destroy_handle(handle); }
void gvox_destroy_parser(GvoxParser handle) { destroy_handle(handle); }
void gvox_destroy_serializer(GvoxSerializer handle) { destroy_handle(handle); }
void gvox_destroy_container(GvoxContainer handle) { destroy_handle(handle); }

#define HANDLE_GET_DESC(type, Type, TYPE)                                                                                                                       \
    auto name_str = std::string_view{name};                                                                                                                     \
    auto iter = std::find_if(standard_##type##s.begin(), standard_##type##s.end(), [name_str](Std##Type##Info const &info) { return info.first == name_str; }); \
    if (iter == standard_##type##s.end()) {                                                                                                                     \
        return GVOX_ERROR_UNKNOWN_STANDARD_##TYPE;                                                                                                              \
    }                                                                                                                                                           \
    *desc = iter->second;                                                                                                                                       \
    return GVOX_SUCCESS

auto gvox_get_standard_input_stream_description(char const *name, GvoxInputStreamDescription *desc) -> GvoxResult { HANDLE_GET_DESC(input_stream, InputStream, INPUT_STREAM); }
auto gvox_get_standard_output_stream_description(char const *name, GvoxOutputStreamDescription *desc) -> GvoxResult { HANDLE_GET_DESC(output_stream, OutputStream, OUTPUT_STREAM); }
auto gvox_get_standard_input_adapter_description(char const *name, GvoxInputAdapterDescription *desc) -> GvoxResult { HANDLE_GET_DESC(input_adapter, InputAdapter, INPUT_ADAPTER); }
auto gvox_get_standard_output_adapter_description(char const *name, GvoxOutputAdapterDescription *desc) -> GvoxResult { HANDLE_GET_DESC(output_adapter, OutputAdapter, OUTPUT_ADAPTER); }
auto gvox_get_standard_parser_description(char const *name, GvoxParserDescription *desc) -> GvoxResult { HANDLE_GET_DESC(parser, Parser, PARSER); }
auto gvox_get_standard_serializer_description(char const *name, GvoxSerializerDescription *desc) -> GvoxResult { HANDLE_GET_DESC(serializer, Serializer, SERIALIZER); }
auto gvox_get_standard_container_description(char const *name, GvoxContainerDescription *desc) -> GvoxResult { HANDLE_GET_DESC(container, Container, CONTAINER); }

#undef HANDLE_GET_DESC

auto gvox_blit(GvoxBlitInfo const *info) -> GvoxResult {
    if (info == nullptr) {
        return GVOX_ERROR_INVALID_ARGUMENT;
    }
    if (info->struct_type != GVOX_STRUCT_TYPE_BLIT_INFO) {
        return GVOX_ERROR_BAD_STRUCT_TYPE;
    }

    return GVOX_ERROR_UNKNOWN;
}

// Adapter API

auto gvox_input_read(GvoxInputStream handle, uint8_t *data, size_t size) -> GvoxResult {
    return handle->desc.read(handle->self, data, size);
}
auto gvox_input_seek(GvoxInputStream handle, long offset, GvoxSeekOrigin origin) -> GvoxResult {
    return handle->desc.seek(handle->self, offset, origin);
}
auto gvox_input_tell(GvoxInputStream handle) -> long {
    return handle->desc.tell(handle->self);
}

auto gvox_output_write(GvoxOutputStream handle, uint8_t *data, size_t size) -> GvoxResult {
    return handle->desc.write(handle->self, data, size);
}
auto gvox_output_seek(GvoxOutputStream handle, long offset, GvoxSeekOrigin origin) -> GvoxResult {
    return handle->desc.seek(handle->self, offset, origin);
}
auto gvox_output_seek(GvoxOutputStream handle) -> long {
    return handle->desc.tell(handle->self);
}
