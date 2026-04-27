#pragma once

#include "interface/buffer.h"


struct ID3D12Resource2;


namespace render {

class D3D12ResourceManager;


class D3D12Buffer final : public IBuffer
{
	friend class D3D12ResourceManager;

public:
	~D3D12Buffer() override;

	// Uploads data to the buffer. Use Map/Unmap methods
	void upload(const void* data, size_t size) override;

	ID3D12Resource2* getResource() { return resource_; }
	ID3D12Resource2* getResource() const { return resource_; }

private:
	D3D12Buffer(const Desc& desc, ID3D12Resource2* resource);

	void setGpuDebugName() override;

private:
	ID3D12Resource2* resource_;
};

}  // namespace render
