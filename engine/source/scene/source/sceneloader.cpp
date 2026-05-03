#include "engine/scene/sceneloader.h"
#include "engine/scene/resourcemanager.h"
#include "engine/scene/common.h"
#include "engine/scene/scene.h"
#include "engine/scene/camera.h"

#include "actor.hpp"

#include <engine/math/math_utils.h>
#include <engine/core/asset/gltfloader.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>


namespace scene {

struct SceneModelEntry
{
	std::string model_name;
	math::Vector3 position = math::Vector3(0.0f, 0.0f, 0.0f);
	math::Vector3 scale = math::Vector3(1.0f, 1.0f, 1.0f);
	math::Vector3 rotation_euler_degrees = math::Vector3(0.0f, 0.0f, 0.0f);
};


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


static std::string resolveSceneFilePath(const std::string& scene_name, const std::string& data_root_path)
{
	if (scene_name.empty())
	{
		return {};
	}

	const std::string root = data_root_path.empty() ? kDataDirectory : data_root_path;
	return root + "/" + kScenesDirectory + "/" + scene_name + "/" + scene_name + ".json";
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


static std::string extractObjectValue(std::string_view text, std::string_view key)
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

	size_t object_begin = text.find('{', colon_pos + 1);
	if (object_begin == std::string_view::npos)
	{
		return {};
	}

	int depth = 0;
	size_t object_end = std::string_view::npos;
	for (size_t i = object_begin; i < text.size(); ++i)
	{
		if (text[i] == '{')
		{
			++depth;
		}
		else if (text[i] == '}')
		{
			--depth;
			if (depth == 0)
			{
				object_end = i;
				break;
			}
		}
	}

	if (object_end == std::string_view::npos || object_end <= object_begin)
	{
		return {};
	}

	return std::string(text.substr(object_begin, object_end - object_begin + 1));
}


static std::vector<std::string> extractObjectsArray(std::string_view text, std::string_view key)
{
	std::vector<std::string> result;

	const std::string pattern = std::string("\"") + std::string(key) + "\"";
	const size_t key_pos = text.find(pattern);
	if (key_pos == std::string_view::npos)
	{
		return result;
	}

	const size_t colon_pos = text.find(':', key_pos + pattern.size());
	if (colon_pos == std::string_view::npos)
	{
		return result;
	}

	size_t array_begin = text.find('[', colon_pos + 1);
	if (array_begin == std::string_view::npos)
	{
		return result;
	}

	int array_depth = 0;
	size_t array_end = std::string_view::npos;
	for (size_t i = array_begin; i < text.size(); ++i)
	{
		if (text[i] == '[')
		{
			++array_depth;
		}
		else if (text[i] == ']')
		{
			--array_depth;
			if (array_depth == 0)
			{
				array_end = i;
				break;
			}
		}
	}

	if (array_end == std::string_view::npos)
	{
		return result;
	}

	int object_depth = 0;
	size_t object_begin = std::string_view::npos;
	for (size_t i = array_begin + 1; i < array_end; ++i)
	{
		if (text[i] == '{')
		{
			if (object_depth == 0)
			{
				object_begin = i;
			}
			++object_depth;
		}
		else if (text[i] == '}')
		{
			--object_depth;
			if (object_depth == 0 && object_begin != std::string_view::npos)
			{
				result.emplace_back(text.substr(object_begin, i - object_begin + 1));
				object_begin = std::string_view::npos;
			}
		}
	}

	return result;
}


static bool extractNumberValue(std::string_view text, std::string_view key, float& value)
{
	const std::string pattern = std::string("\"") + std::string(key) + "\"";
	const size_t key_pos = text.find(pattern);
	if (key_pos == std::string_view::npos)
	{
		return false;
	}

	size_t colon_pos = text.find(':', key_pos + pattern.size());
	if (colon_pos == std::string_view::npos)
	{
		return false;
	}

	size_t number_begin = colon_pos + 1;
	while (number_begin < text.size() && std::isspace(static_cast<unsigned char>(text[number_begin])) != 0)
	{
		++number_begin;
	}

	if (number_begin >= text.size())
	{
		return false;
	}

	size_t number_end = number_begin;
	while (number_end < text.size())
	{
		const char c = text[number_end];
		if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E')
		{
			++number_end;
			continue;
		}
		break;
	}

	if (number_end <= number_begin)
	{
		return false;
	}

	try
	{
		value = std::stof(std::string(text.substr(number_begin, number_end - number_begin)));
	}
	catch (...)
	{
		return false;
	}

	return true;
}


static bool extractVector3Value(std::string_view text, std::string_view key, math::Vector3& out_value)
{
	const std::string pattern = std::string("\"") + std::string(key) + "\"";
	const size_t key_pos = text.find(pattern);
	if (key_pos == std::string_view::npos)
	{
		return false;
	}

	size_t colon_pos = text.find(':', key_pos + pattern.size());
	if (colon_pos == std::string_view::npos)
	{
		return false;
	}

	size_t array_begin = text.find('[', colon_pos + 1);
	if (array_begin == std::string_view::npos)
	{
		return false;
	}

	size_t array_end = text.find(']', array_begin + 1);
	if (array_end == std::string_view::npos)
	{
		return false;
	}

	std::string array_text = std::string(text.substr(array_begin + 1, array_end - array_begin - 1));
	std::replace(array_text.begin(), array_text.end(), ',', ' ');
	std::istringstream stream(array_text);
	float x = 0.0f;
	float y = 0.0f;
	float z = 0.0f;
	if (!(stream >> x >> y >> z))
	{
		return false;
	}

	out_value = math::Vector3(x, y, z);
	return true;
}


static std::string getDirectoryFromPath(const std::string& file_path)
{
	const size_t slash = file_path.find_last_of("/\\");
	if (slash == std::string::npos)
	{
		return {};
	}
	return file_path.substr(0, slash);
}


static std::string resolvePathRelativeToDirectory(const std::string& relative_or_absolute_path, const std::string& directory)
{
	if (relative_or_absolute_path.empty())
	{
		return relative_or_absolute_path;
	}

	if (relative_or_absolute_path.size() >= 2 && relative_or_absolute_path[1] == ':')
	{
		return relative_or_absolute_path;
	}

	if (relative_or_absolute_path[0] == '/' || relative_or_absolute_path[0] == '\\')
	{
		return relative_or_absolute_path;
	}

	if (directory.empty())
	{
		return relative_or_absolute_path;
	}

	return directory + "/" + relative_or_absolute_path;
}


static std::string resolveModelPath(const std::string& scene_path, const std::string& model_name, const std::string& data_root_path)
{
	if (model_name.empty())
	{
		return {};
	}

	const std::string root = data_root_path.empty() ? "data" : data_root_path;
	if (scene_path.empty())
	{
		return root + "/" + kScenesDirectory + "/" + kModelsDirectory + "/" + model_name;
	}

	return root + "/" + kScenesDirectory + "/" + scene_path + "/" + kModelsDirectory + "/" + model_name;
}


static std::vector<SceneModelEntry> extractModelEntries(std::string_view json_text)
{
	std::vector<SceneModelEntry> entries;
	const std::vector<std::string> model_objects = extractObjectsArray(json_text, "models");
	for (const std::string& model_object : model_objects)
	{
		SceneModelEntry entry;
		entry.model_name = extractStringValue(model_object, "name");

		extractVector3Value(model_object, "position", entry.position);
		extractVector3Value(model_object, "scale", entry.scale);
		extractVector3Value(model_object, "rotation_euler_degrees", entry.rotation_euler_degrees);

		if (!entry.model_name.empty())
		{
			entries.push_back(entry);
		}
	}

	return entries;
}


static void addActorsFromLoadedModel(
	Scene& scene,
	const asset::LoadedModelData& loaded_model,
	const std::string& resource_path,
	const math::Transform& base_transform)
{
	auto actor = std::make_shared<StaticMeshActor>("gltf_actor");
	actor->getTransform() = base_transform;

	scene::MeshRendererComponent& mesh_renderer = actor->getMeshRenderer();
	mesh_renderer.clearMeshSlots();

	for (const asset::LoadedPrimitiveData& primitive_data : loaded_model.primitives)
	{
		scene::MeshSlot& slot = mesh_renderer.addMeshSlot();
		slot.mesh.resource_path = resource_path;
		slot.mesh.primitive = EMeshPrimitive::kTriangle;
		slot.mesh.mesh_index = primitive_data.mesh_index;
		slot.mesh.primitive_index = primitive_data.primitive_index;

		if (primitive_data.material_index >= 0 && static_cast<size_t>(primitive_data.material_index) < loaded_model.materials.size())
		{
			const asset::LoadedMaterialData& material_data = loaded_model.materials[static_cast<size_t>(primitive_data.material_index)];
			if (!material_data.name.empty())
			{
				slot.material.name = material_data.name;
			}
			else
			{
				slot.material.name = "gltf_material_" + std::to_string(primitive_data.material_index);
			}
		}
		else
		{
			slot.material.name = "default";
		}
	}

	if (!mesh_renderer.hasMeshSlots())
	{
		scene::MeshSlot& slot = mesh_renderer.addMeshSlot();
		slot.mesh.resource_path = resource_path;
		slot.mesh.primitive = EMeshPrimitive::kTriangle;
		slot.material.name = "default";
	}

	scene.addActor(actor);
}


static std::unique_ptr<Scene> loadSceneFromJsonText(
	std::string_view json_text,
	const std::string& scene_file_path,
	const std::string& data_root_path,
	ResourceManager& resource_manager)
{
	std::string scene_name = extractStringValue(json_text, "name");
	if (scene_name.empty())
	{
		scene_name = "scene";
	}

	auto scene = std::make_unique<Scene>(scene_name);

	math::Vector3 camera_eye(0.0f, 0.0f, -2.0f);
	math::Vector3 camera_target(0.0f, 0.0f, 0.0f);
	math::Vector3 camera_up(0.0f, 1.0f, 0.0f);
	float camera_fov_degrees = 60.0f;
	float camera_aspect = 16.0f / 9.0f;
	float camera_near = 0.1f;
	float camera_far = 100.0f;

	const std::string camera_object_text = extractObjectValue(json_text, "camera");
	if (!camera_object_text.empty())
	{
		extractVector3Value(camera_object_text, "eye", camera_eye);
		extractVector3Value(camera_object_text, "target", camera_target);
		extractVector3Value(camera_object_text, "up", camera_up);
		extractNumberValue(camera_object_text, "fov_degrees", camera_fov_degrees);
		extractNumberValue(camera_object_text, "aspect", camera_aspect);
		extractNumberValue(camera_object_text, "near", camera_near);
		extractNumberValue(camera_object_text, "far", camera_far);
	}

	scene->getCamera().setLookAt(camera_eye, camera_target, camera_up);
	scene->getCamera().setPerspective(math::radians(camera_fov_degrees), camera_aspect, camera_near, camera_far);

	const std::string scene_path = extractStringValue(json_text, "scene_path");
	std::vector<SceneModelEntry> model_entries = extractModelEntries(json_text);

	if (model_entries.empty())
	{
		const std::string model_name = extractStringValue(json_text, "model_name");
		const std::string model_path = extractStringValue(json_text, "model_path");
		const std::string legacy_model = extractStringValue(json_text, "model");

		SceneModelEntry entry;
		if (!model_name.empty())
		{
			entry.model_name = model_name;
		}
		else
		{
			entry.model_name = !model_path.empty() ? model_path : legacy_model;
		}

		if (!entry.model_name.empty())
		{
			model_entries.push_back(entry);
		}
	}

	for (const SceneModelEntry& model_entry : model_entries)
	{
		const std::string resolved_model_path = resolveModelPath(scene_path, model_entry.model_name, data_root_path);
		const std::string resolved_model_name = model_entry.model_name;
		const asset::LoadedModelData& loaded_model = resource_manager.getOrLoadModel(resolved_model_name, resolved_model_path);

		math::Transform model_transform = math::Transform::identity();
		model_transform.position = model_entry.position;
		model_transform.scale = model_entry.scale;
		model_transform.rotation = math::Quaternion::fromEuler(
			math::radians(model_entry.rotation_euler_degrees.x),
			math::radians(model_entry.rotation_euler_degrees.y),
			math::radians(model_entry.rotation_euler_degrees.z));

		addActorsFromLoadedModel(*scene, loaded_model, resolved_model_path, model_transform);
	}

	if (scene->getStaticMeshActors().empty())
	{
		auto actor = std::make_shared<RotatingActor>("main_actor", math::Vector3(0.0f, 1.0f, 0.0f));
		scene->addActor(actor);
	}

	return scene;
}


std::unique_ptr<Scene> SceneLoader::loadFromFile(const std::string& scene_name, const std::string& data_root_path)
{
	auto resource_manager = ResourceManager::create();
	return loadFromFile(scene_name, *resource_manager, data_root_path);
}


std::unique_ptr<Scene> SceneLoader::loadFromFile(const std::string& scene_name, ResourceManager& resource_manager, const std::string& data_root_path)
{
	const std::string scene_file_path = resolveSceneFilePath(scene_name, data_root_path);
	if (scene_file_path.empty())
	{
		return nullptr;
	}

	const std::string json_text = readTextFile(scene_file_path);
	if (json_text.empty())
	{
		return nullptr;
	}

	return loadSceneFromJsonText(json_text, scene_file_path, data_root_path, resource_manager);
}


void SceneLoader::populateSceneActorsFromGltf(Scene& scene, const std::string& gltf_file_path)
{
	auto resource_manager = ResourceManager::create();
	populateSceneActorsFromGltf(scene, gltf_file_path, *resource_manager);
}


void SceneLoader::populateSceneActorsFromGltf(Scene& scene, const std::string& gltf_file_path, ResourceManager& resource_manager)
{
	const asset::LoadedModelData& loaded_model = resource_manager.getOrLoadModel(gltf_file_path, gltf_file_path);
	addActorsFromLoadedModel(scene, loaded_model, gltf_file_path, math::Transform::identity());
}

}  // namespace scene
