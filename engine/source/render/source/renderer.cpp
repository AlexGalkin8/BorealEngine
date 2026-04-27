#include "renderer.h"
#include "deferredrenderer.hpp"
#include "interface/renderdevice.h"
#include "resourcemanager.hpp"
#include "pipelinestatemanager.hpp"


namespace render {

template<EShadingTechnique>
std::unique_ptr<IRenderer> IRenderer::create(IRenderDevice&) = delete;


template<>
std::unique_ptr<IRenderer> IRenderer::create<EShadingTechnique::kDeferred>(IRenderDevice& render_device)
{
	return DeferredRenderer::create(render_device);
}


void IRenderer::render(const scene::Scene& scene, const scene::Camera& camera, IDeviceContext& ctx, ISwapChain& swap_chain)
{
}


IRenderer::~IRenderer()
{

}


IRenderer::IRenderer(IRenderDevice& render_device)
	: render_device_(render_device)
	, pipeline_state_manager_(render_device.getPipelineStateManager())
	, resource_manager_(render_device.getResourceManager())
{
}

}  // namespace render
