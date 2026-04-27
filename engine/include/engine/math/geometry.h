#pragma once

#include <engine/math/vector.h>
#include <engine/math/ray.h>
#include <engine/math/plane.h>
#include <engine/math/sphere.h>
#include <engine/math/aabb.h>

namespace math {

inline bool intersects(const AABB& a, const AABB& b)
{
    return a.intersects(b);
}

inline bool intersects(const Sphere& a, const Sphere& b)
{
    return a.intersects(b);
}

inline bool intersects(const Sphere& s, const AABB& b)
{
    return s.intersects(b);
}

inline bool intersects(const AABB& b, const Sphere& s)
{
    return s.intersects(b);
}

inline bool intersects(const Ray& r, const Plane& p, float& t)
{
    return r.intersectsPlane(p, t);
}

inline bool intersects(const Ray& r, const Sphere& s, float& tNear, float& tFar)
{
    return r.intersectsSphere(s, tNear, tFar);
}

inline bool intersects(const Ray& r, const AABB& b, float& tNear, float& tFar)
{
    return r.intersectsAABB(b, tNear, tFar);
}

} // namespace math
