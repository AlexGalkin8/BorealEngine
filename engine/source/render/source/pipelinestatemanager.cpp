#include "pipelinestatemanager.hpp"
#include "interface/renderdevice.h"
#include "interface/pipelinestate.hpp"
#include "interface/shader.hpp"
#include "interface/d3d12/d3d12shadercompiler.hpp"
#include "interface/d3d12/d3d12pipelinestatemanager.hpp"

#include <sstream>


namespace render {

static EShaderType translateShaderStageToShaderType(EGraphicsShaderStage stage)
{
	switch (stage)
	{
		case EGraphicsShaderStage::kVertex:   return EShaderType::kVertex;
		case EGraphicsShaderStage::kPixel:    return EShaderType::kPixel;
		case EGraphicsShaderStage::kDomain:   return EShaderType::kDomain;
		case EGraphicsShaderStage::kHull:     return EShaderType::kHull;
		case EGraphicsShaderStage::kGeometry: return EShaderType::kGeometry;
		default:                              return EShaderType::kVertex; // Default to vertex shader for unknown stages
	}
}


static EShaderType translateShaderStageToShaderType(EComputeShaderStage stage)
{
	return EShaderType::kCompute;
}


static EShaderType translateShaderStageToShaderType(EMeshShaderStage stage)
{
	switch (stage)
	{
		case EMeshShaderStage::kAmplification: return EShaderType::kAmplification;
		case EMeshShaderStage::kMesh:          return EShaderType::kMesh;
		case EMeshShaderStage::kPixel:         return EShaderType::kPixel;
		default:                               return EShaderType::kMesh;
	}
}


static const wchar_t* translateShaderStageToShaderEntryPoint(EGraphicsShaderStage stage)
{
	switch (stage)
	{
		case EGraphicsShaderStage::kVertex:   return L"vertexShader";
		case EGraphicsShaderStage::kPixel:    return L"pixelShader";
		case EGraphicsShaderStage::kDomain:   return L"domainShader";
		case EGraphicsShaderStage::kHull:     return L"hullShader";
		case EGraphicsShaderStage::kGeometry: return L"geometryShader";
		default:                              return L"vertexShader";
	}
}


template<ERenderAPI>
std::unique_ptr<IPipelineStateManager> IPipelineStateManager::create(IRenderDevice& render_device) = delete;


template<>
std::unique_ptr<IPipelineStateManager> IPipelineStateManager::create<ERenderAPI::kD3D12>(IRenderDevice& render_device)
{
	std::unique_ptr<IShaderCompiler> shader_compiler = D3D12ShaderCompiler::create(true);
	return std::unique_ptr<D3D12PipelineStateManager>(new D3D12PipelineStateManager(render_device, std::move(shader_compiler)));
}


IPipelineState& IPipelineStateManager::createPipelineState(const GraphicsPipelineDesc& desc)
{
	PipelineStateKey key = makePipelineStateKey(desc);

	if (graphics_pipeline_state_cache_.count(key))
	{
		return *graphics_pipeline_state_cache_.at(key);
	}
	else
	{
		GraphicsCompiledShaders compiled_shaders = compileGraphicsShaders(desc);
		std::unique_ptr<IPipelineState> pipeline_state = createPipelineStateInternal(desc);
		graphics_pipeline_state_cache_.emplace(key, std::move(pipeline_state));
		return *graphics_pipeline_state_cache_.at(key);
	}
}


IPipelineState& IPipelineStateManager::createPipelineState(const ComputePipelineDesc& desc)
{
	// TODO: Implement compute pipeline state creation
	throw std::runtime_error("ComputePipelineState creation not yet implemented");
}


IPipelineState& IPipelineStateManager::createPipelineState(const MeshPipelineDesc& desc)
{
	// TODO: Implement mesh pipeline state creation
	throw std::runtime_error("MeshPipelineState creation not yet implemented");
}


IPipelineStateManager::~IPipelineStateManager()
{

}


IPipelineStateManager::IPipelineStateManager(IRenderDevice& render_device, std::unique_ptr<IShaderCompiler>&& shader_compiler)
	: render_device_(render_device)
	, shader_compiler_(std::move(shader_compiler))

{
}


IPipelineStateManager::PipelineStateKey IPipelineStateManager::makePipelineStateKey(const GraphicsPipelineDesc& desc)
{
	std::ostringstream oss;

	// Shader state
	oss << desc.shader_state.shader_name << '|';
	for (const auto& define : desc.shader_state.defines)
	{
		oss << utility::translateWStringToString(define.name) << '|';
		oss << utility::translateWStringToString(define.value) << '|';
	}
	oss << static_cast<uint32_t>(desc.shader_state.shader_stages) << '|';

	// Input layout
	oss << static_cast<uint32_t>(desc.input_layout_state.topology) << '|';
	for (const auto& attr : desc.input_layout_state.vertex_attributes)
	{
		oss << static_cast<uint32_t>(attr.semantic.type) << ':' << attr.semantic.index << ':';
		oss << static_cast<uint32_t>(attr.format) << ':' << attr.input_slot << ':' << attr.aligned_byte_offset << ';';
	}
	oss << '|';

	// Rasterizer
	oss << static_cast<uint32_t>(desc.rasterizer_state.fill_mode) << ':' << static_cast<uint32_t>(desc.rasterizer_state.cull_mode)
		<< ':' << (desc.rasterizer_state.depth_clip_enable ? '1' : '0') << '|';

	// Output merger / render targets
	oss << desc.output_merger_state.render_target_count << '|';
	for (uint32_t i = 0; i < desc.output_merger_state.render_target_count; ++i)
	{
		const auto& rt = desc.output_merger_state.render_targets[i];
		oss << static_cast<uint32_t>(rt.format) << ':' << static_cast<uint32_t>(rt.blend) << ';';
	}
	oss << '|';

	// Depth stencil
	oss << static_cast<uint32_t>(desc.output_merger_state.depth_stencil.format) << ':'
		<< (desc.output_merger_state.depth_stencil.depth_enable ? '1' : '0') << ':'
		<< (desc.output_merger_state.depth_stencil.depth_write ? '1' : '0') << ':'
		<< static_cast<uint32_t>(desc.output_merger_state.depth_stencil.depth_func) << ':'
		<< (desc.output_merger_state.depth_stencil.stencil_enable ? '1' : '0');

	oss << desc.resource_layout_state.makeHash();

	return oss.str();
}


GraphicsCompiledShaders IPipelineStateManager::compileGraphicsShaders(const GraphicsPipelineDesc& desc)
{
	ASSERT_MSG_RETURN_EMPTY(!desc.shader_state.shader_name.empty(), "Shader name is required for creating graphics pipeline state");

	GraphicsCompiledShaders compiled_shaders = {};

	ASSERT_MSG_RETURN_EMPTY((desc.shader_state.shader_stages | EGraphicsShaderStage::kVertex) != EGraphicsShaderStage(0),
		"Vertex shader stage is required for creating graphics pipeline state. Shader name=%s", desc.shader_state.shader_name);

	compiled_shaders.vertex_shader = shader_compiler_->compile(
		translateShaderStageToShaderType(EGraphicsShaderStage::kVertex),
		desc.shader_state.shader_name,
		translateShaderStageToShaderEntryPoint(EGraphicsShaderStage::kVertex),
		desc.shader_state.defines);

	if ((desc.shader_state.shader_stages & EGraphicsShaderStage::kPixel) != EGraphicsShaderStage(0))
	{
		compiled_shaders.pixel_shader = shader_compiler_->compile(
			translateShaderStageToShaderType(EGraphicsShaderStage::kPixel),
			desc.shader_state.shader_name,
			translateShaderStageToShaderEntryPoint(EGraphicsShaderStage::kPixel),
			desc.shader_state.defines);
	}

	if ((desc.shader_state.shader_stages & EGraphicsShaderStage::kHull) != EGraphicsShaderStage(0) &&
		(desc.shader_state.shader_stages & EGraphicsShaderStage::kDomain) != EGraphicsShaderStage(0))
	{
		compiled_shaders.hull_shader = shader_compiler_->compile(
			translateShaderStageToShaderType(EGraphicsShaderStage::kHull),
			desc.shader_state.shader_name,
			translateShaderStageToShaderEntryPoint(EGraphicsShaderStage::kHull),
			desc.shader_state.defines);

		compiled_shaders.domain_shader = shader_compiler_->compile(
			translateShaderStageToShaderType(EGraphicsShaderStage::kDomain),
			desc.shader_state.shader_name,
			translateShaderStageToShaderEntryPoint(EGraphicsShaderStage::kDomain),
			desc.shader_state.defines);
	}

	if ((desc.shader_state.shader_stages & EGraphicsShaderStage::kGeometry) != EGraphicsShaderStage(0))
	{
		compiled_shaders.geometry_shader = shader_compiler_->compile(
			translateShaderStageToShaderType(EGraphicsShaderStage::kGeometry),
			desc.shader_state.shader_name,
			translateShaderStageToShaderEntryPoint(EGraphicsShaderStage::kGeometry),
			desc.shader_state.defines);
	}

	return compiled_shaders;
}


ComputeCompiledShaders IPipelineStateManager::compileComputeShaders(const ComputePipelineDesc& desc)
{
	ComputeCompiledShaders compiled_shaders = {};
	compiled_shaders.compute_shader = shader_compiler_->compile(
		translateShaderStageToShaderType(EComputeShaderStage::kCompute),
		desc.shader_state.shader_name,
		L"computeShader",
		desc.shader_state.defines);
	return ComputeCompiledShaders();
}


MeshCompiledShaders IPipelineStateManager::compileMeshShaders(const MeshPipelineDesc& desc)
{
	MeshCompiledShaders compiled_shaders = {};
	ASSERT_FAIL("Mesh shader compilation not yet implemented");
	return compiled_shaders;
}

}  // namespace render
