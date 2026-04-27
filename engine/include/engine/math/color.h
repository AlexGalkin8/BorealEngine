#pragma once

#include <engine/math/vector.h>
#include <engine/math/math_utils.h>

#include <cstdint>
#include <cmath>


namespace math {

namespace constants {
    constexpr float kSRGBLinearThreshold = 0.0031308f;
    constexpr float kSRGBGammaThreshold = 0.04045f;
    constexpr float kSRGBLinearScale = 12.92f;
    constexpr float kSRGBA = 1.055f;
    constexpr float kSRGBB = 0.055f;
    constexpr float kSRGBGamma = 2.4f;
    constexpr float kInvSRGBGamma = 1.0f / kSRGBGamma;
}

// ============================================================================
//  Color - RGBA color, float channels in [0, 1]
//  Built on top of Vector4 (r, g, b, a)
// ============================================================================

struct Color
{
    Vector4 rgba;

    // Constructors
    constexpr Color()                                            : rgba(0.0f, 0.0f, 0.0f, 1.0f) {}
    constexpr Color(float r, float g, float b, float a = 1.0f)   : rgba(r, g, b, a) {}
    constexpr explicit Color(const Vector4& v)                   : rgba(v) {}
    constexpr explicit Color(const Vector3& rgb, float a = 1.0f) : rgba(rgb[0], rgb[1], rgb[2], a) {}

    // Channel accessors
    constexpr float  r() const { return rgba[0]; }
    constexpr float  g() const { return rgba[1]; }
    constexpr float  b() const { return rgba[2]; }
    constexpr float  a() const { return rgba[3]; }
    constexpr float& r()       { return rgba[0]; }
    constexpr float& g()       { return rgba[1]; }
    constexpr float& b()       { return rgba[2]; }
    constexpr float& a()       { return rgba[3]; }

    // Conversions
    constexpr Vector4 toVector4() const { return rgba; }
    constexpr Vector3 toVector3() const { return Vector3(rgba[0], rgba[1], rgba[2]); }

    // Pack to 32-bit 0xRRGGBBAA
    constexpr uint32_t toRGBA8() const
    {
        auto pack = [](float v) constexpr -> uint32_t {
            return static_cast<uint32_t>(v <= 0.0f ? 0u : v >= 1.0f ? 255u : static_cast<uint32_t>(v * 255.0f + 0.5f));
        };
        return (pack(rgba[0]) << 24) | (pack(rgba[1]) << 16) | (pack(rgba[2]) << 8) | pack(rgba[3]);
    }

    // Unpack from 32-bit 0xRRGGBBAA
    static constexpr Color fromRGBA8(uint32_t value)
    {
        constexpr float inv = 1.0f / 255.0f;
        return Color(
            static_cast<float>((value >> 24) & 0xFF) * inv,
            static_cast<float>((value >> 16) & 0xFF) * inv,
            static_cast<float>((value >>  8) & 0xFF) * inv,
            static_cast<float>((value >>  0) & 0xFF) * inv
        );
    }

    // Arithmetic operators — delegate to Vector4
    constexpr Color operator+(const Color& c) const { return Color(rgba + c.rgba); }
    constexpr Color operator-(const Color& c) const { return Color(rgba - c.rgba); }
    constexpr Color operator*(float s)        const { return Color(rgba * s); }
    constexpr Color operator/(float s)        const { return Color(rgba / s); }

    // Component-wise multiply (tinting)
    constexpr Color operator*(const Color& c) const
    {
        return Color(rgba[0] * c.rgba[0], rgba[1] * c.rgba[1], rgba[2] * c.rgba[2], rgba[3] * c.rgba[3]);
    }

    constexpr Color& operator+=(const Color& c) { rgba += c.rgba; return *this; }
    constexpr Color& operator-=(const Color& c) { rgba -= c.rgba; return *this; }
    constexpr Color& operator*=(float s)        { rgba *= s;      return *this; }
    constexpr Color& operator/=(float s)        { rgba /= s;      return *this; }

    constexpr bool operator==(const Color& c) const { return rgba == c.rgba; }
    constexpr bool operator!=(const Color& c) const { return rgba != c.rgba; }

    // Clamp all channels to [0, 1]
    constexpr Color saturate() const
    {
        return Color(
            Vector4::min(Vector4::max(rgba, Vector4(0.0f, 0.0f, 0.0f, 0.0f)),
                                      Vector4(1.0f, 1.0f, 1.0f, 1.0f))
        );
    }

    // Linear interpolation
    static constexpr Color lerp(const Color& a, const Color& b, float t)
    {
        return Color(Vector4::lerp(a.rgba, b.rgba, t));
    }

    // Linear -> sRGB
    Color toSRGB() const
    {
        auto encode = [](float v) {
            return v <= constants::kSRGBLinearThreshold
                ? v * constants::kSRGBLinearScale
                : constants::kSRGBA * std::pow(v, constants::kInvSRGBGamma) - constants::kSRGBB;
        };
        return Color(encode(rgba[0]), encode(rgba[1]), encode(rgba[2]), rgba[3]);
    }

    // sRGB -> linear
    Color toLinear() const
    {
        auto decode = [](float v) {
            return v <= constants::kSRGBGammaThreshold
                ? v / constants::kSRGBLinearScale
                : std::pow((v + constants::kSRGBB) / constants::kSRGBA, constants::kSRGBGamma);
        };
        return Color(decode(rgba[0]), decode(rgba[1]), decode(rgba[2]), rgba[3]);
    }

    // ============================================================================
    //  Predefined colors
    // ============================================================================

    static constexpr Color black()       { return {0.0f, 0.0f, 0.0f}; }
    static constexpr Color white()       { return {1.0f, 1.0f, 1.0f}; }
    static constexpr Color red()         { return {1.0f, 0.0f, 0.0f}; }
    static constexpr Color green()       { return {0.0f, 1.0f, 0.0f}; }
    static constexpr Color blue()        { return {0.0f, 0.0f, 1.0f}; }
    static constexpr Color yellow()      { return {1.0f, 1.0f, 0.0f}; }
    static constexpr Color cyan()        { return {0.0f, 1.0f, 1.0f}; }
    static constexpr Color magenta()     { return {1.0f, 0.0f, 1.0f}; }
    static constexpr Color transparent() { return {0.0f, 0.0f, 0.0f, 0.0f}; }
};

constexpr Color operator*(float s, const Color& c) { return c * s; }

} // namespace math
