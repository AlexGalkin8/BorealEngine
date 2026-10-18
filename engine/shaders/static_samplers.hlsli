#ifndef STATIC_SAMPLERS_HLSLI
#define STATIC_SAMPLERS_HLSLI

SamplerState gPointWrapSampler		: register(s0);
SamplerState gPointClampSampler		: register(s1);
SamplerState gLinearWrapSampler		: register(s2);
SamplerState gLinearClampSampler		: register(s3);
SamplerState gAnisotropicWrapSampler	: register(s4);
SamplerState gAnisotropicClampSampler	: register(s5);
SamplerComparisonState gShadowSampler	: register(s6);

#endif // STATIC_SAMPLERS_HLSLI
