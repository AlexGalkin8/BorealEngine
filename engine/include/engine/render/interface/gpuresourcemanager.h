#pragma once

#include "buffer.h"
#include "texture.h"

#include <memory>
#include <vector>

namespace render {

class IRenderDevice;

// Owns GPU resources for the lifetime of the device. Returned pointers are borrowed.
// Individual eviction/retirement is deliberately not exposed yet.
class GpuResourceManager final : public utility::NonCopyable
{
	friend class IRenderDevice;

public:
	IBuffer* createBuffer(const IBuffer::Desc& desc);
	ITexture* createTexture(const ITexture::Desc& desc);
	~GpuResourceManager();

private:
	explicit GpuResourceManager(IRenderDevice& render_device);

	IRenderDevice& render_device_;
	std::vector<std::unique_ptr<IBuffer>> buffers_;
	std::vector<std::unique_ptr<ITexture>> textures_;
};

}  // namespace render
