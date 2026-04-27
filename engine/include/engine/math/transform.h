#pragma once

#include <engine/math/conventions.h>
#include <engine/math/vector.h>
#include <engine/math/matrix.h>
#include <engine/math/quaternion.h>

namespace math {

struct Transform
{
	Vector3 position;
	Quaternion rotation;
	Vector3 scale;

	constexpr Transform()
		: position(0.0f, 0.0f, 0.0f)
		, rotation()
		, scale(1.0f, 1.0f, 1.0f)
	{
	}

	constexpr Transform(const Vector3& position, const Quaternion& rotation, const Vector3& scale)
		: position(position), rotation(rotation), scale(scale)
	{
	}

	static constexpr Transform identity() { return Transform(); }

	Matrix4x4 toMatrix4x4() const
	{
		static_assert(conventions::kMatrixLayout == conventions::MatrixLayout::ColumnMajor);
		static_assert(conventions::kVectorConvention == conventions::VectorConvention::ColumnVector);

		// Column-vector convention: M = T * R * S
		Matrix4x4 m = rotation.toMatrix4x4(); // R

		// R * S (scale matrix columns)
		m[0][0] *= scale[0]; m[0][1] *= scale[1]; m[0][2] *= scale[2];
		m[1][0] *= scale[0]; m[1][1] *= scale[1]; m[1][2] *= scale[2];
		m[2][0] *= scale[0]; m[2][1] *= scale[1]; m[2][2] *= scale[2];

		// T * (R * S): translation in the last column
		m[0][3] = position[0];
		m[1][3] = position[1];
		m[2][3] = position[2];
		m[3][0] = 0.0f;
		m[3][1] = 0.0f;
		m[3][2] = 0.0f;
		m[3][3] = 1.0f;
		return m;
	}

	Vector3 transformPoint(const Vector3& p) const
	{
		Vector3 scaled(p[0] * scale[0], p[1] * scale[1], p[2] * scale[2]);
		return rotation.rotateVector(scaled) + position;
	}

	Vector3 transformDirection(const Vector3& d) const
	{
		Vector3 scaled(d[0] * scale[0], d[1] * scale[1], d[2] * scale[2]);
		return rotation.rotateVector(scaled);
	}

	Transform operator*(const Transform& rhs) const
	{
		Transform out;
		out.scale = Vector3(scale[0] * rhs.scale[0], scale[1] * rhs.scale[1], scale[2] * rhs.scale[2]);
		out.rotation = (rotation * rhs.rotation).normalized();
		Vector3 rhsPosScaled(rhs.position[0] * scale[0], rhs.position[1] * scale[1], rhs.position[2] * scale[2]);
		out.position = position + rotation.rotateVector(rhsPosScaled);
		return out;
	}
};

} // namespace math
