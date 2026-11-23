#include "commandlinehandler.h"

#include <engine/core/utility/log.h>
#include <engine/core/utility/utility.h>

#include <algorithm>
#include <iterator>
#include <utility>

#include <windows.h>
#include <shellapi.h>


namespace engine {

CommandLineHandler CommandLineHandler::fromProcess()
{
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::wstring> result;
	if (!argv)
	{
		return CommandLineHandler(std::move(result));
	}

	result.reserve(static_cast<size_t>(argc));
	for (int i = 0; i < argc; ++i)
	{
		result.emplace_back(argv[i]);
	}

	LocalFree(argv);
	return CommandLineHandler(std::move(result));
}


CommandLineHandler::CommandLineHandler(std::vector<std::wstring> argv)
	: arguments_(std::move(argv))
{
	if (!arguments_.empty())
	{
		executable_name_ = std::move(arguments_.front());
		arguments_.erase(arguments_.begin());
	}
}


static bool parseCaptureToolOptions(const std::vector<std::wstring>& arguments, render::FrameCaptureSettings& options)
{
	const auto tool = std::find(arguments.begin(), arguments.end(), L"--tool");
	const auto options_begin = std::next(tool, 2);
	const auto options_end = std::find_if(options_begin, arguments.end(), [](const std::wstring& argument)
	{
		return argument.starts_with(L"--");
	});
	for (auto option = options_begin; option != options_end; ++option)
	{
		if (*option == L"tool-path")
		{
			if (!options.tool_path.empty() || ++option == options_end || option->empty() ||
				*option == L"tool-path" || *option == L"open-capture-tool")
			{
				LOG_ERROR("Startup", "tool-path expects one path to the selected tool library.");
				return false;
			}
			options.tool_path = *option;
		}
		else if (*option == L"open-capture-tool")
		{
			if (options.open_mode != render::ECaptureToolOpenMode::kNone ||
				++option == options_end || (*option != L"live" && *option != L"capture"))
			{
				LOG_ERROR("Startup", "open-capture-tool expects one value: live or capture.");
				return false;
			}
			options.open_mode = *option == L"live" ? render::ECaptureToolOpenMode::kLive : render::ECaptureToolOpenMode::kCapture;
		}
		else
		{
			LOG_ERROR("Startup", "Unknown capture tool parameter: {}.", utility::translateWStringToString(*option));
			return false;
		}
	}
	return true;
}


bool CommandLineHandler::handleCommandLine(StartupOptions& options) const
{
	CommandLineHandler command_line = *this;
	const ECommandLineOptionResult settings_option = command_line.consumeOption(L"--settings", options.settings_file_path);
	if (settings_option == ECommandLineOptionResult::kMissingValue)
	{
		LOG_ERROR("Startup", "--settings requires a file path.");
		return false;
	}
	if (settings_option == ECommandLineOptionResult::kDuplicate)
	{
		LOG_ERROR("Startup", "--settings may be passed only once.");
		return false;
	}

	std::wstring tool_name;
	const ECommandLineOptionResult tool_option = command_line.consumeOption(L"--tool", tool_name);
	if (tool_option != ECommandLineOptionResult::kAbsent &&
		(tool_option != ECommandLineOptionResult::kFound || tool_name != L"renderdoc"))
	{
		LOG_ERROR("Startup", "--tool expects one supported capture tool: renderdoc.");
		return false;
	}

	if (tool_option == ECommandLineOptionResult::kAbsent)
	{
		return true;
	}

	options.frame_capture.tool = render::ECaptureTool::kRenderDoc;
	return parseCaptureToolOptions(arguments_, options.frame_capture);
}


bool CommandLineHandler::consumeFlag(std::wstring_view flag)
{
	const std::wstring flag_name(flag);
	const auto options_end = std::find(arguments_.begin(), arguments_.end(), L"--");
	const auto new_end = std::remove(arguments_.begin(), options_end, flag_name);
	if (new_end == options_end)
	{
		return false;
	}

	arguments_.erase(new_end, options_end);
	return true;
}


ECommandLineOptionResult CommandLineHandler::consumeOption(std::wstring_view option, std::wstring& value)
{
	const auto options_end = std::find(arguments_.begin(), arguments_.end(), L"--");
	const auto option_it = std::find(arguments_.begin(), options_end, option);
	if (option_it == options_end)
	{
		return ECommandLineOptionResult::kAbsent;
	}

	const auto value_it = std::next(option_it);
	if (value_it == options_end || value_it->empty() || value_it->starts_with(L"--"))
	{
		return ECommandLineOptionResult::kMissingValue;
	}
	if (std::find(std::next(value_it), options_end, option) != options_end)
	{
		return ECommandLineOptionResult::kDuplicate;
	}

	value = std::move(*value_it);
	arguments_.erase(option_it, std::next(value_it));
	return ECommandLineOptionResult::kFound;
}

}  // namespace engine
