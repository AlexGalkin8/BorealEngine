#pragma once

#include <engine/render/renderer.h>

#include <cstdint>
#include <memory>
#include <string>

namespace engine {

struct EngineSettings
{
	std::string data_root_path = "data";
	std::string default_scene_name = "default_scene";
	std::string input_config_name = "input";
	std::wstring window_name = L"BorealEngine";
	uint32_t window_width = 1280;
	uint32_t window_height = 720;
};

struct Settings
{
	engine::EngineSettings engine_settings;
	render::RenderSettings render_settings;
};

std::unique_ptr<Settings> loadSettingsFromFile(const std::string& settings_file_path);

} // namespace engine
