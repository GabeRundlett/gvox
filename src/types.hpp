#pragma once

#include <cstdint>
#include <cstddef>

#if 0
#include <span>
#include <array>

namespace gvox { inline namespace types {
    namespace detail {

        template <typename T, size_t N>
        struct GenericVector {
            std::array<T, N> array;
            constexpr T &operator[](size_t i) noexcept { return array[i]; }
            constexpr T const &operator[](size_t i) const noexcept { return array[i]; }

            constexpr T const &x() const { return array[0]; }
            constexpr T const &y() const { return array[1]; }
            constexpr T const &z() const { return array[2]; }
            constexpr T const &w() const { return array[3]; }
            constexpr T &x() { return array[0]; }
            constexpr T &y() { return array[1]; }
            constexpr T &z() { return array[2]; }
            constexpr T &w() { return array[3]; }
        };

        template <typename T, size_t M, size_t N>
        struct GenericMatrix : std::array<GenericVector<T, N>, M> {
        };
    } // namespace detail

    using f32vec2 = detail::GenericVector<float, 2>;
    using f32mat2x2 = detail::GenericMatrix<float, 2, 2>;
    using f32mat2x3 = detail::GenericMatrix<float, 2, 3>;
    using f32mat2x4 = detail::GenericMatrix<float, 2, 4>;
    using f32mat2x5 = detail::GenericMatrix<float, 2, 5>;
    using f32vec3 = detail::GenericVector<float, 3>;
    using f32mat3x2 = detail::GenericMatrix<float, 3, 2>;
    using f32mat3x3 = detail::GenericMatrix<float, 3, 3>;
    using f32mat3x4 = detail::GenericMatrix<float, 3, 4>;
    using f32mat3x5 = detail::GenericMatrix<float, 3, 5>;
    using f32vec4 = detail::GenericVector<float, 4>;
    using f32mat4x2 = detail::GenericMatrix<float, 4, 2>;
    using f32mat4x3 = detail::GenericMatrix<float, 4, 3>;
    using f32mat4x4 = detail::GenericMatrix<float, 4, 4>;
    using f32mat4x5 = detail::GenericMatrix<float, 4, 5>;
    using f32vec5 = detail::GenericVector<float, 5>;
    using f32mat5x2 = detail::GenericMatrix<float, 5, 2>;
    using f32mat5x3 = detail::GenericMatrix<float, 5, 3>;
    using f32mat5x4 = detail::GenericMatrix<float, 5, 4>;
    using f32mat5x5 = detail::GenericMatrix<float, 5, 5>;

    template <typename T, size_t N>
    constexpr auto operator+(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = a[i] + b[i];
        return result;
    }
    template <typename T, size_t N>
    constexpr auto operator-(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = a[i] - b[i];
        return result;
    }
    template <typename T, size_t N>
    constexpr auto operator*(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = a[i] * b[i];
        return result;
    }
    template <typename T, size_t N>
    constexpr auto operator/(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = a[i] / b[i];
        return result;
    }
    template <typename T, size_t N>
    constexpr auto dot(detail::GenericVector<T, N> const &a, detail::GenericVector<T, N> const &b) -> T {
        T result = 0;
        for (size_t i = 0; i < N; ++i)
            result += a[i] * b[i];
        return result;
    }

    template <typename T, size_t N>
    constexpr auto operator+(detail::GenericVector<T, N> const &a, T b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = a[i] + b;
        return result;
    }
    template <typename T, size_t N>
    constexpr auto operator-(detail::GenericVector<T, N> const &a, T b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = a[i] - b;
        return result;
    }
    template <typename T, size_t N>
    constexpr auto operator*(detail::GenericVector<T, N> const &a, T b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = a[i] * b;
        return result;
    }
    template <typename T, size_t N>
    constexpr auto operator/(detail::GenericVector<T, N> const &a, T b) -> detail::GenericVector<T, N> {
        detail::GenericVector<T, N> result;
        for (size_t i = 0; i < N; ++i)
            result[i] = a[i] / b;
        return result;
    }

