#pragma once

#include <cstdint>


namespace render {
class IRenderDevice;
}  // namespace render


namespace engine {

class DebugMenu
{
public:
	bool isVisible() const { return visible_; }
	void toggle() { visible_ = !visible_; }
	void draw(uint32_t width, uint32_t height, bool& pause_on_focus_lost, render::IRenderDevice& render_device);

private:
	bool visible_ = false;
	bool show_rendering_stats_ = false;
	bool show_imgui_metrics_ = false;
};

}  // namespace engine
