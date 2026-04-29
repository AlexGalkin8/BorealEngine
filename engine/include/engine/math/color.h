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
    float r;
    float g;
    float b;
    float a;

    // Constructors
    constexpr Color()
        : r(0.0f), g(0.0f), b(0.0f), a(1.0f) {}

    constexpr Color(float r, float g, float b, float a = 1.0f)
        : r(r), g(g), b(b), a(a) {}

    constexpr explicit Color(const Vector4& v)
        : r(v.x), g(v.y), b(v.z), a(v.w) {}

    constexpr explicit Color(const Vector3& rgb, float a = 1.0f)
        : r(rgb.x), g(rgb.y), b(rgb.z), a(a) {}

    constexpr Color(const Color&) = default;
    constexpr Color(Color&&) noexcept = default;
    constexpr Color& operator=(const Color&) = default;
    constexpr Color& operator=(Color&&) noexcept = default;

    // Conversions
    constexpr Vector4 toVector4() const { return Vector4(r, g, b, a); }
    constexpr Vector3 toVector3() const { return Vector3(r, g, b); }

    // Pack to 32-bit 0xRRGGBBAA
    constexpr uint32_t toRGBA8() const
    {
        auto pack = [](float v) constexpr -> uint32_t {
            return static_cast<uint32_t>(v <= 0.0f ? 0u : v >= 1.0f ? 255u : static_cast<uint32_t>(v * 255.0f + 0.5f));
        };
        return (pack(r) << 24) | (pack(g) << 16) | (pack(b) << 8) | pack(a);
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

    // Arithmetic operators
    constexpr Color operator+(const Color& c) const { return Color(r + c.r, g + c.g, b + c.b, a + c.a); }
    constexpr Color operator-(const Color& c) const { return Color(r - c.r, g - c.g, b - c.b, a - c.a); }
    constexpr Color operator*(float s)        const { return Color(r * s, g * s, b * s, a * s); }
    constexpr Color operator/(float s)        const { return Color(r / s, g / s, b / s, a / s); }

    // Component-wise multiply (tinting)
    constexpr Color operator*(const Color& c) const
    {
        return Color(r * c.r, g * c.g, b * c.b, a * c.a);
    }

    constexpr Color& operator+=(const Color& c) { r += c.r; g += c.g; b += c.b; a += c.a; return *this; }
    constexpr Color& operator-=(const Color& c) { r -= c.r; g -= c.g; b -= c.b; a -= c.a; return *this; }
    constexpr Color& operator*=(float s)        { r *= s; g *= s; b *= s; a *= s; return *this; }
    constexpr Color& operator/=(float s)        { r /= s; g /= s; b /= s; a /= s; return *this; }

    constexpr bool operator==(const Color& c) const { return r == c.r && g == c.g && b == c.b && a == c.a; }
    constexpr bool operator!=(const Color& c) const { return !(*this == c); }

    // Clamp all channels to [0, 1]
    constexpr Color saturate() const
    {
        auto clamp01 = [](float v) constexpr {
            return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
        };
        return Color(clamp01(r), clamp01(g), clamp01(b), clamp01(a));
    }

    // Linear interpolation
    static constexpr Color lerp(const Color& aColor, const Color& bColor, float t)
    {
        return Color(
            aColor.r + (bColor.r - aColor.r) * t,
            aColor.g + (bColor.g - aColor.g) * t,
            aColor.b + (bColor.b - aColor.b) * t,
            aColor.a + (bColor.a - aColor.a) * t
        );
    }

    // Linear -> sRGB
    Color toSRGB() const
    {
        auto encode = [](float v) {
            return v <= constants::kSRGBLinearThreshold
                ? v * constants::kSRGBLinearScale
                : constants::kSRGBA * std::pow(v, constants::kInvSRGBGamma) - constants::kSRGBB;
        };
        return Color(encode(r), encode(g), encode(b), a);
    }

    // sRGB -> linear
    Color toLinear() const
    {
        auto decode = [](float v) {
            return v <= constants::kSRGBGammaThreshold
                ? v / constants::kSRGBLinearScale
                : std::pow((v + constants::kSRGBB) / constants::kSRGBA, constants::kSRGBGamma);
        };
        return Color(decode(r), decode(g), decode(b), a);
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

static_assert(sizeof(Color) == sizeof(float) * 4, "Color size mismatch");

} // namespace math
