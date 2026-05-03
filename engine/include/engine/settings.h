#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace engine {

struct Settings
{
	std::string data_root_path = "data";
	std::string default_scene_name = "default_scene";
	std::wstring window_name = L"BorealEngine";
	uint32_t window_width = 1280;
	uint32_t window_height = 720;
};

extern std::unique_ptr<Settings> loadSettingsFromFile(const std::string& settings_file_path);

} // namespace engine
