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
        f.planes[Left]   = Plane(Vector3(m[3][0] + m[0][0], m[3][1] + m[0][1], m[3][2] + m[0][2]), m[3][3] + m[0][3]).normalized();
        f.planes[Right]  = Plane(Vector3(m[3][0] - m[0][0], m[3][1] - m[0][1], m[3][2] - m[0][2]), m[3][3] - m[0][3]).normalized();
        f.planes[Bottom] = Plane(Vector3(m[3][0] + m[1][0], m[3][1] + m[1][1], m[3][2] + m[1][2]), m[3][3] + m[1][3]).normalized();
        f.planes[Top]    = Plane(Vector3(m[3][0] - m[1][0], m[3][1] - m[1][1], m[3][2] - m[1][2]), m[3][3] - m[1][3]).normalized();
        f.planes[Near]   = Plane(Vector3(m[3][0] + m[2][0], m[3][1] + m[2][1], m[3][2] + m[2][2]), m[3][3] + m[2][3]).normalized();
        f.planes[Far]    = Plane(Vector3(m[3][0] - m[2][0], m[3][1] - m[2][1], m[3][2] - m[2][2]), m[3][3] - m[2][3]).normalized();

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

} // namespace math
