#include <engine/scene/resourcemanager.h>

#include <unordered_map>


namespace scene {

class ResourceManager::Impl
{
public:
	const asset::LoadedModelData& getOrLoadModel(const std::string& model_name, const std::string& model_path)
	{
		(const std::string&)model_name;

		auto it = models_by_path_.find(model_path);
		if (it != models_by_path_.end()) {
			return it->second;
		}

		auto inserted = models_by_path_.emplace(model_path, asset::loadModelFromGltf(model_path));
		return inserted.first->second;
	}

private:
	std::unordered_map<std::string, asset::LoadedModelData> models_by_path_;
};

std::unique_ptr<ResourceManager> ResourceManager::create()
{
	return std::unique_ptr<ResourceManager>(new ResourceManager());
}

const asset::LoadedModelData& ResourceManager::getOrLoadModel(const std::string& model_name, const std::string& model_path)
{
	return impl_->getOrLoadModel(model_name, model_path);
}

ResourceManager::ResourceManager()
	: impl_(std::make_unique<Impl>())
{
}

ResourceManager::~ResourceManager() = default;

} // namespace scene
