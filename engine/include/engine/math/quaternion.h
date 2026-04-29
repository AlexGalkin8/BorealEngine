#pragma once

#include <engine/core/utility/assert.h>
#include <engine/math/vector.h>
#include <engine/math/matrix.h>

#include <cmath>


namespace math {

struct Quaternion
{
	float x;
	float y;
	float z;
	float w;

	constexpr Quaternion() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
	constexpr Quaternion(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	static constexpr Quaternion identity() { return Quaternion(0.0f, 0.0f, 0.0f, 1.0f); }

	constexpr float lengthSq() const { return x * x + y * y + z * z + w * w; }
	float length() const { return std::sqrt(lengthSq()); }

	Quaternion normalized() const
	{
		float len = length();
		ASSERT_MSG(len > 0.0f, "Quaternion length must be > 0");
		return *this / len;
	}

	void normalize()
	{
		float len = length();
		ASSERT_MSG(len > 0.0f, "Quaternion length must be > 0");
		x /= len;
		y /= len;
		z /= len;
		w /= len;
	}

	constexpr Quaternion conjugate() const { return Quaternion(-x, -y, -z, w); }

	Quaternion inverse() const
	{
		float lenSq = lengthSq();
		ASSERT_MSG(lenSq > 0.0f, "Quaternion lengthSq must be > 0");
		return conjugate() / lenSq;
	}

	constexpr Quaternion operator+(const Quaternion& q) const { return Quaternion(x + q.x, y + q.y, z + q.z, w + q.w); }
	constexpr Quaternion operator-(const Quaternion& q) const { return Quaternion(x - q.x, y - q.y, z - q.z, w - q.w); }
	constexpr Quaternion operator*(float s) const { return Quaternion(x * s, y * s, z * s, w * s); }
	constexpr Quaternion operator/(float s) const { return Quaternion(x / s, y / s, z / s, w / s); }

	constexpr Quaternion operator*(const Quaternion& q) const
	{
		return Quaternion(
			w * q.x + x * q.w + y * q.z - z * q.y,
			w * q.y - x * q.z + y * q.w + z * q.x,
			w * q.z + x * q.y - y * q.x + z * q.w,
			w * q.w - x * q.x - y * q.y - z * q.z
		);
	}

	constexpr Quaternion& operator+=(const Quaternion& q) { *this = *this + q; return *this; }
	constexpr Quaternion& operator-=(const Quaternion& q) { *this = *this - q; return *this; }
	constexpr Quaternion& operator*=(float s) { *this = *this * s; return *this; }
	constexpr Quaternion& operator/=(float s) { *this = *this / s; return *this; }
	constexpr Quaternion& operator*=(const Quaternion& q) { *this = *this * q; return *this; }

	constexpr bool operator==(const Quaternion& q) const { return x == q.x && y == q.y && z == q.z && w == q.w; }
	constexpr bool operator!=(const Quaternion& q) const { return !(*this == q); }

	constexpr float dot(const Quaternion& q) const { return x * q.x + y * q.y + z * q.z + w * q.w; }

	static Quaternion fromAxisAngle(const Vector3& axis, float radians)
	{
		float lenSq = axis.dot(axis);
		ASSERT_MSG(lenSq > 0.0f, "Axis lengthSq must be > 0");
		float invLen = 1.0f / std::sqrt(lenSq);
		float half = radians * 0.5f;
		float s = std::sin(half);
		float c = std::cos(half);
	 return Quaternion(axis.x * invLen * s, axis.y * invLen * s, axis.z * invLen * s, c);
	}

	// Rotation order: X (pitch) then Y (yaw) then Z (roll)
	static Quaternion fromEuler(float pitch, float yaw, float roll)
	{
		float hx = pitch * 0.5f;
		float hy = yaw * 0.5f;
		float hz = roll * 0.5f;

		float sx = std::sin(hx), cx = std::cos(hx);
		float sy = std::sin(hy), cy = std::cos(hy);
		float sz = std::sin(hz), cz = std::cos(hz);

		Quaternion qx(sx, 0.0f, 0.0f, cx);
		Quaternion qy(0.0f, sy, 0.0f, cy);
		Quaternion qz(0.0f, 0.0f, sz, cz);
		return (qz * qy * qx).normalized();
	}

	static Vector3 cross(const Vector3& a, const Vector3& b)
	{
		return Vector3(
			a.y * b.z - a.z * b.y,
			a.z * b.x - a.x * b.z,
			a.x * b.y - a.y * b.x
		);
	}

	Vector3 rotateVector(const Vector3& v) const
	{
		Vector3 u(x, y, z);
		float s = w;
		return (u * (2.0f * u.dot(v))) + (v * (s * s - u.dot(u))) + (cross(u, v) * (2.0f * s));
	}

	Matrix4x4 toMatrix4x4() const
	{
		Quaternion q = normalized();
		float xx = q.x * q.x;
		float yy = q.y * q.y;
		float zz = q.z * q.z;
		float xy = q.x * q.y;
		float xz = q.x * q.z;
		float yz = q.y * q.z;
		float wx = q.w * q.x;
		float wy = q.w * q.y;
		float wz = q.w * q.z;

		Matrix4x4 rotationMatrix = {
			1.0f - 2.0f * (yy + zz), 2.0f * (xy - wz),        2.0f * (xz + wy),        0.0f,
			2.0f * (xy + wz),        1.0f - 2.0f * (xx + zz), 2.0f * (yz - wx),        0.0f,
			2.0f * (xz - wy),        2.0f * (yz + wx),        1.0f - 2.0f * (xx + yy), 0.0f,
			0.0f,                    0.0f,                    0.0f,                    1.0f
		};

		return rotationMatrix;
	}
};

constexpr Quaternion operator*(float s, const Quaternion& q) { return q * s; }

static_assert(sizeof(Quaternion) == sizeof(float) * 4, "Quaternion size mismatch");

} // namespace math