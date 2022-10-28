#pragma once

using f32 = float;
using u32 = unsigned int;
using i32 = int;
template <typename T>
struct t_vec2 { T x, y; };
template <typename T>
struct t_vec3 { T x, y, z; };
template <typename T>
struct t_vec4 { T x, y, z, w; };
using f32vec2 = t_vec2<f32>;
using f32vec3 = t_vec3<f32>;
using f32vec4 = t_vec4<f32>;
using u32vec2 = t_vec2<u32>;
using u32vec3 = t_vec3<u32>;
using u32vec4 = t_vec4<u32>;

u32 asuint(f32 x) {
    return *reinterpret_cast<u32 *>(&x);
}
u32 asuint(f32vec2 v) {
    f32 a = v.x;
    f32 b = v.y + static_cast<f32>(asuint(a));
    return asuint(b);
}
u32 asuint(f32vec3 v) {
    f32 a = v.x;
    f32 b = v.y + static_cast<f32>(asuint(a));
    f32 c = v.z + static_cast<f32>(asuint(b));
    return asuint(c);
}
u32 asuint(f32vec4 v) {
    f32 a = v.x;
    f32 b = v.y + static_cast<f32>(asuint(a));
    f32 c = v.z + static_cast<f32>(asuint(b));
    f32 d = v.w + static_cast<f32>(asuint(c));
    return asuint(d);
}
f32 asfloat(u32 x) {
    return *reinterpret_cast<f32 *>(&x);
}

f32 frac(f32 x) {
    return x - static_cast<f32>(static_cast<i32>(x));
}
f32vec3 frac(f32vec3 v) {
    return f32vec3{frac(v.x), frac(v.y), frac(v.z)};
}
f32 dot(f32vec3 a, f32vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
f32 mix(f32 a, f32 b, f32 f) {
    return a * (1.0f - f) + b * f;
}
f32 smoothstep(f32 lo, f32 hi, f32 x) {
    if (x < lo)
        return 0.0f;
    if (x >= hi)
        return 1.0f;
    x = (x - lo) / (hi - lo);
    return x * x * (3.0f - 2.0f * x);
}

u32 rand_hash(u32 x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}
u32 rand_hash(u32vec2 v) { return rand_hash(v.x ^ rand_hash(v.y)); }
u32 rand_hash(u32vec3 v) {
    return rand_hash(v.x ^ rand_hash(v.y) ^ rand_hash(v.z));
}
u32 rand_hash(u32vec4 v) {
    return rand_hash(v.x ^ rand_hash(v.y) ^ rand_hash(v.z) ^ rand_hash(v.w));
}
f32 rand_float_construct(u32 m) {
    const u32 ieee_mantissa = 0x007FFFFFu;
    const u32 ieee_one = 0x3F800000u;
    m &= ieee_mantissa;
    m |= ieee_one;
    f32 f = asfloat(m);
    return f - 1.0f;
}
f32 rand(f32 x) { return rand_float_construct(rand_hash(asuint(x))); }
f32 rand(f32vec2 v) { return rand_float_construct(rand_hash(asuint(v))); }
f32 rand(f32vec3 v) { return rand_float_construct(rand_hash(asuint(v))); }
f32 rand(f32vec4 v) { return rand_float_construct(rand_hash(asuint(v))); }
f32vec2 rand_vec2(f32vec2 v) { return f32vec2{rand(v.x), rand(v.y)}; }
f32vec3 rand_vec3(f32vec3 v) { return f32vec3{rand(v.x), rand(v.y), rand(v.z)}; }
f32vec4 rand_vec4(f32vec4 v) {
    return f32vec4{rand(v.x), rand(v.y), rand(v.z), rand(v.w)};
}
f32 noise(f32vec3 x) {
    const f32vec3 st = f32vec3{110.0f, 241.0f, 171.0f};
    f32vec3 i = f32vec3{floor(x.x), floor(x.y), floor(x.z)};
    f32vec3 f = frac(x);
    f32 n = dot(i, st);
    f32vec3 u = f32vec3{
        f.x * f.x * (3.0f - 2.0f * f.x),
        f.y * f.y * (3.0f - 2.0f * f.y),
        f.z * f.z * (3.0f - 2.0f * f.z),
    };
    f32 r = mix(
        mix(
            mix(rand(n + dot(st, f32vec3{0.0f, 0.0f, 0.0f})),
                rand(n + dot(st, f32vec3{1.0f, 0.0f, 0.0f})), u.x),
            mix(rand(n + dot(st, f32vec3{0.0f, 1.0f, 0.0f})),
                rand(n + dot(st, f32vec3{1.0f, 1.0f, 0.0f})), u.x),
            u.y),
        mix(
            mix(rand(n + dot(st, f32vec3{0.0f, 0.0f, 1.0f})),
                rand(n + dot(st, f32vec3{1.0f, 0.0f, 1.0f})), u.x),
            mix(rand(n + dot(st, f32vec3{0.0f, 1.0f, 1.0f})),
                rand(n + dot(st, f32vec3{1.0f, 1.0f, 1.0f})), u.x),
            u.y),
        u.z);
    return r * 2.0f - 1.0f;
}

struct FractalNoiseConfig {
    f32 amplitude;
    f32 persistance;
    f32 scale;
    f32 lacunarity;
    u32 octaves;
};

f32 fractal_noise(f32vec3 a_pos, FractalNoiseConfig config) {
    f32 value = 0.0f;
    f32 max_value = 0.0f;
    f32 amplitude = config.amplitude;
    // glm::mat3 rot_mat = glm::mat3(
    //     0.2184223f, -0.5347182f, 0.8163137f,
    //     0.9079879f, -0.1951438f, -0.3707788f,
    //     0.3575608f, 0.8221893f, 0.4428939f);
    f32vec3 pos = {a_pos.x, a_pos.y, a_pos.z};
    for (u32 i = 0; i < config.octaves; ++i) {
        // pos = (pos * rot_mat) + f32vec3(71.444f, 25.170f, -54.766f);
        pos = f32vec3{pos.x + 71.444f, pos.y + 25.170f, pos.z + -54.766f};
        f32vec3 p = f32vec3{pos.x * config.scale, pos.y * config.scale, pos.z * config.scale};
        value += noise(p) * config.amplitude;
        max_value += config.amplitude;
        config.amplitude *= config.persistance;
        config.scale *= config.lacunarity;
    }
    return value / max_value * amplitude;
}
