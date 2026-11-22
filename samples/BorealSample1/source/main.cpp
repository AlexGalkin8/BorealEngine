#include <engine/engine.h>

#include <cstdlib>
#include <memory>

#include <windows.h>


#ifdef WIN32
_Use_decl_annotations_
int WINAPI wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*pCmdLine*/, int /*nCmdShow*/)
{
	std::unique_ptr<Engine> engine = Engine::create();
	if (!engine)
	{
		return EXIT_FAILURE;
	}

	engine->run();
	return EXIT_SUCCESS;
}
#else
#error "The main() function is not implemented for the current platform."
#endif // WIN32
