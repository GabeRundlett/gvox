#pragma once

#include <cstdint>
#include <array>
#include <limits>

auto const MAX_DEPTH = std::numeric_limits<float>::max();

struct Vec3 {
    float x, y, z;

    Vec3() = default;
    explicit Vec3(float a) : x{a}, y{a}, z{a} {}
    explicit Vec3(float x, float y, float z) : x{x}, y{y}, z{z} {}

    auto operator+(Vec3 a) const -> Vec3 {
        auto b = *this;
        b.x += a.x;
        b.y += a.y;
        b.z += a.z;
        return b;
    }
    auto operator-(Vec3 a) const -> Vec3 {
        auto b = *this;
        b.x -= a.x;
        b.y -= a.y;
        b.z -= a.z;
        return b;
    }
    auto operator*(auto a) const -> Vec3 {
        auto b = *this;
        b.x *= a;
        b.y *= a;
        b.z *= a;
        return b;
    }
    auto operator[](int i) const -> float {
        switch (i) {
        case 1: return y;
        case 2: return z;
        default: return x;
        }
    }
};
using Triangle = std::array<Vec3, 3>;

struct Ray {
    Vec3 o;
    Vec3 d;
};

struct IntersectionRecord {
    float t = MAX_DEPTH;
    Vec3 nrm;
};

auto xorshf96() -> uint32_t {
    thread_local uint32_t x = 123456789;
    thread_local uint32_t y = 362436069;
    thread_local uint32_t z = 521288629;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;
    auto t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;
    return z;
}

auto fast_random() -> uint32_t {
    return xorshf96();
}

auto fast_rand_float() -> float {
    return static_cast<float>(fast_random() & 0xffff) / static_cast<float>(0xffff);
}

auto cross(Vec3 a, Vec3 b) -> Vec3 {
    return Vec3{
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}
auto dot(Vec3 a, Vec3 b) -> float {
    return a.x * b.x +
           a.y * b.y +
           a.z * b.z;
}
auto mag_sq(Vec3 a) -> float {
    return dot(a, a);
}
auto normalize(Vec3 a) -> Vec3 {
    return a * (1.0f / sqrtf(mag_sq(a)));
}

auto min(Vec3 a, Vec3 b) -> Vec3 {
    return Vec3{
        std::min(a.x, b.x),
        std::min(a.y, b.y),
        std::min(a.z, b.z),
    };
}
auto max(Vec3 a, Vec3 b) -> Vec3 {
    return Vec3{
        std::max(a.x, b.x),
        std::max(a.y, b.y),
        std::max(a.z, b.z),
    };
}

inline void intersect_tri(Ray const &ray, IntersectionRecord &ir, const Triangle &tri) {
    const Vec3 edge1 = tri[1] - tri[0];
    const Vec3 edge2 = tri[2] - tri[0];
    const Vec3 h = cross(ray.d, edge2);
    const float a = dot(edge1, h);
    if (a > -0.0001f && a < 0.0001f) {
        return; // ray parallel to triangle
    }
    const float f = 1 / a;
    const Vec3 s = ray.o - tri[0];
    const float u = f * dot(s, h);
    if (u < 0 || u > 1) {
        return;
    }
    const Vec3 q = cross(s, edge1);
    const float v = f * dot(ray.d, q);
    if (v < 0 || u + v > 1) {
        return;
    }
    const float t = f * dot(edge2, q);
    if (t > 0.0001f) {
        ir.t = std::min(ir.t, t);
        if (t == ir.t) {
            ir.nrm = normalize(cross(edge1, edge2));
        }
    }
}

inline void intersect_aabb(Ray const &ray, IntersectionRecord &ir, const Vec3 bmin, const Vec3 bmax) {
    auto tx1 = (bmin.x - ray.o.x) / ray.d.x;
    auto tx2 = (bmax.x - ray.o.x) / ray.d.x;
    auto tmin = std::min(tx1, tx2);
    auto tmax = std::max(tx1, tx2);
    auto ty1 = (bmin.y - ray.o.y) / ray.d.y;
    auto ty2 = (bmax.y - ray.o.y) / ray.d.y;
    tmin = std::max(tmin, std::min(ty1, ty2)), tmax = std::min(tmax, std::max(ty1, ty2));
    auto tz1 = (bmin.z - ray.o.z) / ray.d.z;
    auto tz2 = (bmax.z - ray.o.z) / ray.d.z;
    tmin = std::max(tmin, std::min(tz1, tz2)), tmax = std::min(tmax, std::max(tz1, tz2));
    if (tmax >= tmin && tmax > 0.0f) {
        ir.t = tmin;
    }
}
