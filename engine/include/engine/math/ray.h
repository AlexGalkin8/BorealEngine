#pragma once

#include <engine/math/vector.h>
#include <engine/math/plane.h>
#include <engine/math/sphere.h>
#include <engine/math/aabb.h>
#include <engine/math/math_utils.h>

#include <cmath>
#include <limits>

namespace math {

struct Ray
{
    Vector3 origin;
    Vector3 direction;

    constexpr Ray() : origin(0.0f, 0.0f, 0.0f), direction(0.0f, 0.0f, 1.0f) {}
    constexpr Ray(const Vector3& origin, const Vector3& direction) : origin(origin), direction(direction) {}

    constexpr Vector3 pointAt(float t) const { return origin + direction * t; }

    bool intersectsPlane(const Plane& plane, float& t) const
    {
        float denom = plane.normal.dot(direction);
        if (abs(denom) < constants::kEpsilon)
            return false;
        t = -(plane.normal.dot(origin) + plane.d) / denom;
        return t >= 0.0f;
    }

    bool intersectsSphere(const Sphere& sphere, float& tNear, float& tFar) const
    {
        Vector3 oc = origin - sphere.center;
        float a = direction.dot(direction);
        float b = 2.0f * oc.dot(direction);
        float c = oc.dot(oc) - sphere.radius * sphere.radius;
        float disc = b * b - 4.0f * a * c;
        if (disc < 0.0f)
            return false;
        float sqrtDisc = std::sqrt(disc);
        tNear = (-b - sqrtDisc) / (2.0f * a);
        tFar = (-b + sqrtDisc) / (2.0f * a);
        if (tNear > tFar)
        {
            float tmp = tNear;
            tNear = tFar;
            tFar = tmp;
        }
        return tFar >= 0.0f;
    }

    bool intersectsAABB(const AABB& box, float& tNear, float& tFar) const
    {
        tNear = -std::numeric_limits<float>::infinity();
        tFar = std::numeric_limits<float>::infinity();

        for (uint32_t axis = 0; axis < 3; ++axis)
        {
            float o = origin[axis];
            float d = direction[axis];
            float minv = box.min[axis];
            float maxv = box.max[axis];

            if (abs(d) < constants::kEpsilon)
            {
                if (o < minv || o > maxv)
                    return false;
                continue;
            }

            float invD = 1.0f / d;
            float t0 = (minv - o) * invD;
            float t1 = (maxv - o) * invD;
            if (t0 > t1)
            {
                float tmp = t0;
                t0 = t1;
                t1 = tmp;
            }

            tNear = t0 > tNear ? t0 : tNear;
            tFar = t1 < tFar ? t1 : tFar;

            if (tNear > tFar)
                return false;
        }

        return tFar >= 0.0f;
    }

    bool intersectsAABB(const AABB& box) const
    {
        float tNear, tFar;
        return intersectsAABB(box, tNear, tFar);
    }
};

} // namespace math
