#pragma once

#include "interface/common.h"
#include "interface/pipelinestatedesc.hpp"

#include <memory>
#include <unordered_map>
#include <string>
#include <functional>


namespace render {
class IRenderDevice;
class IShaderCompiler;
class IPipelineState;
struct GraphicsPipelineDesc;
struct ComputePipelineDesc;
struct MeshPipelineDesc;
}  // namespace render


namespace render {

class IPipelineStateManager
{
public:
	template<ERenderAPI render_api>
	static std::unique_ptr<IPipelineStateManager> create(IRenderDevice& render_device);

	IPipelineState& createPipelineState(const GraphicsPipelineDesc& desc);
	IPipelineState& createPipelineState(const ComputePipelineDesc& desc);
	IPipelineState& createPipelineState(const MeshPipelineDesc& desc);

	virtual ~IPipelineStateManager();

protected:
	IPipelineStateManager(IRenderDevice& render_device, std::unique_ptr<IShaderCompiler>&& shader_compiler);

	IRenderDevice& getRenderDevice() { return render_device_; };

	using PipelineStateKey = std::string;

	PipelineStateKey makePipelineStateKey(const GraphicsPipelineDesc& desc);
	PipelineStateKey makePipelineStateKey(const ComputePipelineDesc& desc);
	PipelineStateKey makePipelineStateKey(const MeshPipelineDesc& desc);

	GraphicsCompiledShaders compileGraphicsShaders(const GraphicsPipelineDesc& desc);
	ComputeCompiledShaders compileComputeShaders(const ComputePipelineDesc& desc);
	MeshCompiledShaders compileMeshShaders(const MeshPipelineDesc& desc);

	virtual std::unique_ptr<IPipelineState> createPipelineStateInternal(const GraphicsPipelineDesc& desc) = 0;
	virtual std::unique_ptr<IPipelineState> createPipelineStateInternal(const ComputePipelineDesc& desc) = 0;
	virtual std::unique_ptr<IPipelineState> createPipelineStateInternal(const MeshPipelineDesc& desc) = 0;

protected:
	IRenderDevice& render_device_;
	std::unique_ptr<IShaderCompiler> shader_compiler_;
	std::unordered_map<PipelineStateKey, std::unique_ptr<IPipelineState>> graphics_pipeline_state_cache_;
	std::unordered_map<PipelineStateKey, std::unique_ptr<IPipelineState>> compute_pipeline_state_cache_;
	std::unordered_map<PipelineStateKey, std::unique_ptr<IPipelineState>> mesh_pipeline_state_cache_;
};

}  // namespace render
