#pragma once

#include <engine/core/utility/assert.h>
#include <engine/math/vector.h>

#include <cstdint>
#include <cmath>
#include <type_traits>
#include <utility>
#include <algorithm>

// Use index_sequence + fold expressions (guaranteed compile-time unroll).
#define USE_FAST_MATH_MATRIX


namespace math::detail {

#pragma region NamedAccess

template<typename T, uint32_t R, uint32_t C>
struct NamedMatrixAccess
{
	constexpr NamedMatrixAccess(T*) {}
};

template<typename T>
struct NamedMatrixAccess<T, 2, 2>
{
	T& m00; T& m01;
	T& m10; T& m11;
	constexpr NamedMatrixAccess(T* d)
		: m00(d[0]), m01(d[2]),
		  m10(d[1]), m11(d[3]) {}
};

template<typename T>
struct NamedMatrixAccess<T, 3, 3>
{
	T& m00; T& m01; T& m02;
	T& m10; T& m11; T& m12;
	T& m20; T& m21; T& m22;
	constexpr NamedMatrixAccess(T* d)
		: m00(d[0]), m01(d[3]), m02(d[6]),
		  m10(d[1]), m11(d[4]), m12(d[7]),
		  m20(d[2]), m21(d[5]), m22(d[8]) {}
};

template<typename T>
struct NamedMatrixAccess<T, 4, 4>
{
	T& m00; T& m01; T& m02; T& m03;
	T& m10; T& m11; T& m12; T& m13;
	T& m20; T& m21; T& m22; T& m23;
	T& m30; T& m31; T& m32; T& m33;
	constexpr NamedMatrixAccess(T* d)
		: m00(d[0]),  m01(d[4]),  m02(d[8]),  m03(d[12]),
		  m10(d[1]),  m11(d[5]),  m12(d[9]),  m13(d[13]),
		  m20(d[2]),  m21(d[6]),  m22(d[10]), m23(d[14]),
		  m30(d[3]),  m31(d[7]),  m32(d[11]), m33(d[15]) {}
};

#pragma endregion

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

	constexpr MatrixSimple(const MatrixSimple& other)
	{
		for (uint32_t i = 0; i < R; i++)
			for (uint32_t j = 0; j < C; j++)
				data[i][j] = other.data[i][j];
	}

	constexpr MatrixSimple(MatrixSimple&& other) noexcept
	{
		for (uint32_t i = 0; i < R; i++)
			for (uint32_t j = 0; j < C; j++)
				data[i][j] = std::move(other.data[i][j]);
	}

	constexpr MatrixSimple& operator=(const MatrixSimple& other)
	{
		if (this != &other)
		{
			for (uint32_t i = 0; i < R; i++)
				for (uint32_t j = 0; j < C; j++)
					data[i][j] = other.data[i][j];
		}
		return *this;
	}

	constexpr MatrixSimple& operator=(MatrixSimple&& other) noexcept
	{
		if (this != &other)
		{
			for (uint32_t i = 0; i < R; i++)
				for (uint32_t j = 0; j < C; j++)
					data[i][j] = std::move(other.data[i][j]);
		}
		return *this;
	}

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
struct MatrixUnrolledStorage
{
	T flat[R * C];
	constexpr MatrixUnrolledStorage() : flat{} {}
};

template<typename T>
struct MatrixUnrolledStorage<T, 2, 2>
{
	union
	{
		T flat[4];
		struct
		{
			T m00, m10;
			T m01, m11;
		};
	};
	constexpr MatrixUnrolledStorage() : flat{} {}
};

template<typename T>
struct MatrixUnrolledStorage<T, 3, 3>
{
	union
	{
		T flat[9];
		struct
		{
			T m00, m10, m20;
			T m01, m11, m21;
			T m02, m12, m22;
		};
	};
	constexpr MatrixUnrolledStorage() : flat{} {}
};

template<typename T>
struct MatrixUnrolledStorage<T, 4, 4>
{
	union
	{
		T flat[16];
		struct
		{
			T m00, m10, m20, m30;
			T m01, m11, m21, m31;
			T m02, m12, m22, m32;
			T m03, m13, m23, m33;
		};
	};
	constexpr MatrixUnrolledStorage() : flat{} {}
};

template<typename T, uint32_t R, uint32_t C>
struct MatrixUnrolled : MatrixUnrolledStorage<T, R, C>
{
	using MatrixUnrolledStorage<T, R, C>::flat;
	static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
	static_assert(R > 0 && C > 0, "R and C must be positive");

