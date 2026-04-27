#pragma once

#include <engine/math/vector.h>
#include <engine/math/aabb.h>
#include <engine/math/math_utils.h>

namespace math {

struct Sphere
{
    Vector3 center;
    float radius;

    constexpr Sphere() : center(0.0f, 0.0f, 0.0f), radius(0.0f) {}
    constexpr Sphere(const Vector3& center, float radius) : center(center), radius(radius) {}

    constexpr float radiusSq() const { return radius * radius; }

    constexpr bool contains(const Vector3& p) const
    {
        Vector3 d = p - center;
        return d.dot(d) <= radiusSq();
    }

    constexpr bool intersects(const Sphere& s) const
    {
        float rr = radius + s.radius;
        Vector3 d = center - s.center;
        return d.dot(d) <= rr * rr;
    }

    constexpr bool intersects(const AABB& b) const
    {
        float x = max(b.min[0], min(center[0], b.max[0]));
        float y = max(b.min[1], min(center[1], b.max[1]));
        float z = max(b.min[2], min(center[2], b.max[2]));
        Vector3 closest(x, y, z);
        Vector3 d = closest - center;
        return d.dot(d) <= radiusSq();
    }
};

} // namespace math
