#pragma once

#include <engine/core/asset/gltfloader.h>

#include <memory>
#include <string>

namespace scene {

class ResourceManager
{
public:
    static std::unique_ptr<ResourceManager> create();

    const asset::LoadedModelData& getOrLoadModel(const std::string& model_name, const std::string& model_path);

    ~ResourceManager();

private:
    ResourceManager();

    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace scene
