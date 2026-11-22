#include <engine/core/utility/commandlinehandler.h>
#include <engine/core/utility/log.h>
#include <engine/core/utility/utility.h>
#include <engine/engine.h>

#include <cstdlib>
#include <memory>

#include <windows.h>

#ifdef WIN32
static std::vector<std::wstring> getCommandLineArgs()
{
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	std::vector<std::wstring> result;
	if (!argv)
	{
		return result;
	}

	result.reserve(static_cast<size_t>(argc));
	for (int i = 0; i < argc; ++i)
	{
		result.emplace_back(argv[i]);
	}

	LocalFree(argv);
	return result;
}


// Window main
_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*pCmdLine*/, int /*nCmdShow*/)
{
	utility::CommandLineHandler command_line(getCommandLineArgs());
	std::wstring settings_file_path = L"data/settings.json";
	const utility::ECommandLineOptionResult settings_option = command_line.consumeOption(L"--settings", settings_file_path);
	if (settings_option == utility::ECommandLineOptionResult::kMissingValue)
	{
		LOG_ERROR("Startup", "--settings requires a file path.");
		return EXIT_FAILURE;
	}
	if (settings_option == utility::ECommandLineOptionResult::kDuplicate)
	{
		LOG_ERROR("Startup", "--settings may be passed only once.");
		return EXIT_FAILURE;
	}

	std::unique_ptr<Engine> engine = Engine::create(utility::translateWStringToString(settings_file_path));
	if (engine)
	{
		engine->run();
	}
	return 0;
}
#else
#error "The main() function is not implemented for the current platform."
#endif // WIN32
