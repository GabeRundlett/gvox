#pragma once

#include <gvox/format.h>
#include <gvox/adapter.h>
#include <vector>

#define IMPL_STRUCT_NAME(Name) Gvox##Name##_ImplT

// Adapters

#define IMPL_STRUCT_DEFAULTS(Name, destructor_extra)                                   \
    void *self{};                                                                      \
    Gvox##Name##Description desc{};                                                    \
    ~IMPL_STRUCT_NAME(Name)() {                                                        \
        if (self != nullptr) {                                                         \
            desc.destroy(self);                                                        \
        }                                                                              \
        destructor_extra;                                                              \
    }                                                                                  \
    IMPL_STRUCT_NAME(Name)                                                             \
    () = default;                                                                      \
    IMPL_STRUCT_NAME(Name)                                                             \
    (IMPL_STRUCT_NAME(Name) const &) = delete;                                         \
    IMPL_STRUCT_NAME(Name)                                                             \
    (IMPL_STRUCT_NAME(Name) &&) = default;                                             \
    auto operator=(IMPL_STRUCT_NAME(Name) const &)->IMPL_STRUCT_NAME(Name) & = delete; \
    auto operator=(IMPL_STRUCT_NAME(Name) &&)->IMPL_STRUCT_NAME(Name) & = default

struct IMPL_STRUCT_NAME(InputAdapter) {
    GvoxInputAdapter next{};
    IMPL_STRUCT_DEFAULTS(InputAdapter, { delete next; });
};
struct IMPL_STRUCT_NAME(OutputAdapter) {
    GvoxOutputAdapter next{};
    IMPL_STRUCT_DEFAULTS(OutputAdapter, { delete next; });
};
struct IMPL_STRUCT_NAME(Parser) {
    IMPL_STRUCT_DEFAULTS(Parser, {});
};
struct IMPL_STRUCT_NAME(Serializer) {
    IMPL_STRUCT_DEFAULTS(Serializer, {});
};
struct IMPL_STRUCT_NAME(Container) {
    IMPL_STRUCT_DEFAULTS(Container, {});
};

// Format

struct FormatDescriptor {
    uint32_t encoding : 10;
    uint32_t component_count : 2;
    uint32_t c0_bit_count : 5;
    uint32_t c1_bit_count : 5;
    uint32_t c2_bit_count : 5;
    uint32_t c3_bit_count : 5;
};

struct Attribute {
    uint32_t bit_count{};
    uint32_t bit_offset{};
    GvoxAttributeType type{};
    FormatDescriptor format_desc{};
};

struct IMPL_STRUCT_NAME(VoxelDesc) {
    uint32_t bit_count{};
    std::vector<Attribute> attributes{};
};
