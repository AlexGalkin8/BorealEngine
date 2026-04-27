#pragma once

#include "resourcemanager.hpp"

#include <memory>


namespace render {

class IRenderDevice;
class D3D12RenderDevice;


class D3D12ResourceManager final : public IResourceManager
{
	friend class IResourceManager;

public:
	static std::unique_ptr<D3D12ResourceManager> create(IRenderDevice& render_device);

	ITexture* createTexture(const ITexture::Desc& desc) override;
	IBuffer* createBuffer(const IBuffer::Desc& desc) override;

	~D3D12ResourceManager() override;

private:
	D3D12ResourceManager(IRenderDevice& render_device);

	D3D12RenderDevice& getRenderDevice();
};

}  // namespace render
