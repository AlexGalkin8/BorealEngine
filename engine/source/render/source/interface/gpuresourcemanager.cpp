#include "interface/gpuresourcemanager.h"
#include "interface/renderdevice.h"

namespace render {

GpuResourceManager::GpuResourceManager(IRenderDevice& render_device)
	: render_device_(render_device)
{
}

GpuResourceManager::~GpuResourceManager()
{
	// The device keeps queues alive until all owned resources have been released.
	render_device_.waitGpuIdle();
}

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

}  // namespace render
