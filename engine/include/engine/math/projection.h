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

	Vector3 f = normalize(target - eye);
	Vector3 r = normalize(cross(up, f));
	Vector3 u = cross(f, r);

	Matrix4x4 m = {
		r.x, r.y, r.z, -dot(r, eye),
		u.x, u.y, u.z, -dot(u, eye),
		f.x, f.y, f.z, -dot(f, eye),
		0.0f, 0.0f, 0.0f, 1.0f
	};

	return m;
}


// D3D-style perspective (LH, z in [0,1])
inline Matrix4x4 perspectiveFovLH(float fovYRadians, float aspect, float zNear, float zFar)
{
	static_assert(conventions::kMatrixLayout == conventions::MatrixLayout::ColumnMajor);
	static_assert(conventions::kVectorConvention == conventions::VectorConvention::ColumnVector);

	float yScale = 1.0f / std::tan(fovYRadians * 0.5f);
	float xScale = yScale / aspect;
	float zTransform = zFar / (zFar - zNear);
	float zTranslate = -zNear * zTransform;

	Matrix4x4 m = {
		xScale,     0.0f,   0.0f,        0.0f,
		0.0f,       yScale, 0.0f,        0.0f,
		0.0f,       0.0f,   zTransform,  zTranslate,
		0.0f,       0.0f,   1.0f,        0.0f
	};

	return m;
}

inline Matrix4x4 orthographicLH(float width, float height, float zNear, float zFar)
{
	static_assert(conventions::kMatrixLayout == conventions::MatrixLayout::ColumnMajor);
	static_assert(conventions::kVectorConvention == conventions::VectorConvention::ColumnVector);

	Matrix4x4 m = Matrix4x4::identity();
	m.m00 = 2.0f / width;
	m.m11 = 2.0f / height;
	m.m22 = 1.0f / (zFar - zNear);
	m.m23 = -zNear / (zFar - zNear);
	return m;
}

} // namespace math
