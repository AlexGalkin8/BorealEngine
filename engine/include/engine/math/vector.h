#pragma once

#include <engine/core/utility/assert.h>

#include <cstdint>
#include <cmath>
#include <type_traits>
#include <utility>

// Use index_sequence + fold expressions (guaranteed compile-time unroll).
#define USE_FAST_MATH_VECTOR


namespace math::detail {

template<typename T, uint32_t N>
struct VectorStorage
{
	T data[N];
	constexpr VectorStorage() : data{} {}
};

template<typename T>
struct VectorStorage<T, 1>
{
	union
	{
		T data[1];
		struct { T x; };
	};
	constexpr VectorStorage() : data{} {}
};

template<typename T>
struct VectorStorage<T, 2>
{
	union
	{
		T data[2];
		struct { T x, y; };
	};
	constexpr VectorStorage() : data{} {}
};

template<typename T>
struct VectorStorage<T, 3>
{
	union
	{
		T data[3];
		struct { T x, y, z; };
	};
	constexpr VectorStorage() : data{} {}
};

template<typename T>
struct VectorStorage<T, 4>
{
	union
	{
		T data[4];
		struct { T x, y, z, w; };
		struct { T r, g, b, a; };
	};
	constexpr VectorStorage() : data{} {}
};

#ifndef USE_FAST_MATH_VECTOR

// ============================================================================
//  VectorSimple - plain loops
// ============================================================================
template<typename T, uint32_t N>
struct VectorSimple : VectorStorage<T, N>
{
	using VectorStorage<T, N>::data;
	static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
	static_assert(N > 0, "N must be positive");

	constexpr VectorSimple() = default;

	constexpr VectorSimple(T value)
	{
		for (uint32_t i = 0; i < N; i++)
			data[i] = value;
	}

	template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == N>>
	constexpr VectorSimple(Args... args)
	{
		const T values[N] = { static_cast<T>(args)... };
		for (uint32_t i = 0; i < N; i++)
			data[i] = values[i];
	}

	constexpr VectorSimple(const VectorSimple& other)
	{
		for (uint32_t i = 0; i < N; i++)
			data[i] = other.data[i];
	}

	constexpr VectorSimple(VectorSimple&& other) noexcept
	{
		for (uint32_t i = 0; i < N; i++)
			data[i] = std::move(other.data[i]);
	}

	constexpr VectorSimple& operator=(const VectorSimple& other)
	{
		if (this != &other)
		{
			for (uint32_t i = 0; i < N; i++)
				data[i] = other.data[i];
		}
		return *this;
	}

	constexpr VectorSimple& operator=(VectorSimple&& other) noexcept
	{
		if (this != &other)
		{
			for (uint32_t i = 0; i < N; i++)
				data[i] = std::move(other.data[i]);
		}
		return *this;
	}

	constexpr T&       operator[](uint32_t i)       { ASSERT(i < N); return data[i]; }
	constexpr const T& operator[](uint32_t i) const { ASSERT(i < N); return data[i]; }

	constexpr VectorSimple operator+(const VectorSimple& v) const { VectorSimple r; for (uint32_t i = 0; i < N; i++) r[i] = data[i] + v[i]; return r; }
	constexpr VectorSimple operator-(const VectorSimple& v) const { VectorSimple r; for (uint32_t i = 0; i < N; i++) r[i] = data[i] - v[i]; return r; }
	constexpr VectorSimple operator*(T s)             const { VectorSimple r; for (uint32_t i = 0; i < N; i++) r[i] = data[i] * s;    return r; }
	constexpr VectorSimple operator/(T s)             const { VectorSimple r; for (uint32_t i = 0; i < N; i++) r[i] = data[i] / s;    return r; }
	constexpr VectorSimple operator-()                const { VectorSimple r; for (uint32_t i = 0; i < N; i++) r[i] = -data[i];       return r; }

	constexpr VectorSimple& operator+=(const VectorSimple& v) { *this = *this + v; return *this; }
	constexpr VectorSimple& operator-=(const VectorSimple& v) { *this = *this - v; return *this; }
	constexpr VectorSimple& operator*=(T s)                   { *this = *this * s; return *this; }
	constexpr VectorSimple& operator/=(T s)                   { *this = *this / s; return *this; }

	constexpr bool operator==(const VectorSimple& v) const { for (uint32_t i = 0; i < N; i++) if (data[i] != v[i]) return false; return true; }
	constexpr bool operator!=(const VectorSimple& v) const { return !(*this == v); }

	constexpr T dot(const VectorSimple& v) const
	{
		T r = T(0);
		for (uint32_t i = 0; i < N; i++)
			r += data[i] * v[i];
		return r;
	}

	constexpr T lengthSq() const { return dot(*this); }
	T           length()   const { return std::sqrt(lengthSq()); }

	VectorSimple normalize() const
	{
		T len = length();
		ASSERT(len > T(0));
		return *this / len;
	}

	static constexpr VectorSimple lerp(const VectorSimple& a, const VectorSimple& b, T t)
	{
		return a + (b - a) * t;
	}

