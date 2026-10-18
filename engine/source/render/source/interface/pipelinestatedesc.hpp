#pragma once

#include "interface/common.h"
#include "resourcelayoutstate.hpp"
#include "shader.hpp"

#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <algorithm>
#include <memory>
#include <optional>


namespace render {

inline constexpr uint32_t getVertexAttributeFormatSize(ETextureFormat format)
{
	switch (format)
	{
	case ETextureFormat::kRG8_Unorm:      return 2;
	case ETextureFormat::kRGBA8_Unorm:    return 4;
	case ETextureFormat::kRGBA8_Unorm_Srgb:return 4;
	case ETextureFormat::kRGBA8_Snorm:    return 4;
	case ETextureFormat::kRG16_Float:     return 4;
	case ETextureFormat::kRGBA16_Float:   return 8;
	case ETextureFormat::kRGBA16_SNorm:   return 8;
	case ETextureFormat::kRG32_Float:     return 8;
	case ETextureFormat::kRGB32_Float:    return 12;
	case ETextureFormat::kRGBA32_Float:   return 16;
	default:                              return 0;
	}
}

class IShader;


enum class EBlendMode
{
	kDisabled = 0,
	kAlpha,          // SrcAlpha, OneMinusSrcAlpha
	kPremultiplied,  // One, OneMinusSrcAlpha
	kAdditive,       // SrcAlpha/One, One
	kMultiply,       // SrcColor, DestColor
	kSubtract,       // Subtract
	kMin,            // Min
	kMax,            // Max
	kCustom
};


enum class EDepthFunc
{
	kNone = 0,
	kNever,
	kLess,
	kEqual,
	kLessEqual,
	kGreather,
	kNotEqual,
	kGreatherEqual,
	kAlways
};


enum class EFillMode
{
	kWireframe = 0,
	kSolid
};


enum class ECullMode
{
	kNone = 0,
	kFront,
	kBack
};


enum class ESemanticType
{
	kPosition = 0,
	kNormal,
	kTangent,
	kBitangent,
	kColor,
	kTexCoord
};


enum class EGraphicsShaderStage
{
	kVertex = 0,
	kPixel,
	kDomain,
	kHull,
	kGeometry
};
ENUM_CLASS_BITWISE_OPERATORS(EGraphicsShaderStage);


enum class EComputeShaderStage
{
	kCompute = 0,
};


enum class EMeshShaderStage
{
	kAmplification = 0,
	kMesh,
	kPixel,
};


inline const std::unordered_map<ESemanticType, std::vector<ETextureFormat>> kSupportedVertexAttributeFormats = {
	{ ESemanticType::kPosition,  { ETextureFormat::kRGBA16_Float,
								   ETextureFormat::kRG32_Float,
								   ETextureFormat::kRGB32_Float,
								   ETextureFormat::kRGBA32_Float } },

	{ ESemanticType::kNormal,    { ETextureFormat::kRGB32_Float,
								   ETextureFormat::kRGBA8_Snorm,
								   ETextureFormat::kRGBA16_SNorm,
								   ETextureFormat::kRG32_Float } },

	{ ESemanticType::kTangent,   { ETextureFormat::kRGBA32_Float,
								   ETextureFormat::kRGBA8_Snorm,
								   ETextureFormat::kRGBA16_SNorm,} },

	{ ESemanticType::kBitangent, { ETextureFormat::kRGBA32_Float,
								   ETextureFormat::kRGBA8_Snorm,
								   ETextureFormat::kRGBA16_SNorm,} },

	{ ESemanticType::kColor,     { ETextureFormat::kRGBA8_Unorm,
								   ETextureFormat::kRGBA8_Unorm_Srgb,
								   ETextureFormat::kRGBA16_Float,
								   ETextureFormat::kRGBA32_Float } },

	{ ESemanticType::kTexCoord, {  ETextureFormat::kRG32_Float,
								   ETextureFormat::kRG16_Float,
								   ETextureFormat::kRGB32_Float,
								   ETextureFormat::kRG8_Unorm } },
};


struct VertexAttribute
{
	ETextureFormat format;

