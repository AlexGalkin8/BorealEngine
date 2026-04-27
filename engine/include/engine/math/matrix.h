#pragma once

#include <engine/core/utility/assert.h>
#include <engine/math/vector.h>

#include <cstdint>
#include <cmath>
#include <type_traits>
#include <utility>

// Use index_sequence + fold expressions (guaranteed compile-time unroll).
#define USE_FAST_MATH_MATRIX


namespace math::detail {

#ifndef  USE_FAST_MATH_MATRIX

// ============================================================================
//  MatrixSimple - plain loops, row-major storage
// ============================================================================

template<typename T, uint32_t R, uint32_t C>
struct MatrixSimple
{
    static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
    static_assert(R > 0 && C > 0, "R and C must be positive");

    T data[R][C] = {};

    constexpr MatrixSimple() = default;

    // Identity matrix
    static constexpr MatrixSimple identity()
    {
        static_assert(R == C, "Identity requires a square matrix");
        MatrixSimple m;
        for (uint32_t i = 0; i < R; i++)
            m.data[i][i] = T(1);
        return m;
    }

    // Row access
    constexpr T*       operator[](uint32_t row)       { ASSERT(row < R); return data[row]; }
    constexpr const T* operator[](uint32_t row) const { ASSERT(row < R); return data[row]; }

    // Element access
    constexpr T&       at(uint32_t row, uint32_t col)       { ASSERT(row < R && col < C); return data[row][col]; }
    constexpr const T& at(uint32_t row, uint32_t col) const { ASSERT(row < R && col < C); return data[row][col]; }

    // Arithmetic operators
    constexpr MatrixSimple operator+(const MatrixSimple& m) const
    {
        MatrixSimple r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t j = 0; j < C; j++)
                r[i][j] = data[i][j] + m[i][j];
        return r;
    }

    constexpr MatrixSimple operator-(const MatrixSimple& m) const
    {
        MatrixSimple r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t j = 0; j < C; j++)
                r[i][j] = data[i][j] - m[i][j];
        return r;
    }

    constexpr MatrixSimple operator*(T s) const
    {
        MatrixSimple r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t j = 0; j < C; j++)
                r[i][j] = data[i][j] * s;
        return r;
    }

    constexpr MatrixSimple operator-() const
    {
        MatrixSimple r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t j = 0; j < C; j++)
                r[i][j] = -data[i][j];
        return r;
    }

    constexpr MatrixSimple& operator+=(const MatrixSimple& m) { *this = *this + m; return *this; }
    constexpr MatrixSimple& operator-=(const MatrixSimple& m) { *this = *this - m; return *this; }
    constexpr MatrixSimple& operator*=(T s)                   { *this = *this * s; return *this; }

    constexpr bool operator==(const MatrixSimple& m) const
    {
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t j = 0; j < C; j++)
                if (data[i][j] != m[i][j]) return false;
        return true;
    }

    constexpr bool operator!=(const MatrixSimple& m) const { return !(*this == m); }

    // Matrix multiplication: (R x C) * (C x K) = (R x K)
    template<uint32_t K>
    constexpr MatrixSimple<T, R, K> operator*(const MatrixSimple<T, C, K>& m) const
    {
        MatrixSimple<T, R, K> r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t k = 0; k < K; k++)
                for (uint32_t j = 0; j < C; j++)
                    r[i][k] += data[i][j] * m[j][k];
        return r;
    }

    // Matrix * Vector (column vector): (R x C) * (C x 1) = (R x 1)
    constexpr VectorSimple<T, R> operator*(const VectorSimple<T, C>& v) const
    {
        VectorSimple<T, R> r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t j = 0; j < C; j++)
                r[i] += data[i][j] * v[j];
        return r;
    }

    // Transpose: (R x C) -> (C x R)
    constexpr MatrixSimple<T, C, R> transpose() const
    {
        MatrixSimple<T, C, R> r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t j = 0; j < C; j++)
                r[j][i] = data[i][j];
        return r;
    }
};

template<typename T, uint32_t R, uint32_t C>
constexpr MatrixSimple<T, R, C> operator*(T s, const MatrixSimple<T, R, C>& m) { return m * s; }

#else

// ============================================================================
//  MatrixUnrolled - index_sequence + fold expressions
// ============================================================================

template<typename T, uint32_t R, uint32_t C, typename Op, std::size_t... I>
constexpr auto applyMatBinary(const T* a, const T* b, Op op, std::index_sequence<I...>)
{
    struct Pack { T data[R * C]; };
    return Pack{ op(a[I], b[I])... };
}

template<typename T, uint32_t R, uint32_t C, typename Op, std::size_t... I>
constexpr auto applyMatUnary(const T* a, Op op, std::index_sequence<I...>)
{
    struct Pack { T data[R * C]; };
    return Pack{ op(a[I])... };
}

template<typename T, uint32_t R, uint32_t C, std::size_t... I>
constexpr bool foldMatEquals(const T* a, const T* b, std::index_sequence<I...>)
{
    return ((a[I] == b[I]) && ...);
}

template<typename T, uint32_t R, uint32_t C>
struct MatrixUnrolled
{
    static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
    static_assert(R > 0 && C > 0, "R and C must be positive");