	static constexpr VectorSimple min(const VectorSimple& a, const VectorSimple& b)
	{
		VectorSimple r;
		for (uint32_t i = 0; i < N; i++)
			r[i] = a[i] < b[i] ? a[i] : b[i];
		return r;
	}

	static constexpr VectorSimple max(const VectorSimple& a, const VectorSimple& b)
	{
		VectorSimple r;
		for (uint32_t i = 0; i < N; i++)
			r[i] = a[i] > b[i] ? a[i] : b[i];
		return r;
	}
};

template<typename T, uint32_t N>
constexpr VectorSimple<T, N> operator*(T s, const VectorSimple<T, N>& v) { return v * s; }

#else

// ============================================================================
//  VectorUnrolled - index_sequence + fold expressions
// ============================================================================

template<typename T, uint32_t N, typename Op, std::size_t... I>
constexpr auto applyBinary(const T* a, const T* b, Op op, std::index_sequence<I...>)
{
	struct Pack { T data[N]; };
	return Pack{ op(a[I], b[I])... };
}

template<typename T, uint32_t N, typename Op, std::size_t... I>
constexpr auto applyUnary(const T* a, Op op, std::index_sequence<I...>)
{
	struct Pack { T data[N]; };
	return Pack{ op(a[I])... };
}

template<typename T, typename Op, std::size_t... I>
constexpr T foldBinary(const T* a, const T* b, Op op, std::index_sequence<I...>)
{
	return (op(a[I], b[I]) + ...);
}

template<typename T, std::size_t... I>
constexpr bool foldEquals(const T* a, const T* b, std::index_sequence<I...>)
{
	return ((a[I] == b[I]) && ...);
}


template<typename T, uint32_t N>
struct VectorUnrolled : VectorStorage<T, N>
{
	using VectorStorage<T, N>::data;
	static_assert(std::is_arithmetic_v<T>, "T must be arithmetic");
	static_assert(N > 0, "N must be positive");

	constexpr VectorUnrolled() = default;

	constexpr VectorUnrolled(T value)
	{
		auto p = applyUnary<T, N>(data, [value](T) constexpr { return value; }, std::make_index_sequence<N>{});
		for (uint32_t i = 0; i < N; i++) data[i] = p.data[i];
	}

	template<typename... Args, typename = std::enable_if_t<sizeof...(Args) == N>>
	constexpr VectorUnrolled(Args... args)
	{
		const T values[N] = { static_cast<T>(args)... };
		for (uint32_t i = 0; i < N; i++)
			data[i] = values[i];
	}

	constexpr VectorUnrolled(const VectorUnrolled& other)
	{
		for (uint32_t i = 0; i < N; i++)
			data[i] = other.data[i];
	}

	constexpr VectorUnrolled(VectorUnrolled&& other) noexcept
	{
		for (uint32_t i = 0; i < N; i++)
			data[i] = std::move(other.data[i]);
	}

	constexpr VectorUnrolled& operator=(const VectorUnrolled& other)
	{
		if (this != &other)
		{
			for (uint32_t i = 0; i < N; i++)
				data[i] = other.data[i];
		}
		return *this;
	}

	constexpr VectorUnrolled& operator=(VectorUnrolled&& other) noexcept
	{
		if (this != &other)
		{
			for (uint32_t i = 0; i < N; i++)
				data[i] = std::move(other.data[i]);
		}
		return *this;
	}

	constexpr T&       operator[](uint32_t i)       { ASSERT(i < N); return data[i]; }
	constexpr const T& operator[](uint32_t i) const { ASSERT(i < N); return data[i]; }

	constexpr VectorUnrolled operator+(const VectorUnrolled& v) const
	{
		auto p = applyBinary<T, N>(data, v.data, [](T a, T b) constexpr { return a + b; }, std::make_index_sequence<N>{});
		VectorUnrolled r; for (uint32_t i = 0; i < N; i++) r.data[i] = p.data[i]; return r;
	}

	constexpr VectorUnrolled operator-(const VectorUnrolled& v) const
	{
		auto p = applyBinary<T, N>(data, v.data, [](T a, T b) constexpr { return a - b; }, std::make_index_sequence<N>{});
		VectorUnrolled r; for (uint32_t i = 0; i < N; i++) r.data[i] = p.data[i]; return r;
	}

	constexpr VectorUnrolled operator*(T s) const
	{
		auto p = applyUnary<T, N>(data, [s](T a) constexpr { return a * s; }, std::make_index_sequence<N>{});
		VectorUnrolled r; for (uint32_t i = 0; i < N; i++) r.data[i] = p.data[i]; return r;
	}

	constexpr VectorUnrolled operator/(T s) const
	{
		auto p = applyUnary<T, N>(data, [s](T a) constexpr { return a / s; }, std::make_index_sequence<N>{});
		VectorUnrolled r; for (uint32_t i = 0; i < N; i++) r.data[i] = p.data[i]; return r;
	}

