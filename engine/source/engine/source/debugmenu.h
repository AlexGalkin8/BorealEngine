#pragma once

#include <cstdint>
#include <string>
#include <vector>


namespace render {
class IRenderDevice;
class IRenderer;
}  // namespace render


namespace engine {

class DebugMenu
{
public:
	bool isVisible() const { return visible_; }
	void toggle() { visible_ = !visible_; }
	std::string draw(uint32_t width, uint32_t height, bool& pause_on_focus_lost, render::IRenderDevice& render_device,
		render::IRenderer& renderer, const std::vector<std::string>& scene_names, const std::string& active_scene_name);

private:
	bool visible_ = false;
	bool show_rendering_stats_ = false;
	bool show_imgui_metrics_ = false;
};

}  // namespace engine
