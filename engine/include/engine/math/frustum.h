#pragma once

#include <engine/math/conventions.h>
#include <engine/math/plane.h>
#include <engine/math/vector.h>
#include <engine/math/aabb.h>
#include <engine/math/sphere.h>
#include <engine/math/matrix.h>

namespace math {

struct Frustum
{
    enum PlaneIndex : uint32_t
    {
        Left = 0,
        Right,
        Bottom,
        Top,
        Near,
        Far,
        Count
    };

    Plane planes[Count];

    Frustum() = default;

    static Frustum fromViewProjection(const Matrix4x4& m)
    {
        static_assert(conventions::kMatrixLayout == conventions::MatrixLayout::ColumnMajor);
        static_assert(conventions::kVectorConvention == conventions::VectorConvention::ColumnVector);

        Frustum f;

        // Column-vector frustum extraction (using matrix rows via m[row][col])
        f.planes[Left]   = Plane(Vector3(m.m30 + m.m00, m.m31 + m.m01, m.m32 + m.m02), m.m33 + m.m03).normalized();
        f.planes[Right]  = Plane(Vector3(m.m30 - m.m00, m.m31 - m.m01, m.m32 - m.m02), m.m33 - m.m03).normalized();
        f.planes[Bottom] = Plane(Vector3(m.m30 + m.m10, m.m31 + m.m11, m.m32 + m.m12), m.m33 + m.m13).normalized();
        f.planes[Top]    = Plane(Vector3(m.m30 - m.m10, m.m31 - m.m11, m.m32 - m.m12), m.m33 - m.m13).normalized();
        f.planes[Near]   = Plane(Vector3(m.m30 + m.m20, m.m31 + m.m21, m.m32 + m.m22), m.m33 + m.m23).normalized();
        f.planes[Far]    = Plane(Vector3(m.m30 - m.m20, m.m31 - m.m21, m.m32 - m.m22), m.m33 - m.m23).normalized();

        return f;
    }

    bool containsPoint(const Vector3& p) const
    {
        for (uint32_t i = 0; i < Count; ++i)
        {
            if (planes[i].distanceToPoint(p) < 0.0f)
                return false;
        }
        return true;
    }

    bool intersectsSphere(const Sphere& s) const
    {
        for (uint32_t i = 0; i < Count; ++i)
        {
            if (planes[i].distanceToPoint(s.center) < -s.radius)
                return false;
        }
        return true;
    }

    bool intersectsAABB(const AABB& b) const
    {
        for (uint32_t i = 0; i < Count; ++i)
        {
            const Vector3& n = planes[i].normal;
            Vector3 p(
                n[0] >= 0.0f ? b.max[0] : b.min[0],
                n[1] >= 0.0f ? b.max[1] : b.min[1],
                n[2] >= 0.0f ? b.max[2] : b.min[2]
            );
            if (planes[i].distanceToPoint(p) < 0.0f)
                return false;
        }
        return true;
    }
};

static_assert(sizeof(Frustum) == sizeof(Plane) * Frustum::Count, "Frustum size mismatch");

} // namespace math
