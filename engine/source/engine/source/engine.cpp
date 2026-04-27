#include "engine.h"

#include <engine/platform/window.h>
#include <engine/scene/scene.h>
#include <engine/scene/camera.h>
#include <engine/render/interface/renderdevice.h>
#include <engine/render/interface/devicecontext.h>
#include <engine/render/interface/swapchain.h>
#include <engine/render/renderer.h>


std::unique_ptr<Engine> Engine::create(const CreationDesc& creation_desc, std::vector<std::wstring>&& cmd_args)
{
	// TODO handling cmd_args

	std::unique_ptr<Engine> engine = std::unique_ptr<Engine>(new Engine);
	engine->data_root_path_ = creation_desc.data_root_path;

	render::IRenderDevice::CreationDesc render_device_desc = {};
	render_device_desc.enable_debug_validation_layer = false;

	engine->render_device_ = render::IRenderDevice::create<render::ERenderAPI::kD3D12>(render_device_desc);

	engine->renderer_ = render::IRenderer::create<render::EShadingTechnique::kDeferred>(*engine->render_device_);

	platform::IWindow::Desc window_desc = {};
	window_desc.name = creation_desc.window_name;
	window_desc.mode = platform::IWindow::EMode::kWindow;
	window_desc.width = creation_desc.window_width;
	window_desc.height = creation_desc.window_height;

	engine->window_ = platform::IWindow::create(window_desc);

	engine->swap_chain_ = engine->render_device_->createSwapChain(*engine->window_);

	return engine;
}


void Engine::run()
{
	is_running_ = true;
	time_.reset();

	while (is_running_)
	{
		if (window_ && !window_->pumpEvents())
		{
			stop();
			break;
		}

		if (window_)
		{
			uint32_t new_width = 0;
			uint32_t new_height = 0;
			if (window_->consumeResize(new_width, new_height))
			{
				handleResize(new_width, new_height);
			}
		}

		if (pause_on_focus_lost_ && window_)
		{
			bool active = window_->isActive();
			if (!active && !isPaused())
			{
				pause();
			}
			else if (active && isPaused())
			{
				resume();
			}
		}

		frame();
	}
}


void Engine::setPauseOnFocusLost(bool enabled)
{
	pause_on_focus_lost_ = enabled;
}


bool Engine::isPauseOnFocusLost() const
{
	return pause_on_focus_lost_;
}


void Engine::addScene(const std::shared_ptr<scene::Scene>& scene)
{
	scenes_.insert(scene);
}


void Engine::removeScene(const std::shared_ptr<scene::Scene>& scene)
{
	scenes_.erase(scene);
}

void Engine::stop()
{
	is_running_ = false;
}


void Engine::pause()
{
	time_.pause();
}


void Engine::resume()
{
	time_.resume();
}


bool Engine::isPaused() const
{
	return time_.isPaused();
}


Engine::~Engine()
{
	render::IDeviceContext& ctx = render_device_->getImmediateContext();
	ctx.waitIdle();
}


Engine::Engine()
	: is_running_(false)
	, pause_on_focus_lost_(false)
{
}


void Engine::handleResize(uint32_t width, uint32_t height)
{
	if (!swap_chain_ || !render_device_ || width == 0 || height == 0)
	{
		return;
	}

	render::IDeviceContext& ctx = render_device_->getImmediateContext();
	ctx.waitIdle();
	swap_chain_->resize(width, height);

	const float aspect = static_cast<float>(width) / static_cast<float>(height);
	for (auto& scene : scenes_)
	{
		scene->getCamera().setAspect(aspect);
	}
}


void Engine::frame()
{
	double dt = time_.tick(0.0, 0.1);

	if (time_.isPaused())
	{
		return;
	}

	render::IDeviceContext& ctx = render_device_->getImmediateContext();

	ctx.begin();

	for (auto& scene : scenes_)
	{
		scene->update(dt);
	}

	for (const auto& scene : scenes_)
	{
		renderer_->render(*scene.get(), scene->getCamera(), ctx, *swap_chain_);
	}

	ctx.end();
	ctx.flush();
	ctx.present(*swap_chain_);
}
