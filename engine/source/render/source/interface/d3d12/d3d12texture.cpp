#include "d3d12texture.hpp"
#include "d3d12common.hpp"
#include "d3d12renderdevice.hpp"

#include <engine/core/utility/assert.h>


namespace render {

namespace {

bool createRtvDescriptor(D3D12RenderDevice& render_device, ID3D12Resource2* resource, D3D12OwnedDescriptor& out_descriptor)
{
	ASSERT_MSG_RETURN(resource != nullptr, false, "RTV resource is null");

	D3D12DescriptorHeap::Allocation rtv_allocation = render_device.getRtvDescriptorHeap()->allocate();
	ASSERT_MSG_RETURN(rtv_allocation.isValid(), false, "Failed to allocate RTV descriptor");

	D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
	rtv_desc.Format = resource->GetDesc().Format;
	rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	rtv_desc.Texture2D.MipSlice = 0;
	rtv_desc.Texture2D.PlaneSlice = 0;

	render_device.getD3D12Device()->CreateRenderTargetView(resource, &rtv_desc, rtv_allocation.cpu);
	out_descriptor.reset(render_device.getRtvDescriptorHeap(), rtv_allocation);
	return true;
}


bool createDsvDescriptor(D3D12RenderDevice& render_device, ID3D12Resource2* resource, DXGI_FORMAT format, D3D12OwnedDescriptor& out_descriptor)
{
	ASSERT_MSG_RETURN(resource != nullptr, false, "D3D12Texture: DSV resource is null");

	D3D12DescriptorHeap::Allocation dsv_allocation = render_device.getDsvDescriptorHeap()->allocate();
	ASSERT_MSG_RETURN(dsv_allocation.isValid(), false, "D3D12Texture: failed to allocate DSV descriptor");

	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {};
	dsv_desc.Format = format;
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
	dsv_desc.Texture2D.MipSlice = 0;
	render_device.getD3D12Device()->CreateDepthStencilView(resource, &dsv_desc, dsv_allocation.cpu);

	out_descriptor.reset(render_device.getDsvDescriptorHeap(), dsv_allocation);
	return true;
}


bool createSrvDescriptor(D3D12RenderDevice& render_device, ID3D12Resource2* resource, const D3D12_RESOURCE_DESC& resource_desc, D3D12OwnedDescriptor& out_descriptor)
{
	ASSERT_MSG_RETURN(resource != nullptr, false, "D3D12Texture: SRV resource is null");

	D3D12DescriptorHeap::Allocation srv_allocation = render_device.getCbvSrvUavCpuDescriptorHeap()->allocate();
	ASSERT_MSG_RETURN(srv_allocation.isValid(), false, "D3D12Texture: failed to allocate persistent SRV descriptor");

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Format = resource_desc.Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.Texture2D.MipLevels = resource_desc.MipLevels;
	render_device.getD3D12Device()->CreateShaderResourceView(resource, &srv_desc, srv_allocation.cpu);

	out_descriptor.reset(render_device.getCbvSrvUavCpuDescriptorHeap(), srv_allocation);
	return true;
}

} // namespace

std::unique_ptr<D3D12Texture> D3D12Texture::create(D3D12RenderDevice& render_device, const ITexture::Desc& desc)
{
	Microsoft::WRL::ComPtr<ID3D12Resource2> resource;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC resource_desc;
	resource_desc.Alignment = 0;
	resource_desc.Width = static_cast<UINT64>(desc.width);
	resource_desc.Height = static_cast<UINT>(desc.height);
	resource_desc.DepthOrArraySize = static_cast<UINT16>(desc.depth);
	resource_desc.MipLevels = static_cast<UINT16>(desc.mipCount);
	resource_desc.Format = internal::translateTextureFormatToDxgiFormat(desc.format);
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	D3D12_RESOURCE_STATES initial_state = D3D12_RESOURCE_STATE_COPY_DEST;
	D3D12_CLEAR_VALUE* clear_value_ptr = nullptr;
	D3D12_CLEAR_VALUE clear_value = {};

	const bool is_depth_texture = isDepthTextureFormat(desc.format);
	if (is_depth_texture)
	{
		resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		initial_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;

		clear_value.Format = resource_desc.Format;
		clear_value.DepthStencil.Depth = 1.0f;
		clear_value.DepthStencil.Stencil = 0;
		clear_value_ptr = &clear_value;
	}
	else
	{
		resource_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}

	D3D12_ASSERT_RETURN_NULL(render_device.getD3D12Device()->CreateCommittedResource(
		&HeapProps, D3D12_HEAP_FLAG_NONE, &resource_desc, initial_state, clear_value_ptr, IID_PPV_ARGS(&resource)));

	D3D12OwnedDescriptor rtv_descriptor = {};
	D3D12OwnedDescriptor dsv_descriptor = {};
	D3D12OwnedDescriptor srv_descriptor = {};

	if (is_depth_texture)
	{
		ASSERT_MSG_RETURN_NULL(createDsvDescriptor(render_device, resource.Get(), resource_desc.Format, dsv_descriptor),
			"Failed to create DSV descriptor for depth texture");
	}
	else
	{
		ASSERT_MSG_RETURN_NULL(createRtvDescriptor(render_device, resource.Get(), rtv_descriptor), "Failed to create RTV descriptor for texture");
		ASSERT_MSG_RETURN_NULL(createSrvDescriptor(render_device, resource.Get(), resource_desc, srv_descriptor), "Failed to create SRV descriptor for texture");
	}

	return std::unique_ptr<D3D12Texture>(new D3D12Texture(
		desc,
		resource.Detach(),
		std::move(srv_descriptor),
		std::move(rtv_descriptor),
		std::move(dsv_descriptor)));
}


std::unique_ptr<D3D12Texture> D3D12Texture::createBackBuffer(D3D12RenderDevice& render_device, const ITexture::Desc& desc, ID3D12Resource2* resource)
{
	ASSERT_MSG_RETURN_NULL(resource != nullptr, "Back buffer resource is null");

	D3D12OwnedDescriptor rtv_descriptor = {};
	D3D12OwnedDescriptor dsv_descriptor = {};
	D3D12OwnedDescriptor srv_descriptor = {};

	ASSERT_MSG_RETURN_NULL(createRtvDescriptor(render_device, resource, rtv_descriptor), "Failed to create RTV descriptor for texture");
	ASSERT_MSG_RETURN_NULL(createSrvDescriptor(render_device, resource, resource->GetDesc(), srv_descriptor), "Failed to create SRV descriptor for texture");

	return std::unique_ptr<D3D12Texture>(new D3D12Texture(desc, resource, std::move(srv_descriptor), std::move(rtv_descriptor)));
}


D3D12_CPU_DESCRIPTOR_HANDLE D3D12Texture::getSrvHandle() const
{
	return srv_descriptor_.getCpuHandle();
}


D3D12_CPU_DESCRIPTOR_HANDLE D3D12Texture::getRtvHandle() const
{
	return rtv_descriptor_.getCpuHandle();
}


D3D12_CPU_DESCRIPTOR_HANDLE D3D12Texture::getDsvHandle() const
{
	ASSERT_MSG_RETURN(dsv_descriptor_.has_value(), D3D12_CPU_DESCRIPTOR_HANDLE{}, "D3D12Texture: attempted to get DSV handle from a texture that doesn't have a DSV descriptor");
	return dsv_descriptor_->getCpuHandle();
}


D3D12Texture::D3D12Texture(
	const Desc& desc,
	ID3D12Resource2* resource,
	D3D12OwnedDescriptor&& srv_descriptor,
	D3D12OwnedDescriptor&& rtv_descriptor,
	std::optional<D3D12OwnedDescriptor>&& dsv_descriptor)
	: ITexture(desc)
	, resource_(resource)
	, srv_descriptor_(std::move(srv_descriptor))
	, rtv_descriptor_(std::move(rtv_descriptor))
	, dsv_descriptor_(std::move(dsv_descriptor))
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
