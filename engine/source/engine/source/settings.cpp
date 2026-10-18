#include <engine/settings.h>

#include <fstream>
#include <sstream>
#include <string_view>

namespace {

static std::string readTextFile(const std::string& path)
{
	std::ifstream file(path, std::ios::in | std::ios::binary);
	if (!file)
	{
		return {};
	}

	std::ostringstream stream;
	stream << file.rdbuf();
	return stream.str();
}


static std::string extractStringValue(std::string_view text, std::string_view key)
{
	const std::string pattern = std::string("\"") + std::string(key) + "\"";
	const size_t key_pos = text.find(pattern);
	if (key_pos == std::string_view::npos)
	{
		return {};
	}

	const size_t colon_pos = text.find(':', key_pos + pattern.size());
	if (colon_pos == std::string_view::npos)
	{
		return {};
	}

	const size_t first_quote = text.find('"', colon_pos + 1);
	if (first_quote == std::string_view::npos)
	{
		return {};
	}

	const size_t second_quote = text.find('"', first_quote + 1);
	if (second_quote == std::string_view::npos)
	{
		return {};
	}

	return std::string(text.substr(first_quote + 1, second_quote - first_quote - 1));
}


static bool extractUIntValue(std::string_view text, std::string_view key, uint32_t& value)
{
	const std::string pattern = std::string("\"") + std::string(key) + "\"";
	const size_t key_pos = text.find(pattern);
	if (key_pos == std::string_view::npos)
	{
		return false;
	}

	const size_t colon_pos = text.find(':', key_pos + pattern.size());
	if (colon_pos == std::string_view::npos)
	{
		return false;
	}

	size_t number_begin = colon_pos + 1;
	while (number_begin < text.size() && (text[number_begin] == ' ' || text[number_begin] == '\t' || text[number_begin] == '\n' || text[number_begin] == '\r'))
	{
		++number_begin;
	}

	size_t number_end = number_begin;
	while (number_end < text.size() && text[number_end] >= '0' && text[number_end] <= '9')
	{
		++number_end;
	}

	if (number_end <= number_begin)
	{
		return false;
	}

	try
	{
		value = static_cast<uint32_t>(std::stoul(std::string(text.substr(number_begin, number_end - number_begin))));
	}
	catch (...)
	{
		return false;
	}

	return true;
}

} // namespace


std::unique_ptr<engine::Settings> engine::loadSettingsFromFile(const std::string& settings_file_path)
{
	auto settings = std::make_unique<engine::Settings>();

	const std::string text = readTextFile(settings_file_path);
	if (text.empty())
	{
		return settings;
	}

	const std::string data_root_path = extractStringValue(text, "data_root_path");
	if (!data_root_path.empty())
	{
		settings->engine_settings.data_root_path = data_root_path;
	}

	const std::string default_scene_name = extractStringValue(text, "default_scene_name");
	if (!default_scene_name.empty())
	{
		settings->engine_settings.default_scene_name = default_scene_name;
	}

	const std::string input_config_name = extractStringValue(text, "input_config_name");
	if (!input_config_name.empty())
	{
		settings->engine_settings.input_config_name = input_config_name;
	}

	const std::string window_name_utf8 = extractStringValue(text, "window_name");
	if (!window_name_utf8.empty())
	{
		settings->engine_settings.window_name = std::wstring(window_name_utf8.begin(), window_name_utf8.end());
	}

	uint32_t width = 0;
	if (extractUIntValue(text, "window_width", width) && width > 0)
	{
		settings->engine_settings.window_width = width;
	}

	uint32_t height = 0;
	if (extractUIntValue(text, "window_height", height) && height > 0)
	{
		settings->engine_settings.window_height = height;
	}

	uint32_t render_width = 0;
	if (extractUIntValue(text, "render_width", render_width) && render_width > 0)
	{
		settings->render_settings.render_width = render_width;
	}

	uint32_t render_height = 0;
	if (extractUIntValue(text, "render_height", render_height) && render_height > 0)
	{
		settings->render_settings.render_height = render_height;
	}

	if (settings->render_settings.render_width == 0)
	{
		settings->render_settings.render_width = settings->engine_settings.window_width;
	}

	if (settings->render_settings.render_height == 0)
	{
		settings->render_settings.render_height = settings->engine_settings.window_height;
	}

	return settings;
}
