#pragma once

#include <cmath>
#include <cstdint>

namespace math {

namespace constants {
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTwoPi = kPi * 2.0f;
    constexpr float kHalfPi = kPi * 0.5f;
    constexpr float kEpsilon = 1e-6f;
}

template<typename T>
constexpr T min(T a, T b) { return a < b ? a : b; }

template<typename T>
constexpr T max(T a, T b) { return a > b ? a : b; }

template<typename T>
constexpr T clamp(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

template<typename T>
constexpr T abs(T v) { return v < T(0) ? -v : v; }

constexpr float radians(float degrees) { return degrees * (constants::kPi / 180.0f); }
constexpr float degrees(float radiansValue) { return radiansValue * (180.0f / constants::kPi); }

constexpr bool nearlyEqual(float a, float b, float epsilon = constants::kEpsilon)
{
    return abs(a - b) <= epsilon;
}

constexpr float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

constexpr float smoothstep(float edge0, float edge1, float x)
{
    float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}


constexpr Vector3 cross(const Vector3& x, const Vector3& y) {return x.cross(y); }


template<typename T>
constexpr float dot(const T& x, const T& y) { return x.dot(y); }


template<typename T>
constexpr T normalize(T x) { return x.normalize(); }

} // namespace math
