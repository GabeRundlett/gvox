#pragma once

#include <cstdint>
#include <cstddef>

#if 0
#include <span>
#include <array>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usize = std::size_t;
using b32 = u32;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using isize = std::ptrdiff_t;

using f32 = float;
using f64 = double;

namespace gvox { inline namespace types {
    namespace detail {
        template <typename T, usize N>
        struct GenericVecMembers {
            std::array<T, N> array;
            constexpr T &operator[](usize i) noexcept { return array[i]; }
            constexpr T const &operator[](usize i) const noexcept { return array[i]; }
        };

        template <typename T>
        struct GenericVecMembers<T, 2> {
            T x, y;
            constexpr T &operator[](usize i) noexcept {
                switch (i) {
                case 1: return y;
                default: return x;
                }
            }
            constexpr T const &operator[](usize i) const noexcept {
                switch (i) {
                case 1: return y;
                default: return x;
                }
            }
        };
        template <typename T>
        struct GenericVecMembers<T, 3> {
            T x, y, z;
            constexpr T &operator[](usize i) noexcept {
                switch (i) {
                case 1: return y;
                case 2: return z;
                default: return x;
                }
            }
            constexpr T const &operator[](usize i) const noexcept {
                switch (i) {
                case 1: return y;
                case 2: return z;
                default: return x;
                }
            }
        };
        template <typename T>
        struct GenericVecMembers<T, 4> {
            T x, y, z, w;
            constexpr T &operator[](usize i) noexcept {
                switch (i) {
                case 1: return y;
                case 2: return z;
                case 3: return w;
                default: return x;
                }
            }
            constexpr T const &operator[](usize i) const noexcept {
                switch (i) {
                case 1: return y;
                case 2: return z;
                case 3: return w;
                default: return x;
                }
            }
        };

        template <typename T>
        struct GenericVecMembers<T, 5> {
            T x, y, z, w, v;
            constexpr T &operator[](usize i) noexcept {
                switch (i) {
                case 1: return y;
                case 2: return z;
                case 3: return w;
                case 4: return v;
                default: return x;
                }
            }
            constexpr T const &operator[](usize i) const noexcept {
                switch (i) {
                case 1: return y;
                case 2: return z;
                case 3: return w;
                case 4: return v;
                default: return x;
                }
            }
        };

        template <typename T, usize N>
        struct GenericVector : GenericVecMembers<T, N> {
        };

        template <typename T, usize M, usize N>
        struct GenericMatrix : std::array<GenericVector<T, N>, M> {
        };
    } // namespace detail

    using f32vec2 = detail::GenericVector<f32, 2>;
    using f32mat2x2 = detail::GenericMatrix<f32, 2, 2>;
    using f32mat2x3 = detail::GenericMatrix<f32, 2, 3>;
    using f32mat2x4 = detail::GenericMatrix<f32, 2, 4>;
    using f32mat2x5 = detail::GenericMatrix<f32, 2, 5>;
    using f32vec3 = detail::GenericVector<f32, 3>;
    using f32mat3x2 = detail::GenericMatrix<f32, 3, 2>;
    using f32mat3x3 = detail::GenericMatrix<f32, 3, 3>;
    using f32mat3x4 = detail::GenericMatrix<f32, 3, 4>;
    using f32mat3x5 = detail::GenericMatrix<f32, 3, 5>;
    using f32vec4 = detail::GenericVector<f32, 4>;
    using f32mat4x2 = detail::GenericMatrix<f32, 4, 2>;
    using f32mat4x3 = detail::GenericMatrix<f32, 4, 3>;
    using f32mat4x4 = detail::GenericMatrix<f32, 4, 4>;
    using f32mat4x5 = detail::GenericMatrix<f32, 4, 5>;
    using f32vec5 = detail::GenericVector<f32, 5>;
    using f32mat5x2 = detail::GenericMatrix<f32, 5, 2>;
    using f32mat5x3 = detail::GenericMatrix<f32, 5, 3>;
    using f32mat5x4 = detail::GenericMatrix<f32, 5, 4>;
    using f32mat5x5 = detail::GenericMatrix<f32, 5, 5>;