	// Flat column-major storage for GPU-friendly column-major convention
	// (storage lives in MatrixUnrolledStorage<T, R, C>)

	static constexpr uint32_t indexOf(uint32_t row, uint32_t col)
	{
		return col * R + row;
	}

	struct RowProxy
	{
		MatrixUnrolled& matrix;
		uint32_t row;
		constexpr T& operator[](uint32_t col) { ASSERT(col < C); return matrix.at(row, col); }
	};

	struct ConstRowProxy
	{
		const MatrixUnrolled& matrix;
		uint32_t row;
		constexpr const T& operator[](uint32_t col) const { ASSERT(col < C); return matrix.at(row, col); }
	};

	constexpr MatrixUnrolled() = default;
	constexpr MatrixUnrolled(std::initializer_list<T> init)
	{
		ASSERT(init.size() == R * C);
		auto it = init.begin();
		for (uint32_t row = 0; row < R; ++row)
		{
			for (uint32_t col = 0; col < C; ++col)
			{
				at(row, col) = *it++;
			}
		}
	}

	constexpr MatrixUnrolled(const MatrixUnrolled& other)
	{
		for (uint32_t i = 0; i < R * C; i++)
			flat[i] = other.flat[i];
	}

	constexpr MatrixUnrolled(MatrixUnrolled&& other) noexcept
	{
		for (uint32_t i = 0; i < R * C; i++)
			flat[i] = std::move(other.flat[i]);
	}

	constexpr MatrixUnrolled& operator=(const MatrixUnrolled& other)
	{
		if (this != &other)
		{
			for (uint32_t i = 0; i < R * C; i++)
				flat[i] = other.flat[i];
		}
		return *this;
	}

	constexpr MatrixUnrolled& operator=(MatrixUnrolled&& other) noexcept
	{
		if (this != &other)
		{
			for (uint32_t i = 0; i < R * C; i++)
				flat[i] = std::move(other.flat[i]);
		}
		return *this;
	}

	static constexpr MatrixUnrolled identity()
	{
		static_assert(R == C, "Identity requires a square matrix");
		MatrixUnrolled m;
		for (uint32_t i = 0; i < R; i++)
			m.at(i, i) = T(1);
		return m;
	}

	constexpr RowProxy operator[](uint32_t row) { ASSERT(row < R); return RowProxy{ *this, row }; }
	constexpr ConstRowProxy operator[](uint32_t row) const { ASSERT(row < R); return ConstRowProxy{ *this, row }; }

	constexpr T& at(uint32_t row, uint32_t col) { ASSERT(row < R && col < C); return flat[indexOf(row, col)]; }
	constexpr const T& at(uint32_t row, uint32_t col) const { ASSERT(row < R && col < C); return flat[indexOf(row, col)]; }

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

static_assert(sizeof(Matrix2x2) == sizeof(float) * 4,  "Matrix2x2 size mismatch");
static_assert(sizeof(Matrix3x3) == sizeof(float) * 9,  "Matrix3x3 size mismatch");
static_assert(sizeof(Matrix4x4) == sizeof(float) * 16, "Matrix4x4 size mismatch");
static_assert(sizeof(Matrix3x4) == sizeof(float) * 12, "Matrix3x4 size mismatch");
static_assert(sizeof(Matrix4x3) == sizeof(float) * 12, "Matrix4x3 size mismatch");

} // namespace math