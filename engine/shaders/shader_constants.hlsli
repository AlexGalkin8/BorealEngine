#ifndef BOREAL_SHADER_CONSTANTS_HLSLI
#define BOREAL_SHADER_CONSTANTS_HLSLI

static const float kPi = 3.14159265358979323846;

static const float kMinHalfVectorLengthSquared = 1e-8;
static const float kMinViewVectorLengthSquared = 1e-20;
static const float kMinGeometricNormalLengthSquared = 1e-30;

static const float kSrgbLinearThreshold = 0.0031308;
static const float kSrgbLinearScale = 12.92;
static const float kSrgbPowerScale = 1.055;
static const float kSrgbPowerOffset = 0.055;
static const float kSrgbExponent = 1.0 / 2.4;

#endif
