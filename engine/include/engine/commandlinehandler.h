#pragma once

#include <engine/render/interface/gpuframecapturer.h>

#include <string>
#include <string_view>
#include <vector>


namespace engine {

enum class ECommandLineOptionResult
{
	kAbsent,
	kFound,
	kMissingValue,
	kDuplicate,
};


class CommandLineHandler
{
public:
	struct StartupOptions
	{
		std::wstring settings_file_path = L"data/settings.json";
		render::FrameCaptureSettings frame_capture;
	};


	static CommandLineHandler fromProcess();

	explicit CommandLineHandler(std::vector<std::wstring> argv);

	const std::wstring& getExecutableName() const { return executable_name_; }
	const std::vector<std::wstring>& getRemainingArguments() const { return arguments_; }

	bool handleCommandLine(StartupOptions& options) const;
	bool consumeFlag(std::wstring_view flag);
	ECommandLineOptionResult consumeOption(std::wstring_view option, std::wstring& value);

private:
	std::wstring executable_name_;
	std::vector<std::wstring> arguments_;
};

}  // namespace engine
