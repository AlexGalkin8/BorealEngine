#pragma once

#include "d3d12descriptorheap.hpp"
#include "d3d12commandqueue.hpp"
#include "interface/renderdevice.h"

#include <vector>


struct ID3D12Device9;
struct IDXGIFactory6;
struct IDXGIAdapter4;
struct ID3D12DescriptorHeap;
struct ID3D12CommandQueue;


namespace render {

class D3D12RenderDevice final : public IRenderDevice
{
	friend class IRenderDevice;
	friend class D3D12SwapChain;
	friend class D3D12ResourceManager;
	friend class D3D12PipelineStateManager;

public:
	std::unique_ptr<ISwapChain> createSwapChain(const platform::IWindow& window) override;
	std::unique_ptr<ITexture> createTexture(const ITexture::Desc& desc) override;
	std::unique_ptr<IBuffer> createBuffer(const IBuffer::Desc& desc) override;

	// Command list management
	ICommandList& acquireCommandList(ECommandListType type, const std::string& name) override;
	void submitCommandList(ICommandList& command_list) override;
	void executeCommandLists(ECommandListType type) override;

	// GPU synchronization
	void waitGpuIdle() override;

	// D3D12-specific getters
	ID3D12Device9* const getD3D12Device() const { return device_; }
	IDXGIFactory6* const getD3D12DxgiFactory() const { return dxgi_factory_; }

	ID3D12CommandQueue* const getDirectCommandQueue() const { return direct_queue_->getD3D12CommandQueue(); }
	ID3D12CommandQueue* const getComputeCommandQueue() const { return compute_queue_->getD3D12CommandQueue(); }
	ID3D12CommandQueue* const getCopyCommandQueue() const { return copy_queue_->getD3D12CommandQueue(); }

	D3D12DescriptorHeap* getRtvDescriptorHeap() const { return rtv_descriptor_heap_.get(); }
	D3D12DescriptorHeap* getDsvDescriptorHeap() const { return dsv_descriptor_heap_.get(); }
	D3D12DescriptorHeap* getCbvSrvUavCpuDescriptorHeap() const { return cbv_srv_uav_cpu_descriptor_heap_.get(); }
	D3D12DescriptorRingBuffer* getCbvSrvUavDescriptorHeap() const { return cbv_srv_uav_descriptor_heap_.get(); }

	~D3D12RenderDevice() override;

private:
	D3D12RenderDevice();

	static std::unique_ptr<D3D12RenderDevice> create(const CreationDesc& creation_desc);

private:
	IDXGIFactory6* dxgi_factory_ = nullptr;
	IDXGIAdapter4* adapter_ = nullptr;
	ID3D12Device9* device_ = nullptr;

	std::unique_ptr<D3D12DescriptorHeap> rtv_descriptor_heap_;
	std::unique_ptr<D3D12DescriptorHeap> dsv_descriptor_heap_;
	std::unique_ptr<D3D12DescriptorHeap> cbv_srv_uav_cpu_descriptor_heap_;
	std::unique_ptr<D3D12DescriptorRingBuffer> cbv_srv_uav_descriptor_heap_;

	std::unique_ptr<D3D12CommandQueue<ECommandListType::kDirect>>  direct_queue_;
	std::unique_ptr<D3D12CommandQueue<ECommandListType::kCompute>> compute_queue_;
	std::unique_ptr<D3D12CommandQueue<ECommandListType::kCopy>>    copy_queue_;
};

}  // namespace render
