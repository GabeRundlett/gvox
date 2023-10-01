#pragma once

#include <gvox/format.h>
#include <gvox/stream.h>
#include <vector>

#define IMPL_STRUCT_NAME(Name) Gvox##Name##_ImplT

// Streams

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

struct IMPL_STRUCT_NAME(InputStream) {
    GvoxInputStream next{};
    IMPL_STRUCT_DEFAULTS(InputStream, { delete next; });
};
struct IMPL_STRUCT_NAME(OutputStream) {
    GvoxOutputStream next{};
    IMPL_STRUCT_DEFAULTS(OutputStream, { delete next; });
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
struct IMPL_STRUCT_NAME(Iterator) {
    void *parent_self{};
    void *self{};
    void (*destroy_iterator)(void *, void *);
    void (*iterator_next)(void *, void **, GvoxInputStream, GvoxIteratorValue *);
    ~IMPL_STRUCT_NAME(Iterator)() {
        destroy_iterator(parent_self, self);
    }
    IMPL_STRUCT_NAME(Iterator)
    () = default;
    IMPL_STRUCT_NAME(Iterator)
    (IMPL_STRUCT_NAME(Iterator) const &) = delete;
    IMPL_STRUCT_NAME(Iterator)
    (IMPL_STRUCT_NAME(Iterator) &&) = default;
    auto operator=(IMPL_STRUCT_NAME(Iterator) const &) -> IMPL_STRUCT_NAME(Iterator) & = delete;
    auto operator=(IMPL_STRUCT_NAME(Iterator) &&) -> IMPL_STRUCT_NAME(Iterator) & = default;
};

// Format

struct FormatDescriptor {
    uint64_t encoding : 10;
    uint64_t component_count : 2;
    uint64_t c0_bit_count : 5;
    uint64_t c1_bit_count : 5;
    uint64_t c2_bit_count : 5;
    uint64_t c3_bit_count : 5;
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
