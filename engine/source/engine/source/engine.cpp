#include "engine.h"

#include "commandlinehandler.h"
#include "debugmenu.h"

#include <engine/core/utility/assert.h>
#include <engine/platform/window.h>
#include <engine/platform/input.h>
#include <engine/scene/scene.h>
#include <engine/scene/camera.h>
#include <engine/scene/resourcemanager.h>
#include <engine/scene/sceneloader.h>
#include <engine/render/interface/renderdevice.h>
#include <engine/render/interface/swapchain.h>
#include <engine/render/imguibackend.h>
#include <engine/render/renderer.h>
#include <engine/scene/common.h>

#include <utility>


std::unique_ptr<Engine> Engine::create()
{
	const engine::CommandLineHandler command_line = engine::CommandLineHandler::fromProcess();
	engine::CommandLineHandler::StartupOptions options;
	if (!command_line.handleCommandLine(options))
	{
		return nullptr;
	}

	return create(utility::translateWStringToString(options.settings_file_path), options.frame_capture);
}


std::unique_ptr<Engine> Engine::create(const std::string& settings_file_name)
{
	return create(settings_file_name, {});
}


std::unique_ptr<Engine> Engine::create(const std::string& settings_file_name,
	const render::FrameCaptureSettings& frame_capture)
{
	std::unique_ptr<Engine> engine = std::unique_ptr<Engine>(new Engine());

	engine->settings_ = engine::loadSettingsFromFile(settings_file_name);
	ASSERT_MSG_RETURN_NULL(engine->settings_ != nullptr, "Failed to load settings file: %s", settings_file_name.c_str());

	render::IRenderDevice::CreationDesc render_device_desc = {};
	render_device_desc.enable_debug_validation_layer = false;
	render_device_desc.frame_capture = frame_capture;

	engine->render_device_ = render::IRenderDevice::create<render::ERenderAPI::kD3D12>(render_device_desc);
	ASSERT_MSG_RETURN_NULL(engine->render_device_ != nullptr, "Failed to create render device");

	platform::IWindow::Desc window_desc = {};
	window_desc.name = engine->settings_->engine_settings.window_name;
	window_desc.mode = platform::IWindow::EMode::kWindow;
	window_desc.width = engine->settings_->engine_settings.window_width;
	window_desc.height = engine->settings_->engine_settings.window_height;

	engine->window_ = platform::IWindow::create(window_desc);
	ASSERT_MSG_RETURN_NULL(engine->window_ != nullptr, "Failed to create window");

	engine->input_ = platform::IInput::create(engine->settings_->engine_settings.data_root_path,
		engine->settings_->engine_settings.input_config_name,
		engine->window_->getHandle());
	ASSERT_MSG_RETURN_NULL(engine->input_ != nullptr, "Failed to create input");

	engine->swap_chain_ = engine->render_device_->createSwapChain(*engine->window_);
	ASSERT_MSG_RETURN_NULL(engine->swap_chain_ != nullptr, "Failed to create swap chain");

	render::RenderSettings render_settings = engine->settings_->render_settings;
	render_settings.render_width = engine->swap_chain_->getWidth();
	render_settings.render_height = engine->swap_chain_->getHeight();
	engine->renderer_ = render::IRenderer::create<render::EShadingTechnique::kDeferred>(*engine->render_device_, render_settings);
	ASSERT_MSG_RETURN_NULL(engine->renderer_ != nullptr, "Failed to create renderer");

	engine->imgui_backend_ = render::ImGuiBackend::create(*engine->render_device_);
	ASSERT_MSG_RETURN_NULL(engine->imgui_backend_ != nullptr, "Failed to initialize ImGui");
	engine->debug_menu_ = std::make_unique<engine::DebugMenu>();

	// Create default scene
	{
		std::unique_ptr<scene::ResourceManager> resource_manager = scene::ResourceManager::create();
		ASSERT_MSG_RETURN_NULL(resource_manager != nullptr, "Failed to create scene resource manager");

		std::unique_ptr<scene::Scene> default_scene = scene::SceneLoader::loadFromFile(
			engine->settings_->engine_settings.default_scene_name,
			*resource_manager,
			engine->settings_->engine_settings.data_root_path,
			engine->input_.get());
		ASSERT_MSG_RETURN_NULL(default_scene != nullptr, "Failed to create default scene");

		const float aspect = static_cast<float>(render_settings.render_width) / static_cast<float>(render_settings.render_height);
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
	if (render_device_)
	{
		render_device_->waitGpuIdle();
	}
}


Engine::Engine()
	: is_running_(false)
	, pause_on_focus_lost_(false)
{
}


void Engine::handleResize(uint32_t width, uint32_t height)
{
	ASSERT_MSG_RETURN_VOID(swap_chain_ && renderer_, "Resize requires an initialized swapchain and renderer");
	if (width == 0 || height == 0)
	{
		return;
	}

	const render::RenderSettings& render_settings = renderer_->getRenderSettings();
	if (swap_chain_->getWidth() == width && swap_chain_->getHeight() == height &&
		render_settings.render_width == width && render_settings.render_height == height)
	{
		return;
	}

	render_device_->waitGpuIdle();
	swap_chain_->resize(width, height);

	const uint32_t output_width = swap_chain_->getWidth();
	const uint32_t output_height = swap_chain_->getHeight();
	if (output_width == 0 || output_height == 0)
	{
		return;
	}

	renderer_->resize(output_width, output_height);
	if (render_settings.render_width != output_width || render_settings.render_height != output_height)
	{
		swap_chain_->resize(render_settings.render_width, render_settings.render_height);
		return;
	}

	if (current_scene_)
	{
		const float aspect = static_cast<float>(output_width) / static_cast<float>(output_height);
		current_scene_->getCamera().setAspect(aspect);
	}
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

	input_->update();
	if (input_->isKeyPressed(platform::EKey::kF11))
	{
		window_->toggleFullscreen();
		return;
	}
	if (input_->isKeyPressed(platform::EKey::kF1))
	{
		debug_menu_->toggle();
	}

	const render::RenderSettings& render_settings = renderer_->getRenderSettings();
	if (swap_chain_->getWidth() != render_settings.render_width || swap_chain_->getHeight() != render_settings.render_height)
	{
		return;
	}

	const uint32_t width = swap_chain_->getWidth();
	const uint32_t height = swap_chain_->getHeight();
	imgui_backend_->newFrame(*input_, width, height, static_cast<float>(dt));
	if (debug_menu_->isVisible())
	{
		debug_menu_->draw(width, height, pause_on_focus_lost_, *render_device_);
	}
	const bool mouse_captured = debug_menu_->isVisible() && imgui_backend_->wantsMouse();
	input_->setMouseLookEnabled(!mouse_captured);
	const bool keyboard_captured = debug_menu_->isVisible() && imgui_backend_->wantsKeyboard();
	current_scene_->update(dt, !mouse_captured && !keyboard_captured);

	render::ITexture& back_buffer = swap_chain_->getCurrentBackBuffer();
	render_device_->beginFrameCapture();
	renderer_->render(*current_scene_, current_scene_->getCamera(), back_buffer);
	imgui_backend_->draw(back_buffer);
	swap_chain_->present();
	render_device_->endFrameCapture();
}
