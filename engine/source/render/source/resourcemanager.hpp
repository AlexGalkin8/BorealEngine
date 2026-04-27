#pragma once

#include "interface/common.h"
#include "interface/texture.h"
#include "interface/buffer.h"

#include <unordered_set>
#include <memory>


namespace render {

class IRenderDevice;


class IResourceManager : public utility::NonCopyable
{
public:
	template<ERenderAPI render_api>
	static std::unique_ptr<IResourceManager> create(IRenderDevice& render_device);

	virtual ITexture* createTexture(const ITexture::Desc& desc) = 0;
	virtual IBuffer* createBuffer(const IBuffer::Desc& desc) = 0;

	virtual ~IResourceManager();

protected:
	IResourceManager(IRenderDevice& render_device);

	IRenderDevice& getRenderDevice() { return render_device_; };

protected:
	std::unordered_set<internal::IResource*> resources_;
	std::unordered_set<ITexture*> textures_;
	std::unordered_set<IBuffer*> buffers_;

private:
	IRenderDevice& render_device_;
};

}  // namespace render
