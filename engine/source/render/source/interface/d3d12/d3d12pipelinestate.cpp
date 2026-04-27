#include "d3d12pipelinestate.hpp"
#include "d3d12common.hpp"
#include "interface/devicecontext.h"
#include "interface/shader.hpp"

#include <array>
#include <vector>
#include <algorithm>
#include <iterator>


namespace render {

static constexpr D3D12_FILL_MODE translateFillModeToD3D12FillMode(EFillMode fill_mode)
{
	switch (fill_mode)
	{
		case EFillMode::kWireframe: return D3D12_FILL_MODE_WIREFRAME;
		case EFillMode::kSolid:     return D3D12_FILL_MODE_SOLID;
		default:                   return D3D12_FILL_MODE_SOLID;
	}
}


static constexpr D3D12_CULL_MODE translateCullModeToD3D12CullMode(ECullMode cull_mode)
{
	switch (cull_mode)
	{
		case ECullMode::kNone: return D3D12_CULL_MODE_NONE;
		case ECullMode::kFront:return D3D12_CULL_MODE_FRONT;
		case ECullMode::kBack: return D3D12_CULL_MODE_BACK;
		default:               return D3D12_CULL_MODE_NONE;
	}
}


static constexpr D3D12_COMPARISON_FUNC translateDepthFuncToD3D12ComparisonFunc(EDepthFunc depth_func)
{
	switch (depth_func)
	{
		case EDepthFunc::kNone:           return D3D12_COMPARISON_FUNC_NEVER;
		case EDepthFunc::kNever:          return D3D12_COMPARISON_FUNC_NEVER;
		case EDepthFunc::kLess:           return D3D12_COMPARISON_FUNC_LESS;
		case EDepthFunc::kEqual:          return D3D12_COMPARISON_FUNC_EQUAL;
		case EDepthFunc::kLessEqual:      return D3D12_COMPARISON_FUNC_LESS_EQUAL;
		case EDepthFunc::kGreather:       return D3D12_COMPARISON_FUNC_GREATER;
		case EDepthFunc::kNotEqual:       return D3D12_COMPARISON_FUNC_NOT_EQUAL;
		case EDepthFunc::kGreatherEqual:  return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
		case EDepthFunc::kAlways:         return D3D12_COMPARISON_FUNC_ALWAYS;
		default:                          return D3D12_COMPARISON_FUNC_LESS;
	}
}


static constexpr D3D12_PRIMITIVE_TOPOLOGY_TYPE translatePrimitiveTopologyToD3D12PrimitiveTopologyType(EPrimitiveTopology topology)
{
	switch (topology)
	{
		case EPrimitiveTopology::kPoint:    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		case EPrimitiveTopology::kLine:     return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		case EPrimitiveTopology::kTriangle: return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		default:                            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}
}


static constexpr D3D12_INPUT_ELEMENT_DESC translateVertexAttributeToD3D12InputElementDesc(const VertexAttribute& attribute)
{
	D3D12_INPUT_ELEMENT_DESC desc = {};
	desc.SemanticName = "";  // This will be set based on the semantic type
	desc.SemanticIndex = attribute.semantic.index;
	desc.Format = internal::translateTextureFormatToDxgiFormat(attribute.format);
	desc.InputSlot = attribute.input_slot;
	desc.AlignedByteOffset = attribute.aligned_byte_offset;
	desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	desc.InstanceDataStepRate = 0;

	// Set SemanticName based on semantic type
	switch (attribute.semantic.type)
	{
		case ESemanticType::kPosition: desc.SemanticName = "POSITION";  break;
		case ESemanticType::kNormal:   desc.SemanticName = "NORMAL";    break;
		case ESemanticType::kTangent:  desc.SemanticName = "TANGENT";   break;
		case ESemanticType::kBitangent:desc.SemanticName = "BITANGENT"; break;
		case ESemanticType::kColor:    desc.SemanticName = "COLOR";     break;
		case ESemanticType::kTexCoord: desc.SemanticName = "TEXCOORD";  break;
		default:                       desc.SemanticName = "";          break;
	}
	return desc;
}


static D3D12_INPUT_LAYOUT_DESC translateInputLayoutStateToD3D12InputLayoutDesc(
	std::vector<D3D12_INPUT_ELEMENT_DESC>& d3d12_input_elements, const InputAssemblerState& input_layout_state)
{
	std::transform(input_layout_state.vertex_attributes.begin(), input_layout_state.vertex_attributes.end(),
		std::back_inserter(d3d12_input_elements), [](const VertexAttribute& attribute)
	{
		return translateVertexAttributeToD3D12InputElementDesc(attribute);
	});
	D3D12_INPUT_LAYOUT_DESC d3d12_desc = {};
	d3d12_desc.NumElements = static_cast<UINT>(d3d12_input_elements.size());
	d3d12_desc.pInputElementDescs = d3d12_input_elements.data();
	return d3d12_desc;
}


static constexpr D3D12_RENDER_TARGET_BLEND_DESC translateBlendModeToD3D12RenderTargetBlendDesc(EBlendMode blend_mode)
{
	D3D12_RENDER_TARGET_BLEND_DESC desc = {};
	switch (blend_mode)
	{
		case EBlendMode::kDisabled:
			desc.BlendEnable = FALSE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_ZERO;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		case EBlendMode::kAlpha:
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		case EBlendMode::kPremultiplied:
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		case EBlendMode::kAdditive:
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_ONE;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		case EBlendMode::kMultiply:
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_DEST_COLOR;
			desc.DestBlend = D3D12_BLEND_ZERO;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_DEST_ALPHA;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		case EBlendMode::kSubtract:
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_ONE;
			desc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		case EBlendMode::kMin:
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_ONE;
			desc.BlendOp = D3D12_BLEND_OP_MIN;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ONE;
			desc.BlendOpAlpha = D3D12_BLEND_OP_MIN;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		case EBlendMode::kMax:
			desc.BlendEnable = TRUE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_ONE;
			desc.BlendOp = D3D12_BLEND_OP_MAX;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ONE;
			desc.BlendOpAlpha = D3D12_BLEND_OP_MAX;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
		default:
			// Default to disabled blending
			desc.BlendEnable = FALSE;
			desc.LogicOpEnable = FALSE;
			desc.SrcBlend = D3D12_BLEND_ONE;
			desc.DestBlend = D3D12_BLEND_ZERO;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.LogicOp = D3D12_LOGIC_OP_NOOP;
			desc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
			break;
	}
	return desc;
}


static D3D12_BLEND_DESC translateOutputMergerStateToD3D12BlendDesc(const OutputMergerState& output_merger_state)
{
	D3D12_BLEND_DESC blend_desc = {};
	blend_desc.IndependentBlendEnable = output_merger_state.independent_blend_enable;
	for (uint32_t i = 0; i < output_merger_state.render_target_count; ++i)
	{
		const RenderTargetDesc& rt_desc = output_merger_state.render_targets[i];
		blend_desc.RenderTarget[i] = translateBlendModeToD3D12RenderTargetBlendDesc(rt_desc.blend);
	}
	return blend_desc;
}


static constexpr D3D12_RASTERIZER_DESC translateRasterizerStateToD3D12RasterizerDesc(const RasterizerState& rasterizer_state)
{
	D3D12_RASTERIZER_DESC rasterizer_desc = {};
	rasterizer_desc.FillMode = translateFillModeToD3D12FillMode(rasterizer_state.fill_mode);
	rasterizer_desc.CullMode = translateCullModeToD3D12CullMode(rasterizer_state.cull_mode);
	rasterizer_desc.DepthClipEnable = rasterizer_state.depth_clip_enable ? TRUE : FALSE;
	rasterizer_desc.FrontCounterClockwise = FALSE;  // Assuming clockwise is front
	rasterizer_desc.DepthBias = 0;
	rasterizer_desc.DepthBiasClamp = 0.0f;
	rasterizer_desc.SlopeScaledDepthBias = 0.0f;
	rasterizer_desc.MultisampleEnable = FALSE;
	rasterizer_desc.AntialiasedLineEnable = FALSE;
	rasterizer_desc.ForcedSampleCount = 0;
	rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	return rasterizer_desc;
}


static constexpr D3D12_DEPTH_STENCIL_DESC translateDepthStencilDescToD3D12DepthStencilDesc(const DepthStencilDesc& depth_stencil_desc)
{
	D3D12_DEPTH_STENCIL_DESC d3d12_desc = {};
	d3d12_desc.DepthEnable = depth_stencil_desc.depth_enable ? TRUE : FALSE;
	d3d12_desc.DepthWriteMask = depth_stencil_desc.depth_write ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	d3d12_desc.DepthFunc = translateDepthFuncToD3D12ComparisonFunc(depth_stencil_desc.depth_func);
	d3d12_desc.StencilEnable = depth_stencil_desc.stencil_enable ? TRUE : FALSE;
	d3d12_desc.StencilReadMask = 0xFF;  // Default read mask
	d3d12_desc.StencilWriteMask = 0xFF; // Default write mask
	// FrontFace and BackFace can be set to default since stencil is disabled by default
	d3d12_desc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3d12_desc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3d12_desc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3d12_desc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3d12_desc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3d12_desc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3d12_desc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3d12_desc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	return d3d12_desc;
}


static constexpr D3D12_DESCRIPTOR_RANGE translateDescriptorRangeToD3D12DescriptorRange(const DescriptorRange& range)
{
	D3D12_DESCRIPTOR_RANGE d3d12_range = {};
	switch (range.descriptors_type)
	{
		case DescriptorType::kCBV: d3d12_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; break;
		case DescriptorType::kSRV: d3d12_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; break;
		case DescriptorType::kUAV: d3d12_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV; break;
		default:                   d3d12_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV; break;
	}
	d3d12_range.NumDescriptors = range.num_descriptors;
	d3d12_range.BaseShaderRegister = range.base_shader_register;
	d3d12_range.RegisterSpace = 0;
	d3d12_range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	return d3d12_range;
}


static D3D12_ROOT_PARAMETER translateRootParameterToD3D12RootParameter(
	const RootParameter& root_parameter,
	std::array<D3D12_DESCRIPTOR_RANGE, kMaxDescriptorRangesPerTable>& d3d12_ranges)
{
	D3D12_ROOT_PARAMETER d3d12_param = {};
	if (auto* table = std::get_if<RootParameter::DescriptorTable>(&root_parameter.getVariant()))
	{
		for (uint32_t r = 0; r < table->num_ranges; ++r)
			d3d12_ranges[r] = translateDescriptorRangeToD3D12DescriptorRange(table->ranges[r]);
		d3d12_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		d3d12_param.DescriptorTable.NumDescriptorRanges = table->num_ranges;
		d3d12_param.DescriptorTable.pDescriptorRanges = d3d12_ranges.data();
	}
	else if (auto* constants = std::get_if<RootParameter::Constants32Bits>(&root_parameter.getVariant()))
	{
		d3d12_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		d3d12_param.Constants.ShaderRegister = constants->shader_register;
		d3d12_param.Constants.Num32BitValues = constants->num_32bit_constants;
		d3d12_param.Constants.RegisterSpace = constants->register_space;
	}
	else if (auto* descriptor = std::get_if<RootParameter::Descriptor>(&root_parameter.getVariant()))
	{
		switch (descriptor->type)
		{
			case DescriptorType::kCBV: d3d12_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; break;
			case DescriptorType::kSRV: d3d12_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV; break;
			case DescriptorType::kUAV: d3d12_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV; break;
			default:                  d3d12_param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV; break;
		}
		d3d12_param.Descriptor.ShaderRegister = descriptor->shader_register;
		d3d12_param.Descriptor.RegisterSpace = descriptor->register_space;
	}
	return d3d12_param;
}


using DescriptorRangesStorage = std::array<std::array<D3D12_DESCRIPTOR_RANGE, kMaxDescriptorRangesPerTable>, kMaxRootParameters>;
using RootParametersStorage = std::array<D3D12_ROOT_PARAMETER, kMaxRootParameters>;

static D3D12_ROOT_SIGNATURE_DESC translateResourceLayoutStateToD3D12RootSignatureDesc(
	RootParametersStorage& d3d12_root_parameters,
	DescriptorRangesStorage& d3d12_descriptor_ranges,
	const ResourceLayoutState& resource_layout_state)
{
	const uint32_t num_params = static_cast<uint32_t>(resource_layout_state.getNumRootParameters());
	for (uint32_t i = 0; i < num_params; ++i)
	{
		d3d12_root_parameters[i] = translateRootParameterToD3D12RootParameter(
			resource_layout_state[i], d3d12_descriptor_ranges[i]);
	}

	D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
	root_signature_desc.NumParameters = num_params;
	root_signature_desc.pParameters = d3d12_root_parameters.data();
	root_signature_desc.NumStaticSamplers = 0;
	root_signature_desc.pStaticSamplers = nullptr;
	root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	return root_signature_desc;
}


std::unique_ptr<D3D12GraphicsPipelineState> D3D12GraphicsPipelineState::create(
	ID3D12Device9* device, const GraphicsPipelineDesc& desc, const GraphicsCompiledShaders& shaders)
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC d3d12_desc = {};

