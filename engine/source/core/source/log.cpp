#include "utility/log.h"
#include "utility/utility.h"

#include <cstdio>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#endif


namespace logging {

static std::string_view translateLogLevelToString(ELogLevel level)
{
	switch (level)
	{
	case ELogLevel::kInfo: return "Info";
	case ELogLevel::kWarning: return "Warning";
	case ELogLevel::kError: return "Error";
	}
	return "Unknown";
}


void write(ELogLevel level, std::string_view category, std::string_view message, const std::source_location& location)
{
	const std::string_view file_path = location.file_name();
	const std::size_t separator = file_path.find_last_of("/\\");
	const std::string_view file_name = file_path.substr(separator == std::string_view::npos ? 0 : separator + 1);
	const std::string line = std::format("[BorealEngine] [{}] [{}] {} ({}:{})\n",
		translateLogLevelToString(level), category, message, file_name, location.line());
	std::fputs(line.c_str(), stderr);
	std::fflush(stderr);

#if defined(_WIN32)
	const std::wstring debug_line = utility::translateStringToWString(line);
	OutputDebugStringW(debug_line.c_str());
#endif
}

}  // namespace logging
