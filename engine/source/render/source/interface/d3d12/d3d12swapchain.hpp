#pragma once

#include "interface/swapchain.h"


struct IDXGISwapChain1;


namespace render {

class IRenderDevice;
class ITexture;
class D3D12RenderDevice;


class D3D12SwapChain final : public ISwapChain
{
public:
	static std::unique_ptr<D3D12SwapChain> create(const IRenderDevice& render_device, const platform::IWindow& window);

	void resize(uint32_t width, uint32_t height) override;
	void present() override;

	~D3D12SwapChain() override;

private:
	D3D12SwapChain(
		D3D12RenderDevice& render_device,
		std::array<std::unique_ptr<ITexture>, kBackBufferCount>&& back_buffers,
		IDXGISwapChain1* swap_chain);

	void updateCurrentBackBufferIndex();

	void setGpuDebugName() override;

private:
	IDXGISwapChain1* swap_chain_;
};

}  // namespace render
