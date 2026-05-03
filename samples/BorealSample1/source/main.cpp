#include <engine/engine.h>

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
	std::unique_ptr<Engine> engine = Engine::create("data/settings.json", getCommandLineArgs());
	if (engine)
	{
		engine->run();
	}
	return 0;
}
#else
#error "The main() function is not implemented for the current platform."
#endif // WIN32
