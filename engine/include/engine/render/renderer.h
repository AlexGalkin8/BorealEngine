#pragma once

#include <engine/core/utility/utility.h>

#include <cstdint>
#include <memory>


namespace scene {
class Scene;
class Camera;
}  // namespace scene

namespace render {
class IRenderDevice;
class ICommandList;
class IPipelineStateManager;
class ITexture;
class IBuffer;
}  // namespace render


namespace render {

enum class EShadingTechnique
{
	kDeferred = 0,
};

enum class EDebugRenderView : uint32_t
{
	kFinal = 0,
	kAlbedo = 1,
	kWorldNormal = 2,
	kLightingHDR = 3,
	kRoughness = 4,
	kMetallic = 5,
};

enum class ELightingModel : uint32_t
{
	kDiffuse = 0,
	kBlinnPhong = 1,
	kPBR = 2,
};

struct RenderSettings
{
	uint32_t render_width = 0;
	uint32_t render_height = 0;
	EDebugRenderView debug_view = EDebugRenderView::kFinal;
	ELightingModel lighting_model = ELightingModel::kPBR;
};


class IRenderer : public utility::NonCopyable
{
public:
	template<EShadingTechnique shading_technique>
	static std::unique_ptr<IRenderer> create(IRenderDevice& render_device, const RenderSettings& render_settings);

	virtual void render(const scene::Scene& scene, const scene::Camera& camera, ITexture& output_target) = 0;

	const RenderSettings& getRenderSettings() const { return render_settings_; }
	void setDebugView(EDebugRenderView view) { render_settings_.debug_view = view; }

	void setLightingModel(ELightingModel model) { render_settings_.lighting_model = model; }

	virtual void resize(uint32_t width, uint32_t height) = 0;

	virtual ~IRenderer();

protected:
	IRenderer(IRenderDevice& render_device, const RenderSettings& render_settings);

	IRenderDevice& getRenderDevice() { return render_device_; }
	const IRenderDevice& getRenderDevice() const { return render_device_; }

protected:
	RenderSettings render_settings_;
	IRenderDevice& render_device_;
	IPipelineStateManager& pipeline_state_manager_;
};

}  // namespace render
