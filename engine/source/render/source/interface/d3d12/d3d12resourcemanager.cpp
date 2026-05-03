#include "d3d12resourcemanager.hpp"
#include "d3d12common.hpp"
#include "d3d12renderdevice.hpp"
#include "d3d12texture.hpp"
#include "d3d12buffer.hpp"


namespace render {

ITexture* D3D12ResourceManager::createTexture(const ITexture::Desc& desc)
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

	if (desc.format == ETextureFormat::D16_Unorm || desc.format == ETextureFormat::D24_Unorm_S8_Uint || desc.format == ETextureFormat::D32_Float)
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
		resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	}

	D3D12_ASSERT_RETURN_NULL(getRenderDevice().getDevice()->CreateCommittedResource(
		&HeapProps, D3D12_HEAP_FLAG_NONE, &resource_desc, initial_state, clear_value_ptr, IID_PPV_ARGS(&resource)));

	D3D12Texture* texture = new D3D12Texture(desc, resource.Detach());
	resources_.insert(texture);
	textures_.insert(texture);

	return texture;
}


IBuffer* D3D12ResourceManager::createBuffer(const IBuffer::Desc& desc)
{
	Microsoft::WRL::ComPtr<ID3D12Resource2> resource;

	D3D12_HEAP_PROPERTIES heap_props = {};
	heap_props.Type = D3D12_HEAP_TYPE_UPLOAD; // для простого треугольника — upload heap

	D3D12_RESOURCE_DESC resource_desc = {};
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resource_desc.Width = desc.size;
	resource_desc.Height = 1;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 1;
	resource_desc.Format = DXGI_FORMAT_UNKNOWN;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_ASSERT_RETURN_NULL(getRenderDevice().getDevice()->CreateCommittedResource(
		&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)));

	D3D12Buffer* buffer = new D3D12Buffer(desc, resource.Detach());
	resources_.insert(buffer);
	buffers_.insert(buffer);

	return buffer;
}


std::unique_ptr<D3D12ResourceManager> D3D12ResourceManager::create(IRenderDevice& render_device)
{
	return std::unique_ptr<D3D12ResourceManager>(new D3D12ResourceManager(render_device));
}


D3D12RenderDevice& D3D12ResourceManager::getRenderDevice()
{
	return static_cast<D3D12RenderDevice&>(IResourceManager::getRenderDevice());
}


D3D12ResourceManager::D3D12ResourceManager(IRenderDevice& render_device)
	: IResourceManager(render_device)
{
}


D3D12ResourceManager::~D3D12ResourceManager()
{
}

}  // namespace render
