#pragma once

#include <engine/math/conventions.h>
#include <engine/math/matrix.h>
#include <engine/math/vector.h>
#include <engine/math/math_utils.h>

namespace math {

// Builds a left-handed view matrix for column-vector convention: clip = P * V * M * position
inline Matrix4x4 lookAtLH(const Vector3& eye, const Vector3& target, const Vector3& up)
{
    static_assert(conventions::kMatrixLayout == conventions::MatrixLayout::ColumnMajor);
    static_assert(conventions::kVectorConvention == conventions::VectorConvention::ColumnVector);

    Vector3 zaxis = (target - eye).normalize();
    Vector3 xaxis(
        up[1] * zaxis[2] - up[2] * zaxis[1],
        up[2] * zaxis[0] - up[0] * zaxis[2],
        up[0] * zaxis[1] - up[1] * zaxis[0]);
    xaxis = xaxis.normalize();

    Vector3 yaxis(
        zaxis[1] * xaxis[2] - zaxis[2] * xaxis[1],
        zaxis[2] * xaxis[0] - zaxis[0] * xaxis[2],
        zaxis[0] * xaxis[1] - zaxis[1] * xaxis[0]);

    Matrix4x4 m = Matrix4x4::identity();

    // Row-major indexing [row][col], column-vector math
    m[0][0] = xaxis[0]; m[0][1] = xaxis[1]; m[0][2] = xaxis[2]; m[0][3] = -xaxis.dot(eye);
    m[1][0] = yaxis[0]; m[1][1] = yaxis[1]; m[1][2] = yaxis[2]; m[1][3] = -yaxis.dot(eye);
    m[2][0] = zaxis[0]; m[2][1] = zaxis[1]; m[2][2] = zaxis[2]; m[2][3] = -zaxis.dot(eye);
    m[3][0] = 0.0f;     m[3][1] = 0.0f;     m[3][2] = 0.0f;     m[3][3] = 1.0f;

    return m;
}


// D3D-style perspective (LH, z in [0,1])
inline Matrix4x4 perspectiveFovLH(float fovYRadians, float aspect, float zNear, float zFar)
{
    static_assert(conventions::kMatrixLayout == conventions::MatrixLayout::ColumnMajor);
    static_assert(conventions::kVectorConvention == conventions::VectorConvention::ColumnVector);

    Matrix4x4 m{};

    float yScale = 1.0f / std::tan(fovYRadians * 0.5f);
    float xScale = yScale / aspect;

    m[0][0] = xScale;
    m[1][1] = yScale;
    m[2][2] = zFar / (zFar - zNear);
    m[2][3] = 1.0f;
    m[3][2] = (-zNear * zFar) / (zFar - zNear);

    return m;
}

inline Matrix4x4 orthographicLH(float width, float height, float zNear, float zFar)
{
    static_assert(conventions::kMatrixLayout == conventions::MatrixLayout::ColumnMajor);
    static_assert(conventions::kVectorConvention == conventions::VectorConvention::ColumnVector);

    Matrix4x4 m = Matrix4x4::identity();
    m[0][0] = 2.0f / width;
    m[1][1] = 2.0f / height;
    m[2][2] = 1.0f / (zFar - zNear);
    m[2][3] = -zNear / (zFar - zNear);
    return m;
}

} // namespace math