	struct
	{
		ESemanticType type;
		uint32_t index = 0;
	} semantic;

	uint32_t input_slot = 0;
	uint32_t aligned_byte_offset = 0;
};


struct ShaderDefine
{
	std::wstring name;
	std::wstring value;
};


struct ShaderState
{
	std::string shader_name;
	std::vector<ShaderDefine> defines;
	EGraphicsShaderStage shader_stages = EGraphicsShaderStage::kVertex | EGraphicsShaderStage::kPixel;

	void addDefine(const std::wstring& name, const std::wstring& value = L"")
	{
		defines.push_back({ name, value });
	}
};


struct InputAssemblerState
{
	std::vector<VertexAttribute> vertex_attributes;
	EPrimitiveTopology topology = EPrimitiveTopology::kTriangle;

	void addAttribute(ESemanticType semantic_type, ETextureFormat format, uint32_t input_slot = 0)
	{
		VertexAttribute attribute = {};
		attribute.format = format;
		attribute.semantic.type = semantic_type;
		attribute.semantic.index = static_cast<uint32_t>(std::count_if(
			vertex_attributes.begin(), vertex_attributes.end(), [semantic_type](const VertexAttribute& attr)
		{
			return attr.semantic.type == semantic_type;
		}));

		attribute.input_slot = input_slot;

		uint32_t aligned_byte_offset = 0;
		for (const VertexAttribute& existing_attribute : vertex_attributes)
		{
			if (existing_attribute.input_slot == input_slot)
			{
				aligned_byte_offset += getVertexAttributeFormatSize(existing_attribute.format);
			}
		}
		attribute.aligned_byte_offset = aligned_byte_offset;

		vertex_attributes.push_back(attribute);
	}

};


struct RenderTargetDesc
{
	ETextureFormat format;
	EBlendMode blend;
};


struct DepthStencilDesc
{
	ETextureFormat format;

	bool depth_enable = true;
	bool depth_write = true;
	EDepthFunc depth_func = EDepthFunc::kLess;

	bool stencil_enable = false;
};


struct OutputMergerState
{
	bool independent_blend_enable = false;
	std::array<RenderTargetDesc, kMaxRenderTargets> render_targets;
	uint32_t render_target_count = 0;
	DepthStencilDesc depth_stencil;

	void addRenderTarget(ETextureFormat format, EBlendMode blend = EBlendMode::kDisabled)
	{
		ASSERT_MSG_RETURN_VOID(render_target_count < kMaxRenderTargets, "Added max render targets");
		render_targets.at(render_target_count) = { format, blend };
		render_target_count++;
	}
};


struct RasterizerState
{
	EFillMode fill_mode = EFillMode::kSolid;
	ECullMode cull_mode = ECullMode::kNone;
	bool depth_clip_enable = true;
};


struct GraphicsPipelineDesc
{
	ShaderState shader_state;
	InputAssemblerState input_layout_state;

	RasterizerState rasterizer_state;
	OutputMergerState output_merger_state;

	ResourceLayoutState resource_layout_state;
};


struct ComputePipelineDesc
{
	ShaderState shader_state;
};

		
struct MeshPipelineDesc
{
	ShaderState shader_state;
};


struct GraphicsCompiledShaders
{
	std::unique_ptr<IShader> vertex_shader;
	std::unique_ptr<IShader> pixel_shader;
	std::unique_ptr<IShader> hull_shader;
	std::unique_ptr<IShader> domain_shader;
	std::unique_ptr<IShader> geometry_shader;
};


struct ComputeCompiledShaders
{
	std::unique_ptr<IShader> compute_shader;
};


struct MeshCompiledShaders
{
	std::unique_ptr<IShader> amplification_shader;
	std::unique_ptr<IShader> mesh_shader;
	std::unique_ptr<IShader> pixel_shader;
};

}  // namespace render
