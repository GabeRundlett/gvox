#pragma once

#include <gvox/gvox.h>
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
    void (*iterator_advance)(void *, void **, GvoxIteratorAdvanceInfo const *, GvoxIteratorValue *);
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

union FormatDescriptor {
    struct PackedMultiChannel {
        uint32_t _encoding : 10;
        uint32_t component_count : 2;
        uint32_t swizzle_mode : 1;
        uint32_t d0_bit_count : 6;
        uint32_t d1_bit_count : 6;
        uint32_t d2_bit_count : 6;
        uint32_t _pad1 : 1;
    };
    struct SingleChannel {
        uint32_t _encoding : 10;
        uint32_t bit_count : 6;
    };

    GvoxFormatEncoding encoding;
    PackedMultiChannel packed;
    SingleChannel single;
};

static_assert(sizeof(FormatDescriptor) == sizeof(GvoxFormat));

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

struct GvoxChainStruct {
    GvoxStructType struct_type;
    void const *next;
};
