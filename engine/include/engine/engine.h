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
class ResourceManager;
}  // namespace scene

namespace render {
class IRenderDevice;
class IRenderer;
class ISwapChain;
}  // namespace render


class Engine : public utility::NonCopyable
{
public:
	static std::unique_ptr<Engine> create(const std::string& settings_file_name, std::vector<std::wstring>&& cmd_args = {});

	void run();

	platform::IInput* getInput() const { return input_.get(); }

	~Engine();

private:
	Engine();

	void handleResize(uint32_t width, uint32_t height);

	void setPauseOnFocusLost(bool enabled);
	bool isPauseOnFocusLost() const;

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

	std::unique_ptr<scene::ResourceManager> resource_manager_;
	std::vector<std::unique_ptr<scene::Scene>> scenes_;
	scene::Scene* current_scene_ = nullptr;

	bool is_running_ = false;
	bool pause_on_focus_lost_ = false;
	core::Time time_;
};
