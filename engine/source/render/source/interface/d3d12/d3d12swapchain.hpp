#pragma once

#include "interface/swapchain.h"

struct IDXGISwapChain1;


namespace render {

class D3D12RenderDevice;


class D3D12SwapChain final : public ISwapChain
{
public:
	static std::unique_ptr<D3D12SwapChain> create(D3D12RenderDevice& render_device, const platform::IWindow& window);

	void resize(uint32_t width, uint32_t height) override;
	void present() override;

	~D3D12SwapChain() override;

private:
	D3D12SwapChain(D3D12RenderDevice& render_device, IDXGISwapChain1* swap_chain);

	void updateCurrentBackBufferIndex();
	bool acquireBackBuffers();

	void setGpuDebugName() override;

private:
	// Owns the native COM reference transferred by create().
	IDXGISwapChain1* swap_chain_;
};

}  // namespace render
