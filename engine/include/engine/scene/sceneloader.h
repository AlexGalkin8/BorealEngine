#pragma once

#include <memory>
#include <string>

namespace scene {
class Scene;
}

namespace scene {

class SceneLoader
{
public:
    static std::shared_ptr<Scene> loadFromFile(const std::string& scene_file_path, const std::string& data_root_path = "");
};

} // namespace scene
