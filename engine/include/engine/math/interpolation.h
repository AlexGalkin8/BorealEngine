#pragma once

#include <engine/math/vector.h>
#include <engine/math/quaternion.h>
#include <engine/math/math_utils.h>

#include <cmath>

namespace math {

namespace constants {
    constexpr float kSlerpDotThreshold = 0.9995f;
}

inline Quaternion nlerp(const Quaternion& a, const Quaternion& b, float t)
{
    Quaternion q = (a * (1.0f - t)) + (b * t);
    return q.normalized();
}

inline Quaternion slerp(Quaternion a, Quaternion b, float t)
{
    float cosTheta = a.dot(b);
    if (cosTheta < 0.0f)
    {
        b = b * -1.0f;
        cosTheta = -cosTheta;
    }

    if (cosTheta > constants::kSlerpDotThreshold)
        return nlerp(a, b, t);

    float theta = std::acos(clamp(cosTheta, -1.0f, 1.0f));
    float sinTheta = std::sin(theta);
    float w0 = std::sin((1.0f - t) * theta) / sinTheta;
    float w1 = std::sin(t * theta) / sinTheta;
    return (a * w0 + b * w1).normalized();
}

inline Vector3 bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, float t)
{
    float u = 1.0f - t;
    return p0 * (u * u) + p1 * (2.0f * u * t) + p2 * (t * t);
}

inline Vector3 bezier(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float t)
{
    float u = 1.0f - t;
    float uu = u * u;
    float tt = t * t;
    return p0 * (uu * u) + p1 * (3.0f * uu * t) + p2 * (3.0f * u * tt) + p3 * (tt * t);
}

} // namespace math