    template <typename T, usize N>
    constexpr auto operator+(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (usize i = 0; i < N; ++i)
            result[i] = a[i] + b[i];
        return result;
    }
    template <typename T, usize N>
    constexpr auto operator-(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (usize i = 0; i < N; ++i)
            result[i] = a[i] - b[i];
        return result;
    }
    template <typename T, usize N>
    constexpr auto operator*(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (usize i = 0; i < N; ++i)
            result[i] = a[i] * b[i];
        return result;
    }
    template <typename T, usize N>
    constexpr auto operator/(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (usize i = 0; i < N; ++i)
            result[i] = a[i] / b[i];
        return result;
    }
    template <typename T, usize N>
    constexpr auto dot(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> T {
        T result = 0;
        for (usize i = 0; i < N; ++i)
            result += a[i] * b[i];
        return result;
    }

    template <typename T, usize N>
    constexpr auto operator+(detail::GenericVector<T, N> const &a, T b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (usize i = 0; i < N; ++i)
            result[i] = a[i] + b;
        return result;
    }
    template <typename T, usize N>
    constexpr auto operator-(detail::GenericVector<T, N> const &a, T b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (usize i = 0; i < N; ++i)
            result[i] = a[i] - b;
        return result;
    }
    template <typename T, usize N>
    constexpr auto operator*(detail::GenericVector<T, N> const &a, T b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (usize i = 0; i < N; ++i)
            result[i] = a[i] * b;
        return result;
    }
    template <typename T, usize N>
    constexpr auto operator/(detail::GenericVector<T, N> const &a, T b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (usize i = 0; i < N; ++i)
            result[i] = a[i] / b;
        return result;
    }

    template <typename T, usize M, usize N, usize P>
    constexpr auto operator*(detail::GenericMatrix<T, M, N> const &a, detail::GenericMatrix<T, N, P> const &b) {
        auto c = detail::GenericMatrix<T, M, P>{};
        for (usize i = 0; i < M; ++i) {
            for (usize j = 0; j < P; ++j) {
                c[i][j] = 0;
                for (usize k = 0; k < N; ++k)
                    c[i][j] += a[i][k] * b[k][j];
            }
        }
        return c;
    }
    template <typename T, usize M, usize N>
    constexpr auto operator*(detail::GenericMatrix<T, M, N> const &a, detail::GenericVector<T, N> const &v) {
        auto c = detail::GenericVector<T, M>{};
        for (usize i = 0; i < M; ++i) {
            c[i] = 0;
            for (usize k = 0; k < N; ++k)
                c[i] += a[i][k] * v[k];
        }
        return c;
    }
    template <typename T, usize N, usize P>
    constexpr auto operator*(detail::GenericVector<T, N> const &v, detail::GenericMatrix<T, N, P> const &b) {
        auto c = detail::GenericVector<std::remove_cv_t<T>, P>{};
        for (usize j = 0; j < P; ++j) {
            c[j] = 0;
            for (usize k = 0; k < N; ++k)
                c[j] += v[k] * b[k][j];
        }
        return c;
    }

    template <typename T, usize M, usize N>
    constexpr auto transpose(detail::GenericMatrix<T, M, N> const &x) {
        auto result = detail::GenericMatrix<T, N, M>{};
        for (usize mi = 0; mi < M; ++mi) {
            for (usize ni = 0; ni < N; ++ni)
                result[ni][mi] = x[mi][ni];
        }
        return result;
    }

    template <typename T, usize M, usize N, usize M_, usize N_>
    constexpr auto extend(detail::GenericMatrix<T, M_, N_> const &x) {
        auto result = detail::GenericMatrix<T, M, N>{};
        for (usize mi = 0; mi < M_; ++mi) {
            for (usize ni = 0; ni < N_; ++ni)
                result[mi][ni] = x[mi][ni];
        }
        return result;
    }

    template <typename T, usize M, usize N, usize M_, usize N_>
    constexpr auto truncate(detail::GenericMatrix<T, M_, N_> const &x) {
        auto result = detail::GenericMatrix<T, M, N>{};
        for (usize mi = 0; mi < M; ++mi) {
            for (usize ni = 0; ni < N; ++ni)
                result[mi][ni] = x[mi][ni];
        }
        return result;
    }

    template <typename T, usize N>
    constexpr auto vec_from_span(std::span<T, N> const x) -> detail::GenericVector<std::remove_cv_t<T>, N> {
        auto result = detail::GenericVector<std::remove_cv_t<T>, N>{};
        for (usize i = 0; i < N; ++i)
            result[i] = x[i];
        return result;
    }

    template <typename T, usize M, usize N>
    constexpr auto mat_from_span(std::span<T, M * N> const x) -> detail::GenericMatrix<std::remove_cv_t<T>, M, N> {
        auto result = detail::GenericMatrix<std::remove_cv_t<T>, M, N>{};
        for (usize mi = 0; mi < M; ++mi) {
            for (usize ni = 0; ni < N; ++ni)
                result[mi][ni] = x[ni + mi * N];
        }
        return result;
    }
}} // namespace gvox::types
#else
#include <Eigen/Dense>

namespace gvox { inline namespace types {
    using f32vec2 = Eigen::Vector<float, 2>;
    using f32mat2x2 = Eigen::Matrix<float, 2, 2>;
    using f32mat2x3 = Eigen::Matrix<float, 2, 3>;
    using f32mat2x4 = Eigen::Matrix<float, 2, 4>;
    using f32mat2x5 = Eigen::Matrix<float, 2, 5>;
    using f32vec3 = Eigen::Vector<float, 3>;
    using f32mat3x2 = Eigen::Matrix<float, 3, 2>;
    using f32mat3x3 = Eigen::Matrix<float, 3, 3>;
    using f32mat3x4 = Eigen::Matrix<float, 3, 4>;
    using f32mat3x5 = Eigen::Matrix<float, 3, 5>;
    using f32vec4 = Eigen::Vector<float, 4>;
    using f32mat4x2 = Eigen::Matrix<float, 4, 2>;
    using f32mat4x3 = Eigen::Matrix<float, 4, 3>;
    using f32mat4x4 = Eigen::Matrix<float, 4, 4>;
    using f32mat4x5 = Eigen::Matrix<float, 4, 5>;
    using f32vec5 = Eigen::Vector<float, 5>;
    using f32mat5x2 = Eigen::Matrix<float, 5, 2>;
    using f32mat5x3 = Eigen::Matrix<float, 5, 3>;
    using f32mat5x4 = Eigen::Matrix<float, 5, 4>;
    using f32mat5x5 = Eigen::Matrix<float, 5, 5>;

    template <typename T, int M, int N, int M_, int N_>
    constexpr auto extend(Eigen::Matrix<T, M_, N_> const &x) {
        auto result = Eigen::Matrix<T, M, N>{};
        for (int mi = 0; mi < M_; ++mi) {
            for (int ni = 0; ni < N_; ++ni)
                result(mi, ni) = x(mi, ni);
        }
        return result;
    }

    template <typename T, int M, int N, int M_, int N_>
    constexpr auto truncate(Eigen::Matrix<T, M_, N_> const &x) {
        auto result = Eigen::Matrix<T, M, N>{};
        for (int mi = 0; mi < M; ++mi) {
            for (int ni = 0; ni < N; ++ni)
                result(mi, ni) = x(mi, ni);
        }
        return result;
    }
}} // namespace gvox::types

#endif
