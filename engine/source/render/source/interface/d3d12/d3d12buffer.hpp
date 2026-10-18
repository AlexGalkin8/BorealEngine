#pragma once

#include "interface/buffer.h"
#include "d3d12descriptorheap.hpp"

#include <memory>

struct ID3D12Resource2;


namespace render {


class D3D12RenderDevice;


class D3D12Buffer final : public IBuffer
{
public:
	static std::unique_ptr<D3D12Buffer> create(D3D12RenderDevice& render_device, const IBuffer::Desc& desc);

	// Uploads data to the buffer. Use Map/Unmap methods
	void upload(const void* data, size_t size) override;

	ID3D12Resource2* getResource() { return resource_; }
	ID3D12Resource2* getResource() const { return resource_; }

	bool hasSrv() const { return srv_descriptor_.isValid(); }
	bool hasCbv() const { return cbv_descriptor_.isValid(); }
	bool hasUav() const { return uav_descriptor_.isValid(); }

	D3D12_CPU_DESCRIPTOR_HANDLE getSrvHandle() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getCbvHandle() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getUavHandle() const;

	~D3D12Buffer() override;

private:
	D3D12Buffer(
		const Desc& desc,
		ID3D12Resource2* resource,
		D3D12OwnedDescriptor&& srv_descriptor,
		D3D12OwnedDescriptor&& cbv_descriptor,
		D3D12OwnedDescriptor&& uav_descriptor);

	void setGpuDebugName() override;

private:
	ID3D12Resource2* resource_;
	D3D12OwnedDescriptor srv_descriptor_;
	D3D12OwnedDescriptor cbv_descriptor_;
	D3D12OwnedDescriptor uav_descriptor_;
};

}  // namespace render
