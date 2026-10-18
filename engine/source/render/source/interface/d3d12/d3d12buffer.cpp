#include "d3d12buffer.hpp"
#include "d3d12common.hpp"
#include "d3d12renderdevice.hpp"


namespace render {

std::unique_ptr<D3D12Buffer> D3D12Buffer::create(D3D12RenderDevice& render_device, const IBuffer::Desc& desc)
{
	const bool is_cpu_writable =
		(desc.usage & EBufferUsage::kConstant) != EBufferUsage(0) ||
		(desc.usage & EBufferUsage::kVertex) != EBufferUsage(0) ||
		(desc.usage & EBufferUsage::kIndex) != EBufferUsage(0);

	D3D12_HEAP_PROPERTIES heap_props = {};
	heap_props.Type = is_cpu_writable ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;

	D3D12_RESOURCE_FLAGS resource_flags = D3D12_RESOURCE_FLAG_NONE;
	if ((desc.usage & EBufferUsage::kUnorderedAccess) != EBufferUsage(0))
		resource_flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_RESOURCE_DESC resource_desc = {};
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resource_desc.Width = desc.size;
	resource_desc.Height = 1;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.MipLevels = 1;
	resource_desc.Format = DXGI_FORMAT_UNKNOWN;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource_desc.Flags = resource_flags;

	const D3D12_RESOURCE_STATES initial_state = is_cpu_writable
		? D3D12_RESOURCE_STATE_GENERIC_READ
		: D3D12_RESOURCE_STATE_COMMON;

	Microsoft::WRL::ComPtr<ID3D12Resource2> resource;
	D3D12_ASSERT_RETURN_NULL(render_device.getD3D12Device()->CreateCommittedResource(
		&heap_props, D3D12_HEAP_FLAG_NONE, &resource_desc,
		initial_state, nullptr, IID_PPV_ARGS(&resource)));

	D3D12OwnedDescriptor srv_descriptor;
	D3D12OwnedDescriptor cbv_descriptor;
	D3D12OwnedDescriptor uav_descriptor;

	// SRV — structured buffer
	if ((desc.usage & EBufferUsage::kStructured) != EBufferUsage(0))
	{
		ASSERT_MSG_RETURN_NULL(desc.stride > 0,
			"D3D12Buffer: stride must be > 0 for structured buffer");

		auto alloc = render_device.getCbvSrvUavCpuDescriptorHeap()->allocate();
		ASSERT_MSG_RETURN_NULL(alloc.isValid(), "D3D12Buffer: failed to allocate SRV");

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Format = DXGI_FORMAT_UNKNOWN;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = static_cast<UINT>(desc.size / desc.stride);
		srv_desc.Buffer.StructureByteStride = desc.stride;
		srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		render_device.getD3D12Device()->CreateShaderResourceView(
			resource.Get(), &srv_desc, alloc.cpu);
		srv_descriptor.reset(render_device.getCbvSrvUavCpuDescriptorHeap(), alloc);
	}
	// SRV — raw byte address buffer
	else if ((desc.usage & EBufferUsage::kRaw) != EBufferUsage(0))
	{
		auto alloc = render_device.getCbvSrvUavCpuDescriptorHeap()->allocate();
		ASSERT_MSG_RETURN_NULL(alloc.isValid(), "D3D12Buffer: failed to allocate raw SRV");

		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
		srv_desc.Format = DXGI_FORMAT_R32_TYPELESS;
		srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srv_desc.Buffer.FirstElement = 0;
		srv_desc.Buffer.NumElements = static_cast<UINT>(desc.size / 4);
		srv_desc.Buffer.StructureByteStride = 0;
		srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

		render_device.getD3D12Device()->CreateShaderResourceView(
			resource.Get(), &srv_desc, alloc.cpu);
		srv_descriptor.reset(render_device.getCbvSrvUavCpuDescriptorHeap(), alloc);
	}

	// CBV
	if ((desc.usage & EBufferUsage::kConstant) != EBufferUsage(0))
	{
		ASSERT_MSG_RETURN_NULL(desc.size % 256 == 0,
			"D3D12Buffer: constant buffer size must be aligned to 256 bytes");

		auto alloc = render_device.getCbvSrvUavCpuDescriptorHeap()->allocate();
		ASSERT_MSG_RETURN_NULL(alloc.isValid(), "D3D12Buffer: failed to allocate CBV");

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
		cbv_desc.BufferLocation = resource->GetGPUVirtualAddress();
		cbv_desc.SizeInBytes = static_cast<UINT>(desc.size);

		render_device.getD3D12Device()->CreateConstantBufferView(&cbv_desc, alloc.cpu);
		cbv_descriptor.reset(render_device.getCbvSrvUavCpuDescriptorHeap(), alloc);
	}

	// UAV
	if ((desc.usage & EBufferUsage::kUnorderedAccess) != EBufferUsage(0))
	{
		ASSERT_MSG_RETURN_NULL(desc.stride > 0,
			"D3D12Buffer: stride must be > 0 for UAV buffer");

		auto alloc = render_device.getCbvSrvUavCpuDescriptorHeap()->allocate();
		ASSERT_MSG_RETURN_NULL(alloc.isValid(), "D3D12Buffer: failed to allocate UAV");

		D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
		uav_desc.Format = DXGI_FORMAT_UNKNOWN;
		uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uav_desc.Buffer.FirstElement = 0;
		uav_desc.Buffer.NumElements = static_cast<UINT>(desc.size / desc.stride);
		uav_desc.Buffer.StructureByteStride = desc.stride;
		uav_desc.Buffer.CounterOffsetInBytes = 0;
		uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		render_device.getD3D12Device()->CreateUnorderedAccessView(
			resource.Get(), nullptr, &uav_desc, alloc.cpu);
		uav_descriptor.reset(render_device.getCbvSrvUavCpuDescriptorHeap(), alloc);
	}

	return std::unique_ptr<D3D12Buffer>(new D3D12Buffer(
		desc, resource.Detach(),
		std::move(srv_descriptor),
		std::move(cbv_descriptor),
		std::move(uav_descriptor)));
}

void D3D12Buffer::upload(const void* data, size_t size)
{
	ASSERT_MSG_RETURN_VOID(size >= desc_.size, "D3D12Buffer::upload: size exceeds buffer capacity");
	void* mapped = nullptr;
	D3D12_RANGE read_range{ 0, 0 };
	D3D12_ASSERT_RETURN_VOID(resource_->Map(0, &read_range, &mapped));
	std::memcpy(mapped, data, size);
	resource_->Unmap(0, nullptr);
}


D3D12Buffer::~D3D12Buffer()
{
	D3D12_SAFE_RELEASE(resource_);
}


D3D12Buffer::D3D12Buffer(
	const Desc& desc,
	ID3D12Resource2* resource,
	D3D12OwnedDescriptor&& srv_descriptor,
	D3D12OwnedDescriptor&& cbv_descriptor,
	D3D12OwnedDescriptor&& uav_descriptor)
	: IBuffer(desc)
	, resource_(resource)
	, srv_descriptor_(std::move(srv_descriptor))
	, cbv_descriptor_(std::move(cbv_descriptor))
	, uav_descriptor_(std::move(uav_descriptor))
{}


D3D12_CPU_DESCRIPTOR_HANDLE D3D12Buffer::getSrvHandle() const
{
	return srv_descriptor_.getCpuHandle();
}


D3D12_CPU_DESCRIPTOR_HANDLE D3D12Buffer::getCbvHandle() const
{
	return cbv_descriptor_.getCpuHandle();
}


D3D12_CPU_DESCRIPTOR_HANDLE D3D12Buffer::getUavHandle() const
{
	return uav_descriptor_.getCpuHandle();
}


void D3D12Buffer::setGpuDebugName()
{
	SET_D3D12_RESOURCE_DEBUG_NAME(resource_, getName());
}

}  // namespace render