    // Flat row-major storage for index_sequence access
    T flat[R * C] = {};

    constexpr MatrixUnrolled() = default;

    static constexpr MatrixUnrolled identity()
    {
        static_assert(R == C, "Identity requires a square matrix");
        MatrixUnrolled m;
        for (uint32_t i = 0; i < R; i++)
            m.flat[i * C + i] = T(1);
        return m;
    }

    constexpr T*       operator[](uint32_t row)       { ASSERT(row < R); return &flat[row * C]; }
    constexpr const T* operator[](uint32_t row) const { ASSERT(row < R); return &flat[row * C]; }

    constexpr T&       at(uint32_t row, uint32_t col)       { ASSERT(row < R && col < C); return flat[row * C + col]; }
    constexpr const T& at(uint32_t row, uint32_t col) const { ASSERT(row < R && col < C); return flat[row * C + col]; }

    constexpr MatrixUnrolled operator+(const MatrixUnrolled& m) const
    {
        auto p = applyMatBinary<T, R, C>(flat, m.flat, [](T a, T b) constexpr { return a + b; }, std::make_index_sequence<R * C>{});
        MatrixUnrolled r; for (uint32_t i = 0; i < R * C; i++) r.flat[i] = p.data[i]; return r;
    }

    constexpr MatrixUnrolled operator-(const MatrixUnrolled& m) const
    {
        auto p = applyMatBinary<T, R, C>(flat, m.flat, [](T a, T b) constexpr { return a - b; }, std::make_index_sequence<R * C>{});
        MatrixUnrolled r; for (uint32_t i = 0; i < R * C; i++) r.flat[i] = p.data[i]; return r;
    }

    constexpr MatrixUnrolled operator*(T s) const
    {
        auto p = applyMatUnary<T, R, C>(flat, [s](T a) constexpr { return a * s; }, std::make_index_sequence<R * C>{});
        MatrixUnrolled r; for (uint32_t i = 0; i < R * C; i++) r.flat[i] = p.data[i]; return r;
    }

    constexpr MatrixUnrolled operator-() const
    {
        auto p = applyMatUnary<T, R, C>(flat, [](T a) constexpr { return -a; }, std::make_index_sequence<R * C>{});
        MatrixUnrolled r; for (uint32_t i = 0; i < R * C; i++) r.flat[i] = p.data[i]; return r;
    }

    constexpr MatrixUnrolled& operator+=(const MatrixUnrolled& m) { *this = *this + m; return *this; }
    constexpr MatrixUnrolled& operator-=(const MatrixUnrolled& m) { *this = *this - m; return *this; }
    constexpr MatrixUnrolled& operator*=(T s)                     { *this = *this * s; return *this; }

    constexpr bool operator==(const MatrixUnrolled& m) const
    {
        return foldMatEquals<T, R, C>(flat, m.flat, std::make_index_sequence<R * C>{});
    }

    constexpr bool operator!=(const MatrixUnrolled& m) const { return !(*this == m); }

    // Matrix multiplication: (R x C) * (C x K) = (R x K)
    template<uint32_t K>
    constexpr MatrixUnrolled<T, R, K> operator*(const MatrixUnrolled<T, C, K>& m) const
    {
        MatrixUnrolled<T, R, K> r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t k = 0; k < K; k++)
                for (uint32_t j = 0; j < C; j++)
                    r[i][k] += (*this)[i][j] * m[j][k];
        return r;
    }

    // Matrix * Vector (column vector): (R x C) * (C x 1) = (R x 1)
    constexpr VectorUnrolled<T, R> operator*(const VectorUnrolled<T, C>& v) const
    {
        VectorUnrolled<T, R> r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t j = 0; j < C; j++)
                r[i] += (*this)[i][j] * v[j];
        return r;
    }

    // Transpose: (R x C) -> (C x R)
    constexpr MatrixUnrolled<T, C, R> transpose() const
    {
        MatrixUnrolled<T, C, R> r;
        for (uint32_t i = 0; i < R; i++)
            for (uint32_t j = 0; j < C; j++)
                r[j][i] = (*this)[i][j];
        return r;
    }
};

template<typename T, uint32_t R, uint32_t C>
constexpr MatrixUnrolled<T, R, C> operator*(T s, const MatrixUnrolled<T, R, C>& m) { return m * s; }

#endif // ! USE_FAST_MATH_MATRIX


// ============================================================================
//  Matrix alias - select implementation via define
// ============================================================================

#ifdef USE_FAST_MATH_MATRIX
    template<typename T, uint32_t R, uint32_t C>
    using Matrix = MatrixUnrolled<T, R, C>;
#else
    template<typename T, uint32_t R, uint32_t C>
    using Matrix = MatrixSimple<T, R, C>;
#endif

} // namespace math::detail


// ============================================================================
//  Public API - only these types are visible in namespace math
// ============================================================================

namespace math {

using Matrix2x2 = detail::Matrix<float, 2, 2>;
using Matrix3x3 = detail::Matrix<float, 3, 3>;
using Matrix4x4 = detail::Matrix<float, 4, 4>;
using Matrix3x4 = detail::Matrix<float, 3, 4>;
using Matrix4x3 = detail::Matrix<float, 4, 3>;

} // namespace math