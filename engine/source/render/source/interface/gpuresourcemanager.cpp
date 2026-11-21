#include "interface/gpuresourcemanager.h"
#include "interface/renderdevice.h"

#include <algorithm>


namespace render {

IBuffer* GpuResourceManager::createBuffer(const IBuffer::Desc& desc)
{
	auto buffer = render_device_.createBuffer(desc);
	if (!buffer) return nullptr;
	auto* result = buffer.get();
	buffers_.push_back(std::move(buffer));
	return result;
}


ITexture* GpuResourceManager::createTexture(const ITexture::Desc& desc)
{
	auto texture = render_device_.createTexture(desc);
	if (!texture) return nullptr;
	auto* result = texture.get();
	textures_.push_back(std::move(texture));
	return result;
}


void GpuResourceManager::releaseTexture(ITexture& texture)
{
	const auto it = std::find_if(textures_.begin(), textures_.end(),
		[&texture](const std::unique_ptr<ITexture>& owned) { return owned.get() == &texture; });
	ASSERT_MSG_RETURN_VOID(it != textures_.end(), "Texture is not owned by this manager");
	textures_.erase(it);
}


GpuResourceManager::~GpuResourceManager()
{
	render_device_.waitGpuIdle();
}


GpuResourceManager::GpuResourceManager(IRenderDevice& render_device)
	: render_device_(render_device)
{
}

}  // namespace render
