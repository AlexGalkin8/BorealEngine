#pragma once

#include <engine/core/utility/assert.h>
#include <engine/math/vector.h>

#include <cmath>

namespace math {

struct Plane
{
	Vector3 normal;
	float d;

	constexpr Plane() : normal(0.0f, 1.0f, 0.0f), d(0.0f) {}
	constexpr Plane(const Vector3& normal, float d) : normal(normal), d(d) {}

	static Plane fromPointNormal(const Vector3& point, const Vector3& n)
	{
		float lenSq = n.dot(n);
		ASSERT_MSG(lenSq > 0.0f, "Plane normal lengthSq must be > 0");
		Vector3 nn = n / std::sqrt(lenSq);
		return Plane(nn, -nn.dot(point));
	}

	static Plane fromTriangle(const Vector3& a, const Vector3& b, const Vector3& c)
	{
		Vector3 ab = b - a;
		Vector3 ac = c - a;
		Vector3 n(
			ab[1] * ac[2] - ab[2] * ac[1],
			ab[2] * ac[0] - ab[0] * ac[2],
			ab[0] * ac[1] - ab[1] * ac[0]
		);
		return fromPointNormal(a, n);
	}

	constexpr float distanceToPoint(const Vector3& p) const
	{
		return normal.dot(p) + d;
	}

	constexpr Vector3 projectPoint(const Vector3& p) const
	{
		return p - normal * distanceToPoint(p);
	}

	Plane normalized() const
	{
		float lenSq = normal.dot(normal);
		ASSERT_MSG(lenSq > 0.0f, "Plane normal lengthSq must be > 0");
		float invLen = 1.0f / std::sqrt(lenSq);
		return Plane(normal * invLen, d * invLen);
	}

	void normalize()
	{
		float lenSq = normal.dot(normal);
		ASSERT_MSG(lenSq > 0.0f, "Plane normal lengthSq must be > 0");
		float invLen = 1.0f / std::sqrt(lenSq);
		normal *= invLen;
		d *= invLen;
	}
};

static_assert(sizeof(Plane) == sizeof(Vector3) + sizeof(float), "Plane size mismatch");

} // namespace math