	constexpr VectorUnrolled operator-() const
	{
		auto p = applyUnary<T, N>(data, [](T a) constexpr { return -a; }, std::make_index_sequence<N>{});
		VectorUnrolled r; for (uint32_t i = 0; i < N; i++) r.data[i] = p.data[i]; return r;
	}

	constexpr VectorUnrolled& operator+=(const VectorUnrolled& v) { *this = *this + v; return *this; }
	constexpr VectorUnrolled& operator-=(const VectorUnrolled& v) { *this = *this - v; return *this; }
	constexpr VectorUnrolled& operator*=(T s)                     { *this = *this * s; return *this; }
	constexpr VectorUnrolled& operator/=(T s)                     { *this = *this / s; return *this; }

	constexpr bool operator==(const VectorUnrolled& v) const
	{
		return foldEquals<T>(data, v.data, std::make_index_sequence<N>{});
	}

	constexpr bool operator!=(const VectorUnrolled& v) const { return !(*this == v); }

	constexpr T dot(const VectorUnrolled& v) const
	{
		return foldBinary<T>(data, v.data, [](T a, T b) constexpr { return a * b; }, std::make_index_sequence<N>{});
	}

	constexpr VectorUnrolled cross(const VectorUnrolled& v) const
	{
		static_assert(N == 3, "Cross product is only defined for 3D vectors");
		return VectorUnrolled(
			data[1] * v.data[2] - data[2] * v.data[1],
			data[2] * v.data[0] - data[0] * v.data[2],
			data[0] * v.data[1] - data[1] * v.data[0]
		);
	}

	constexpr T lengthSq() const { return dot(*this); }
	T           length()   const { return std::sqrt(lengthSq()); }

	VectorUnrolled normalize() const
	{
		T len = length();
		ASSERT(len > T(0));
		return *this / len;
	}

	static constexpr VectorUnrolled lerp(const VectorUnrolled& a, const VectorUnrolled& b, T t)
	{
		auto p = applyBinary<T, N>(a.data, b.data, [t](T x, T y) constexpr { return x + (y - x) * t; }, std::make_index_sequence<N>{});
		VectorUnrolled r; for (uint32_t i = 0; i < N; i++) r.data[i] = p.data[i]; return r;
	}

	static constexpr VectorUnrolled min(const VectorUnrolled& a, const VectorUnrolled& b)
	{
		auto p = applyBinary<T, N>(a.data, b.data, [](T x, T y) constexpr { return x < y ? x : y; }, std::make_index_sequence<N>{});
		VectorUnrolled r; for (uint32_t i = 0; i < N; i++) r.data[i] = p.data[i]; return r;
	}

	static constexpr VectorUnrolled max(const VectorUnrolled& a, const VectorUnrolled& b)
	{
		auto p = applyBinary<T, N>(a.data, b.data, [](T x, T y) constexpr { return x > y ? x : y; }, std::make_index_sequence<N>{});
		VectorUnrolled r; for (uint32_t i = 0; i < N; i++) r.data[i] = p.data[i]; return r;
	}
};

template<typename T, uint32_t N>
constexpr VectorUnrolled<T, N> operator*(T s, const VectorUnrolled<T, N>& v) { return v * s; }

#endif // !USE_FAST_MATH_VECTOR


// ============================================================================
//  Vector alias - select implementation via define
// ============================================================================

#ifdef USE_FAST_MATH_VECTOR
	template<typename T, uint32_t N>
	using Vector = VectorUnrolled<T, N>;
#else
	template<typename T, uint32_t N>
	using Vector = VectorSimple<T, N>;
#endif

} // namespace math::detail


// ============================================================================
//  Public API - only these types are visible in namespace math
// ============================================================================

namespace math {

using Vector2 = detail::Vector<float,    2>;
using Vector3 = detail::Vector<float,    3>;
using Vector4 = detail::Vector<float,    4>;
using Int2    = detail::Vector<int32_t,  2>;
using Int3    = detail::Vector<int32_t,  3>;
using Int4    = detail::Vector<int32_t,  4>;
using UInt2   = detail::Vector<uint32_t, 2>;
using UInt3   = detail::Vector<uint32_t, 3>;
using UInt4   = detail::Vector<uint32_t, 4>;

static_assert(sizeof(Vector2) == sizeof(float) * 2, "Vector2 size mismatch");
static_assert(sizeof(Vector3) == sizeof(float) * 3, "Vector3 size mismatch");
static_assert(sizeof(Vector4) == sizeof(float) * 4, "Vector4 size mismatch");
static_assert(sizeof(Int2)    == sizeof(int32_t) * 2, "Int2 size mismatch");
static_assert(sizeof(Int3)    == sizeof(int32_t) * 3, "Int3 size mismatch");
static_assert(sizeof(Int4)    == sizeof(int32_t) * 4, "Int4 size mismatch");
static_assert(sizeof(UInt2)   == sizeof(uint32_t) * 2, "UInt2 size mismatch");
static_assert(sizeof(UInt3)   == sizeof(uint32_t) * 3, "UInt3 size mismatch");
static_assert(sizeof(UInt4)   == sizeof(uint32_t) * 4, "UInt4 size mismatch");

} // namespace math
