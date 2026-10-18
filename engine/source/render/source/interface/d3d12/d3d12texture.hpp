#pragma once

#include "interface/texture.h"
#include "d3d12descriptorheap.hpp"

#include <memory>
#include <optional>


struct ID3D12Resource2;


namespace render {

class D3D12RenderDevice;
class D3D12SwapChain;


class D3D12Texture final : public ITexture
{
public:
	static std::unique_ptr<D3D12Texture> create(D3D12RenderDevice& render_device, const ITexture::Desc& desc);
	static std::unique_ptr<D3D12Texture> createBackBuffer(D3D12RenderDevice& render_device, const ITexture::Desc& desc, ID3D12Resource2* resource);

	ID3D12Resource2* getResource() { return resource_; }

	D3D12_CPU_DESCRIPTOR_HANDLE getSrvHandle() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getRtvHandle() const;
	D3D12_CPU_DESCRIPTOR_HANDLE getDsvHandle() const;

	bool hasSrv() const { return srv_descriptor_.isValid(); }
	bool hasRtv() const { return rtv_descriptor_.isValid(); }
	bool hasDsv() const { return dsv_descriptor_.has_value() && dsv_descriptor_->isValid(); }

	~D3D12Texture() override;

private:
	D3D12Texture(
		const Desc& desc,
		ID3D12Resource2* resource,
		D3D12OwnedDescriptor&& srv_descriptor = {},
		D3D12OwnedDescriptor&& rtv_descriptor = {},
		std::optional<D3D12OwnedDescriptor>&& dsv_descriptor = std::nullopt);

	void setGpuDebugName() override;

private:
	ID3D12Resource2* resource_ = nullptr;
	D3D12OwnedDescriptor srv_descriptor_;
	D3D12OwnedDescriptor rtv_descriptor_;
	std::optional<D3D12OwnedDescriptor> dsv_descriptor_;
};

}  // namespace render
