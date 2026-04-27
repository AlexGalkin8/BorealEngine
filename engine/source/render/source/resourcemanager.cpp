#include "resourcemanager.hpp"
#include "interface/d3d12/d3d12resourcemanager.hpp"


namespace render {

template<ERenderAPI>
std::unique_ptr<IResourceManager> IResourceManager::create(IRenderDevice& render_device) = delete;


template<>
std::unique_ptr<IResourceManager> IResourceManager::create<ERenderAPI::kD3D12>(IRenderDevice& render_device)
{
	return D3D12ResourceManager::create(render_device);
}


IResourceManager::IResourceManager(IRenderDevice& render_device)
	: render_device_(render_device)
{
}


IResourceManager::~IResourceManager()
{
}

}  // namespace render
