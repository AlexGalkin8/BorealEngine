#pragma once

#include "pipelinestatemanager.hpp"

#include <memory>


namespace render {

class IRenderDevice;
class D3D12RenderDevice;


class D3D12PipelineStateManager final : public IPipelineStateManager
{
	friend class IPipelineStateManager;

public:
	~D3D12PipelineStateManager() override = default;

	std::unique_ptr<IPipelineState> createPipelineStateInternal(const GraphicsPipelineDesc& desc) override;
	std::unique_ptr<IPipelineState> createPipelineStateInternal(const ComputePipelineDesc& desc) override;
	std::unique_ptr<IPipelineState> createPipelineStateInternal(const MeshPipelineDesc& desc) override;

private:
	D3D12PipelineStateManager(IRenderDevice& render_device, std::unique_ptr<IShaderCompiler>&& shader_compiler);

	D3D12RenderDevice& getRenderDevice();
};

}  // namespace render
