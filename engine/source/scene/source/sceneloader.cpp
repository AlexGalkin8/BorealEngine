#include "engine/scene/sceneloader.h"
#include "engine/scene/scene.h"
#include "engine/scene/camera.h"

#include <engine/math/math_utils.h>

#include <fstream>
#include <sstream>
#include <string>

namespace scene {

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


static std::shared_ptr<Scene> loadSceneFromJsonText(std::string_view json_text, const std::string& scene_file_path, const std::string& data_root_path)
{
    (void)scene_file_path;
    (void)data_root_path;

    std::string scene_name = extractStringValue(json_text, "name");
    if (scene_name.empty())
    {
        scene_name = "scene";
    }

    auto scene = std::make_shared<Scene>(scene_name);
    scene->getCamera().setLookAt(
        math::Vector3(0.0f, 0.0f, -2.0f),
        math::Vector3(0.0f, 0.0f, 0.0f),
        math::Vector3(0.0f, 1.0f, 0.0f));
    scene->getCamera().setPerspective(math::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);

    return scene;
}


static std::string loadSceneFileText(const std::string& scene_file_path)
{
    return readTextFile(scene_file_path);
}


std::shared_ptr<Scene> SceneLoader::loadFromFile(const std::string& scene_file_path, const std::string& data_root_path)
{
    const std::string json_text = loadSceneFileText(scene_file_path);
    if (json_text.empty())
    {
        return nullptr;
    }

    return loadSceneFromJsonText(json_text, scene_file_path, data_root_path);
}

}  // namespace scene
