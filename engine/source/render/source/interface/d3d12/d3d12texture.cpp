#include "d3d12texture.hpp"
#include "d3d12common.hpp"


namespace render {

D3D12Texture::D3D12Texture(const Desc& desc, ID3D12Resource2* resource)
	: ITexture(desc)
	, resource_(resource)
{
}


D3D12Texture::~D3D12Texture()
{
	D3D12_SAFE_RELEASE(resource_);
}


void D3D12Texture::setGpuDebugName()
{
	SET_D3D12_RESOURCE_DEBUG_NAME(resource_, getName());
}

}  // namespace render
