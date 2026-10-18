#include "renderer.h"
#include "deferredrenderer.hpp"

#include "interface/renderdevice.h"

#include "pipelinestatemanager.hpp"


namespace render {

template<EShadingTechnique>
std::unique_ptr<IRenderer> IRenderer::create(IRenderDevice&, const RenderSettings&) = delete;


template<>
std::unique_ptr<IRenderer> IRenderer::create<EShadingTechnique::kDeferred>(IRenderDevice& render_device, const RenderSettings& render_settings)
{
	return DeferredRenderer::create(render_device, render_settings);
}


IRenderer::~IRenderer()
{

}


IRenderer::IRenderer(IRenderDevice& render_device, const RenderSettings& render_settings)
	: render_device_(render_device)
	, pipeline_state_manager_(render_device.getPipelineStateManager())
	, render_settings_(render_settings)
{
}

}  // namespace render