	ASSERT_MSG_RETURN_NULL(!shaders.vertex_shader->empty(), "Vertex shader bytecode is required for graphics pipeline state creation");
	d3d12_desc.VS = { shaders.vertex_shader->getBytecodeData(), shaders.vertex_shader->getBytecodeSize() };

	if (shaders.pixel_shader)
	{
		d3d12_desc.PS = { shaders.pixel_shader->getBytecodeData(), shaders.pixel_shader->getBytecodeSize() };
	}
	if (shaders.hull_shader || shaders.domain_shader)
	{
		ASSERT_MSG_RETURN_NULL(shaders.hull_shader && shaders.domain_shader, 
			"Hull shader and domain shader must both be provided for tessellation");

		d3d12_desc.HS = { shaders.hull_shader->getBytecodeData(), shaders.hull_shader->getBytecodeSize() };
		d3d12_desc.DS = { shaders.domain_shader->getBytecodeData(), shaders.domain_shader->getBytecodeSize() };
	}
	if (shaders.geometry_shader)
	{
		d3d12_desc.GS = { shaders.geometry_shader->getBytecodeData(), shaders.geometry_shader->getBytecodeSize() };
	}

	d3d12_desc.BlendState = translateOutputMergerStateToD3D12BlendDesc(desc.output_merger_state);
	d3d12_desc.SampleMask = UINT_MAX;  // Default to allowing all samples
	d3d12_desc.RasterizerState = translateRasterizerStateToD3D12RasterizerDesc(desc.rasterizer_state);
	d3d12_desc.DepthStencilState = translateDepthStencilDescToD3D12DepthStencilDesc(desc.output_merger_state.depth_stencil);
	std::vector<D3D12_INPUT_ELEMENT_DESC> input_elements;
	d3d12_desc.InputLayout = translateInputLayoutStateToD3D12InputLayoutDesc(input_elements, desc.input_layout_state);
	d3d12_desc.PrimitiveTopologyType = translatePrimitiveTopologyToD3D12PrimitiveTopologyType(desc.input_layout_state.topology);
	d3d12_desc.NumRenderTargets = desc.output_merger_state.render_target_count;
	for (uint32_t i = 0; i < desc.output_merger_state.render_target_count; ++i)
		d3d12_desc.RTVFormats[i] = internal::translateTextureFormatToDxgiFormat(desc.output_merger_state.render_targets[i].format);
	d3d12_desc.DSVFormat = internal::translateTextureFormatToDxgiFormat(desc.output_merger_state.depth_stencil.format);
	d3d12_desc.SampleDesc = { 1, 0 };  // Default to no multisampling

