#pragma once

#include "interface/texture.h"


struct ID3D12Resource2;


namespace render {

class D3D12ResourceManager;
class D3D12SwapChain;


class D3D12Texture final : public ITexture
{
	friend class D3D12ResourceManager;
	friend class D3D12SwapChain; // for creating back buffer textures yourself

public:
	~D3D12Texture() override;

	ID3D12Resource2* getResource() { return resource_; }

private:
	D3D12Texture(const Desc& desc, ID3D12Resource2* resource);

	void setGpuDebugName() override;

private:
	ID3D12Resource2* resource_;
};

}  // namespace render
