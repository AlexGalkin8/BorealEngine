#include "d3d12pipelinestatemanager.hpp"
#include "d3d12pipelinestate.hpp"
#include "d3d12renderdevice.hpp"


namespace render {

std::unique_ptr<IPipelineState>D3D12PipelineStateManager::createPipelineStateInternal(const GraphicsPipelineDesc& desc)
{
	return D3D12GraphicsPipelineState::create(getRenderDevice().getDevice(), desc, compileGraphicsShaders(desc));
}


std::unique_ptr<IPipelineState> D3D12PipelineStateManager::createPipelineStateInternal(const ComputePipelineDesc& desc)
{
	return D3D12ComputePipelineState::create(getRenderDevice().getDevice(), desc, compileComputeShaders(desc));
}


std::unique_ptr<IPipelineState> D3D12PipelineStateManager::createPipelineStateInternal(const MeshPipelineDesc& desc)
{
	return D3D12MeshPipelineState::create(getRenderDevice().getDevice(), desc, compileMeshShaders(desc));
}


D3D12PipelineStateManager::D3D12PipelineStateManager(
	IRenderDevice& render_device, std::unique_ptr<IShaderCompiler>&& shader_compiler)
	: IPipelineStateManager(render_device, std::move(shader_compiler))
{

}


D3D12RenderDevice& D3D12PipelineStateManager::getRenderDevice()
{
	return static_cast<D3D12RenderDevice&>(IPipelineStateManager::getRenderDevice());
}


}  // namespace render
