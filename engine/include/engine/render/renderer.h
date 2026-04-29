#pragma once

#include <engine/core/utility/utility.h>

#include <memory>


namespace scene {
class Scene;
class Camera;
}  // namespace scene

namespace render {
class IRenderDevice;
class IDeviceContext;
class ISwapChain;
class IPipelineStateManager;
class IResourceManager;
}  // namespace render


namespace render {

enum class EShadingTechnique
{
	kDeferred = 0,
};


class IRenderer : public utility::NonCopyable
{
public:

	template<EShadingTechnique shading_technique>
	static std::unique_ptr<IRenderer> create(IRenderDevice& render_device);

	virtual void render(const scene::Scene& scene, const scene::Camera& camera, IDeviceContext& ctx, ISwapChain& swap_chain);

	virtual ~IRenderer();

protected:
	IRenderer(IRenderDevice& render_device);

	IRenderDevice& getRenderDevice() { return render_device_; }
	const IRenderDevice& getRenderDevice() const { return render_device_; }

protected:
	IRenderDevice& render_device_;
	IPipelineStateManager& pipeline_state_manager_;
	IResourceManager& resource_manager_;
};

}  // namespace render
