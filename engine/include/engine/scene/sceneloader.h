#pragma once

#include <memory>
#include <string>


namespace platform {
class IInput;
}  // namespace platform

namespace scene {
class Scene;
class ResourceManager;
}


namespace scene {

class SceneLoader
{
public:
	static std::unique_ptr<Scene> loadFromFile(const std::string& scene_name, const std::string& data_root_path = "");
	static std::unique_ptr<Scene> loadFromFile(const std::string& scene_name, ResourceManager& resource_manager, const std::string& data_root_path, platform::IInput* input = nullptr);
	static void populateSceneActorsFromGltf(Scene& scene, const std::string& gltf_file_path);
	static void populateSceneActorsFromGltf(Scene& scene, const std::string& gltf_file_path, ResourceManager& resource_manager);
};

} // namespace scene
