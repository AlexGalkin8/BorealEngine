#include "d3d12buffer.hpp"
#include "d3d12common.hpp"



namespace render {

D3D12Buffer::D3D12Buffer(const Desc& desc, ID3D12Resource2* resource)
	: IBuffer(desc)
	, resource_(resource)
{
}


D3D12Buffer::~D3D12Buffer()
{
	D3D12_SAFE_RELEASE(resource_);
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


void D3D12Buffer::setGpuDebugName()
{
	SET_D3D12_RESOURCE_DEBUG_NAME(resource_, getName());
}

}  // namespace render
