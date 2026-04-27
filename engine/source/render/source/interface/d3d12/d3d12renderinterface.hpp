#pragma once

#include "interface/renderinterface.h"

#include <array>


struct ID3D12Device9;
struct ID3D12CommandQueue;
struct IDXGIFactory6;
struct IDXGIAdapter4;
struct ID3D12DescriptorHeap;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12Fence;


namespace render {

class D3D12RenderInterface final : public IRenderInterface
{
	friend class IRenderInterface;
	friend class D3D12SwapChain;

	struct FrameContext
	{
		ID3D12CommandAllocator* command_allocator = nullptr;
		uint64_t fence_value = 0;
	};

public:
	std::unique_ptr<ISwapChain> createSwapChain(const platform::IWindow& window) override;

	void beginFrame() override;
	void endFrame() override;

	void draw(uint32_t vertex_count, uint32_t vertex_start_offset) override;
	void drawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) override;
	void drawInstanced(uint32_t vertex_count_per_instance, uint32_t instance_count,
		uint32_t start_vertex_location, uint32_t start_instance_location) override;
	void drawIndexedInstanced(uint32_t index_count_per_instance, uint32_t instance_count, uint32_t start_index_location,
		int32_t base_vertex_location, uint32_t start_instance_location) override;

	ID3D12Device9* const getDevice() { return device_; }

	~D3D12RenderInterface();

private:
	D3D12RenderInterface();

	static std::unique_ptr<D3D12RenderInterface> create(const CreationDesc& creation_desc);

private:
	IDXGIFactory6* dxgi_factory_;
	IDXGIAdapter4* adapter_;
	ID3D12Device9* device_;

	ID3D12CommandQueue* direct_queue_;

	ID3D12DescriptorHeap* rtv_heap_;
	ID3D12DescriptorHeap* dsv_heap_;
	ID3D12DescriptorHeap* srv_heap_;

	std::array<FrameContext, kBackBufferCount> frame_contexts_;

	ID3D12GraphicsCommandList* command_list_;
};

}  // namespace render
