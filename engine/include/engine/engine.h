#pragma once

#include <engine/core/utility/utility.h>
#include <engine/core/time/time.h>
#include <engine/settings.h>

#include <memory>
#include <string>
#include <vector>


namespace platform {
class IWindow;
class IInput;
}  // namespace platform

namespace scene {
class Scene;
}  // namespace scene

namespace render {
struct FrameCaptureSettings;
class IRenderDevice;
class IRenderer;
class ISwapChain;
class ImGuiBackend;
}  // namespace render


namespace engine {
class DebugMenu;
}  // namespace engine


class Engine : public utility::NonCopyable
{
public:
	static std::unique_ptr<Engine> create();
	static std::unique_ptr<Engine> create(const std::string& settings_file_name);
	static std::unique_ptr<Engine> create(const std::string& settings_file_name,
		const render::FrameCaptureSettings& frame_capture);

	void run();

	platform::IInput* getInput() const { return input_.get(); }

	~Engine();

private:
	Engine();

	bool loadScene(const std::string& scene_name);
	void handleResize(uint32_t width, uint32_t height);

	void stop();
	void pause();
	void resume();
	bool isPaused() const;

	void frame();

private:
	std::unique_ptr<engine::Settings> settings_;

	std::unique_ptr<platform::IWindow> window_;
	std::unique_ptr<platform::IInput>  input_;

	std::unique_ptr<render::IRenderDevice> render_device_;
	std::unique_ptr<render::ISwapChain> swap_chain_;
	std::unique_ptr<render::IRenderer> renderer_;
	std::unique_ptr<render::ImGuiBackend> imgui_backend_;
	std::unique_ptr<engine::DebugMenu> debug_menu_;

	std::vector<std::string> scene_names_;
	std::string active_scene_name_;
	std::unique_ptr<scene::Scene> current_scene_;

	bool is_running_ = false;
	bool pause_on_focus_lost_ = false;
	core::Time time_;
};
