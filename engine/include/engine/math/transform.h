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
		Matrix4x4 rotationMatrix = rotation.toMatrix4x4();

		Matrix4x4 scaleMatrix = {
			scale.x, 0.0f,   0.0f,   0.0f,
			0.0f,   scale.y, 0.0f,   0.0f,
			0.0f,   0.0f,   scale.z, 0.0f,
			0.0f,   0.0f,   0.0f,   1.0f
		};

		Matrix4x4 translationMatrix = {
			1.0f, 0.0f, 0.0f, position.x,
			0.0f, 1.0f, 0.0f, position.y,
			0.0f, 0.0f, 1.0f, position.z,
			0.0f, 0.0f, 0.0f, 1.0f
		};

		return translationMatrix * rotationMatrix * scaleMatrix;
	}

	Vector3 transformPoint(const Vector3& p) const
	{
	  Vector3 scaled(p.x * scale.x, p.y * scale.y, p.z * scale.z);
		return rotation.rotateVector(scaled) + position;
	}

	Vector3 transformDirection(const Vector3& d) const
	{
		Vector3 scaled(d.x * scale.x, d.y * scale.y, d.z * scale.z);
		return rotation.rotateVector(scaled);
	}

	Transform operator*(const Transform& rhs) const
	{
		Transform out;
		out.scale = Vector3(scale.x * rhs.scale.x, scale.y * rhs.scale.y, scale.z * rhs.scale.z);
		out.rotation = (rotation * rhs.rotation).normalized();
		Vector3 rhsPosScaled(rhs.position.x * scale.x, rhs.position.y * scale.y, rhs.position.z * scale.z);
		out.position = position + rotation.rotateVector(rhsPosScaled);
		return out;
	}
};

static_assert(sizeof(Transform) == sizeof(Vector3) + sizeof(Quaternion) + sizeof(Vector3), "Transform size mismatch");
static_assert(std::is_standard_layout_v<Transform>, "Transform must be standard layout");

} // namespace math