    template <typename T, size_t M, size_t N, size_t P>
    constexpr auto operator*(detail::GenericMatrix<T, M, N> const &a, detail::GenericMatrix<T, N, P> const &b) {
        auto c = detail::GenericMatrix<T, M, P>{};
        for (size_t i = 0; i < M; ++i) {
            for (size_t j = 0; j < P; ++j) {
                c[i][j] = 0;
                for (size_t k = 0; k < N; ++k)
                    c[i][j] += a[i][k] * b[k][j];
            }
        }
        return c;
    }
    template <typename T, size_t M, size_t N>
    constexpr auto operator*(detail::GenericMatrix<T, M, N> const &a, detail::GenericVector<T, N> const &v) {
        auto c = detail::GenericVector<T, M>{};
        for (size_t i = 0; i < M; ++i) {
            c[i] = 0;
            for (size_t k = 0; k < N; ++k)
                c[i] += a[i][k] * v[k];
        }
        return c;
    }
    template <typename T, size_t N, size_t P>
    constexpr auto operator*(detail::GenericVector<T, N> const &v, detail::GenericMatrix<T, N, P> const &b) {
        auto c = detail::GenericVector<std::remove_cv_t<T>, P>{};
        for (size_t j = 0; j < P; ++j) {
            c[j] = 0;
            for (size_t k = 0; k < N; ++k)
                c[j] += v[k] * b[k][j];
        }
        return c;
    }

    template <typename T, size_t M, size_t N>
    constexpr auto transpose(detail::GenericMatrix<T, M, N> const &x) {
        auto result = detail::GenericMatrix<T, N, M>{};
        for (size_t mi = 0; mi < M; ++mi) {
            for (size_t ni = 0; ni < N; ++ni)
                result[ni][mi] = x[mi][ni];
        }
        return result;
    }

    template <typename T, size_t M, size_t N, size_t M_, size_t N_>
    constexpr auto extend(detail::GenericMatrix<std::remove_cv_t<T>, M_, N_> const &x) {
        auto result = detail::GenericMatrix<T, M, N>{};
        for (size_t mi = 0; mi < M_; ++mi) {
            for (size_t ni = 0; ni < N_; ++ni)
                result[mi][ni] = x[mi][ni];
        }
        return result;
    }

    template <typename T, size_t M, size_t N, size_t M_, size_t N_>
    constexpr auto truncate(detail::GenericMatrix<std::remove_cv_t<T>, M_, N_> const &x) {
        auto result = detail::GenericMatrix<T, M, N>{};
        for (size_t mi = 0; mi < M; ++mi) {
            for (size_t ni = 0; ni < N; ++ni)
                result[mi][ni] = x[mi][ni];
        }
        return result;
    }

    template <typename T, size_t N>
    constexpr auto vec_from_span(std::span<T, N> const x) -> detail::GenericVector<std::remove_cv_t<T>, N> {
        auto result = detail::GenericVector<std::remove_cv_t<T>, N>{};
        for (size_t i = 0; i < N; ++i)
            result[i] = x[i];
        return result;
    }

    template <typename T, size_t M, size_t N>
    constexpr auto mat_from_span(std::span<T, M * N> const x) -> detail::GenericMatrix<std::remove_cv_t<T>, M, N> {
        auto result = detail::GenericMatrix<std::remove_cv_t<T>, M, N>{};
        for (size_t mi = 0; mi < M; ++mi) {
            for (size_t ni = 0; ni < N; ++ni)
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
        auto result = Eigen::Matrix<std::remove_cv_t<T>, M, N>{};
        for (int mi = 0; mi < M; ++mi) {
            for (int ni = 0; ni < N; ++ni) {
                if (mi < M_ && ni < N_) {
                    result(mi, ni) = x(mi, ni);
                } else {
                    result(mi, ni) = 0;
                }
            }
        }
        return result;
    }

    template <typename T, int M, int N, int M_, int N_>
    constexpr auto truncate(Eigen::Matrix<T, M_, N_> const &x) {
        auto result = Eigen::Matrix<std::remove_cv_t<T>, M, N>{};
        for (int mi = 0; mi < M; ++mi) {
            for (int ni = 0; ni < N; ++ni)
                result(mi, ni) = x(mi, ni);
        }
        return result;
    }
}} // namespace gvox::types

#endif
