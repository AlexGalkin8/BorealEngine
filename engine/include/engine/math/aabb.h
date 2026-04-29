#pragma once

#include <engine/math/vector.h>
#include <engine/math/transform.h>

#include <algorithm>
#include <limits>

namespace math {

struct AABB
{
	Vector3 min;
	Vector3 max;

	constexpr AABB()
		: min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
		, max(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max())
	{
	}

	constexpr AABB(const Vector3& min, const Vector3& max)
		: min(min), max(max)
	{
	}

	static constexpr AABB fromCenterExtent(const Vector3& center, const Vector3& extent)
	{
		return AABB(center - extent, center + extent);
	}

	constexpr Vector3 size() const { return max - min; }
	constexpr Vector3 extent() const { return (max - min) * 0.5f; }
	constexpr Vector3 center() const { return (min + max) * 0.5f; }

	constexpr bool isValid() const
	{
		return min[0] <= max[0] && min[1] <= max[1] && min[2] <= max[2];
	}

	constexpr bool contains(const Vector3& p) const
	{
		return p[0] >= min[0] && p[0] <= max[0]
			&& p[1] >= min[1] && p[1] <= max[1]
			&& p[2] >= min[2] && p[2] <= max[2];
	}

	constexpr bool contains(const AABB& b) const
	{
		return contains(b.min) && contains(b.max);
	}

	constexpr bool intersects(const AABB& b) const
	{
		return !(max[0] < b.min[0] || min[0] > b.max[0]
			|| max[1] < b.min[1] || min[1] > b.max[1]
			|| max[2] < b.min[2] || min[2] > b.max[2]);
	}

	void expand(const Vector3& p)
	{
		min = Vector3::min(min, p);
		max = Vector3::max(max, p);
	}

	void expand(const AABB& b)
	{
		expand(b.min);
		expand(b.max);
	}

	constexpr AABB merged(const AABB& b) const
	{
		return AABB(Vector3::min(min, b.min), Vector3::max(max, b.max));
	}

	// Transform AABB by transforming 8 corners and recomputing bounds
	AABB transformed(const Transform& t) const
	{
		Vector3 corners[8] = {
			Vector3(min[0], min[1], min[2]),
			Vector3(max[0], min[1], min[2]),
			Vector3(min[0], max[1], min[2]),
			Vector3(max[0], max[1], min[2]),
			Vector3(min[0], min[1], max[2]),
			Vector3(max[0], min[1], max[2]),
			Vector3(min[0], max[1], max[2]),
			Vector3(max[0], max[1], max[2])
		};

		AABB out;
		for (const Vector3& c : corners)
		{
			out.expand(t.transformPoint(c));
		}
		return out;
	}
};

static_assert(sizeof(AABB) == sizeof(Vector3) * 2, "AABB size mismatch");

} // namespace math