	DescriptorRangesStorage d3d12_descriptor_ranges = {};
	RootParametersStorage d3d12_root_parameters = {};
	D3D12_ROOT_SIGNATURE_DESC d3d12_root_signature_desc = translateResourceLayoutStateToD3D12RootSignatureDesc(
		d3d12_root_parameters, d3d12_descriptor_ranges, desc.resource_layout_state);

	Microsoft::WRL::ComPtr<ID3DBlob> serialized_root_signature;
	Microsoft::WRL::ComPtr<ID3DBlob> error_blob;
	D3D12_ASSERT_RETURN_NULL(D3D12SerializeRootSignature(
		&d3d12_root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &serialized_root_signature, &error_blob));

	Microsoft::WRL::ComPtr<ID3D12RootSignature> root_signature;
	D3D12_ASSERT_RETURN_NULL(device->CreateRootSignature(0, serialized_root_signature->GetBufferPointer(),
		serialized_root_signature->GetBufferSize(), IID_PPV_ARGS(&root_signature)));

	d3d12_desc.pRootSignature = root_signature.Get();

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipeline_state;
	D3D12_ASSERT_RETURN_NULL(device->CreateGraphicsPipelineState(&d3d12_desc, IID_PPV_ARGS(&pipeline_state)));

	return std::unique_ptr<D3D12GraphicsPipelineState>(new D3D12GraphicsPipelineState(desc, pipeline_state.Detach(), root_signature.Detach()));
}


