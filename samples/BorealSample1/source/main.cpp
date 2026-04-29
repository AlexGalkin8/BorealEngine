#include <engine/engine.h>
#include <engine/scene/scene.h>
#include <engine/scene/camera.h>
#include <engine/math/math_utils.h>

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
	Engine::CreationDesc engine_desc{};
	engine_desc.data_root_path = "data";
	engine_desc.window_name = L"BorealEngine";
	engine_desc.window_width = 1280;
	engine_desc.window_height = 720;

	std::unique_ptr<Engine> engine = Engine::create(engine_desc, getCommandLineArgs());

	std::shared_ptr<scene::Scene> default_scene = std::make_shared<scene::Scene>("default_scene");

	default_scene->getCamera().setLookAt(
		math::Vector3(0.0f, -100.7f, -10.0f),
		math::Vector3(0.0f, -100.7f, 0.0f),
		math::Vector3(0.0f, 1.0f, 0.0f));
	default_scene->getCamera().setPerspective(math::radians(90.0f), 16.0f / 9.0f, 0.1f, 500.0f);

	const auto& renderables = default_scene->getRenderableActors();
	if (!renderables.empty() && renderables[0])
	{
		auto static_mesh_actor = std::dynamic_pointer_cast<scene::StaticMeshActor>(renderables[0]);
		if (static_mesh_actor)
		{
			static_mesh_actor->getTransform().position = math::Vector3(0.0f, 0.0f, 20.0f);
			static_mesh_actor->getTransform().scale = math::Vector3(0.7f, 0.7f, 0.7f);
			static_mesh_actor->getMaterial().name = "default";
		}
	}

	engine->addScene(default_scene);
	engine->run();
	engine->removeScene(default_scene);

	return 0;
}
#else
#error "The main() function is not implemented for the current platform."
#endif // WIN32
