#include "engine.h"

#include <engine/core/utility/assert.h>
#include <engine/platform/window.h>
#include <engine/scene/scene.h>
#include <engine/scene/camera.h>
#include <engine/scene/resourcemanager.h>
#include <engine/scene/sceneloader.h>
#include <engine/render/interface/renderdevice.h>
#include <engine/render/interface/devicecontext.h>
#include <engine/render/interface/swapchain.h>
#include <engine/render/renderer.h>
#include <engine/scene/common.h>


std::unique_ptr<Engine> Engine::create(const std::string& settings_file_name, std::vector<std::wstring>&& cmd_args)
{
	// TODO handling cmd_args

	std::unique_ptr<Engine> engine = std::unique_ptr<Engine>(new Engine());

	engine->settings_ = engine::loadSettingsFromFile(settings_file_name);
	ASSERT_MSG_RETURN_NULL(engine->settings_ != nullptr, "Failed to load settings file: %s", settings_file_name.c_str());

	engine->resource_manager_ = scene::ResourceManager::create();
	ASSERT_MSG_RETURN_NULL(engine->resource_manager_ != nullptr, "Failed to create scene::ResourceManager");

	render::IRenderDevice::CreationDesc render_device_desc = {};
	render_device_desc.enable_debug_validation_layer = false;

	engine->render_device_ = render::IRenderDevice::create<render::ERenderAPI::kD3D12>(render_device_desc);
	ASSERT_MSG_RETURN_NULL(engine->render_device_ != nullptr, "Failed to create render device");

	engine->renderer_ = render::IRenderer::create<render::EShadingTechnique::kDeferred>(*engine->render_device_);
	ASSERT_MSG_RETURN_NULL(engine->renderer_ != nullptr, "Failed to create renderer");

	platform::IWindow::Desc window_desc = {};
	window_desc.name = engine->settings_->window_name;
	window_desc.mode = platform::IWindow::EMode::kWindow;
	window_desc.width = engine->settings_->window_width;
	window_desc.height = engine->settings_->window_height;

	engine->window_ = platform::IWindow::create(window_desc);
	ASSERT_MSG_RETURN_NULL(engine->window_ != nullptr, "Failed to create window");

	engine->swap_chain_ = engine->render_device_->createSwapChain(*engine->window_);
	ASSERT_MSG_RETURN_NULL(engine->swap_chain_ != nullptr, "Failed to create swap chain");

	// Create default scene
	{
		std::unique_ptr<scene::Scene> default_scene = scene::SceneLoader::loadFromFile(
			engine->settings_->default_scene_name, *engine->resource_manager_, engine->settings_->data_root_path);
		ASSERT_MSG_RETURN_NULL(default_scene != nullptr, "Failed to create default scene");

		const float aspect = static_cast<float>(engine->settings_->window_width) / static_cast<float>(engine->settings_->window_height);
		default_scene->getCamera().setAspect(aspect);

		engine->scenes_.push_back(std::move(default_scene));
		engine->current_scene_ = engine->scenes_.back().get();
	}

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

	if (current_scene_)
	{
		const float aspect = static_cast<float>(width) / static_cast<float>(height);
		current_scene_->getCamera().setAspect(aspect);
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


void Engine::frame()
{
	double dt = time_.tick(0.0, 0.1);

	if (time_.isPaused())
	{
		return;
	}

	render::IDeviceContext& ctx = render_device_->getImmediateContext();

	ctx.begin();

	if (current_scene_)
	{
		current_scene_->update(dt);
		renderer_->render(*current_scene_, current_scene_->getCamera(), ctx, *swap_chain_);
	}

	ctx.end();
	ctx.flush();
	ctx.present(*swap_chain_);
}
