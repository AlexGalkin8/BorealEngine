#pragma once

#include <engine/core/utility/utility.h>
#include <engine/core/time/time.h>

#include <memory>
#include <string>
#include <unordered_set>


namespace platform {
class IWindow;
}  // namespace platform

namespace scene {
class Scene;
}  // namespace scene

namespace render {
class IRenderDevice;
class IRenderer;
class ISwapChain;
}  // namespace render


class Engine : public utility::NonCopyable
{
public:
	struct CreationDesc
	{
		std::string data_root_path = "data";
		std::wstring window_name = L"BorealEngine";
		uint32_t window_width = 1280;
		uint32_t window_height = 720;
	};

public:
	static std::unique_ptr<Engine> create(const CreationDesc& creation_desc, std::vector<std::wstring>&& cmd_args = {});

	void run();

	void setPauseOnFocusLost(bool enabled);
	bool isPauseOnFocusLost() const;

	const std::string& getDataRootPath() const { return data_root_path_; }

	void addScene(const std::shared_ptr<scene::Scene>& scene);
	void removeScene(const std::shared_ptr<scene::Scene>& scene);

	void stop();
	void pause();
	void resume();
	bool isPaused() const;

	~Engine();

private:
	Engine();
	void handleResize(uint32_t width, uint32_t height);

	void frame();

private:
	std::unique_ptr<platform::IWindow> window_;
	std::unique_ptr<render::IRenderDevice> render_device_;
	std::unique_ptr<render::ISwapChain> swap_chain_;
	std::unique_ptr<render::IRenderer> renderer_;
	std::unordered_set<std::shared_ptr<scene::Scene>> scenes_;
	std::string data_root_path_;

	bool is_running_ = false;
	bool pause_on_focus_lost_ = false;
	core::Time time_{};
};