D3D12GraphicsPipelineState::D3D12GraphicsPipelineState(
	const GraphicsPipelineDesc& desc, ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature)
	: IGraphicsPipelineState(desc)
	, pipeline_state_(pipeline_state)
	, root_signature_(root_signature)
{
}


void D3D12GraphicsPipelineState::setGpuDebugName()
{
	SET_D3D12_RESOURCE_DEBUG_NAME(pipeline_state_, getName());
	SET_D3D12_RESOURCE_DEBUG_NAME(root_signature_, ("RootSignature_" + getName()).c_str());
}


std::unique_ptr<D3D12ComputePipelineState> D3D12ComputePipelineState::create(
	ID3D12Device9* device, const ComputePipelineDesc& desc, const ComputeCompiledShaders& shaders)
{
	return std::unique_ptr<D3D12ComputePipelineState>();
}


D3D12ComputePipelineState::D3D12ComputePipelineState(
	const ComputePipelineDesc& desc, ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature)
	: IComputePipelineState(desc)
	, pipeline_state_(pipeline_state)
	, root_signature_(root_signature)
{
}


void D3D12ComputePipelineState::setGpuDebugName()
{
	SET_D3D12_RESOURCE_DEBUG_NAME(pipeline_state_, getName());
	SET_D3D12_RESOURCE_DEBUG_NAME(root_signature_, ("RootSignature_" + getName()).c_str());
}


std::unique_ptr<D3D12MeshPipelineState> D3D12MeshPipelineState::create(ID3D12Device9* device, const MeshPipelineDesc& desc, const MeshCompiledShaders& shaders)
{
	return std::unique_ptr<D3D12MeshPipelineState>();
}


D3D12MeshPipelineState::D3D12MeshPipelineState(
	const MeshPipelineDesc& desc, ID3D12PipelineState* pipeline_state, ID3D12RootSignature* root_signature)
	: IMeshPipelineState(desc)
	, pipeline_state_(pipeline_state)
	, root_signature_(root_signature)
{
}


void D3D12MeshPipelineState::setGpuDebugName()
{
	SET_D3D12_RESOURCE_DEBUG_NAME(pipeline_state_, getName());
	SET_D3D12_RESOURCE_DEBUG_NAME(root_signature_, ("RootSignature_" + getName()).c_str());
}

}  // namespace render
