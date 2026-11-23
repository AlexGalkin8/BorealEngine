#pragma once

#include "buffer.h"
#include "texture.h"

#include <memory>
#include <vector>

namespace render {
class IRenderDevice;
}


namespace render {

// Owns GPU resources. Returned pointers are borrowed until release or device shutdown.
class GpuResourceManager final : public utility::NonCopyable
{
	friend class IRenderDevice;

public:
	IBuffer* createBuffer(const IBuffer::Desc& desc);
	ITexture* createTexture(const ITexture::Desc& desc);

	void releaseBuffer(IBuffer& buffer);
	void releaseTexture(ITexture& texture);

	~GpuResourceManager();

private:
	explicit GpuResourceManager(IRenderDevice& render_device);

	IRenderDevice& render_device_;
	std::vector<std::unique_ptr<IBuffer>> buffers_;
	std::vector<std::unique_ptr<ITexture>> textures_;
};

}  // namespace render
