#pragma once

#include <string>
#include <vector>


namespace render {
class IRenderDevice;
class IRenderer;
}  // namespace render


namespace scene {
class Scene;
}  // namespace scene


namespace engine {

class DebugMenu
{
public:
	DebugMenu(render::IRenderDevice& render_device, std::vector<std::string> scene_names,
		const std::string& active_scene_name);

	bool isVisible() const { return visible_; }
	void toggle() { visible_ = !visible_; }
	bool shouldPauseOnFocusLost() const { return pause_on_focus_lost_; }
	std::string draw(scene::Scene& scene, render::IRenderer& renderer);

private:
	render::IRenderDevice& render_device_;
	const std::vector<std::string> scene_names_;
	const std::string& active_scene_name_;

	bool pause_on_focus_lost_ = false;
	bool visible_ = false;
	bool show_directional_light_ = false;
	bool show_rendering_stats_ = false;
	bool show_imgui_metrics_ = false;
};

}  // namespace engine
