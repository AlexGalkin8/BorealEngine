#include "d3d12swapchain.hpp"
#include "d3d12common.hpp"
#include "d3d12renderdevice.hpp"
#include "d3d12texture.hpp"

#include <engine/platform/window.h>


namespace render {

std::unique_ptr<D3D12SwapChain> D3D12SwapChain::create(const IRenderDevice& render_device, const platform::IWindow& window)
{
	D3D12RenderDevice& d3d12_render_interface = const_cast<D3D12RenderDevice&>(static_cast<const D3D12RenderDevice&>(render_device));

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.BufferCount = kBackBufferCount;
	swap_chain_desc.Width = window.getWidth();
	swap_chain_desc.Height = window.getHeight();
	swap_chain_desc.Format = internal::translateTextureFormatToDxgiFormat(kBackBufferFormat);
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;

	D3D12_ASSERT_RETURN_NULL(
		d3d12_render_interface.getD3D12DxgiFactory()->CreateSwapChainForHwnd(
		d3d12_render_interface.getDirectCommandQueue(),
		static_cast<HWND>(window.getHandle()),
		&swap_chain_desc, nullptr, nullptr, &swap_chain));

	std::array<std::unique_ptr<ITexture>, kBackBufferCount> back_buffers;
	for (uint32_t i = 0; i < kBackBufferCount; i++)
	{
		ITexture::Desc texture_desc = {};
		texture_desc.width = swap_chain_desc.Width;
		texture_desc.height = swap_chain_desc.Height;
		texture_desc.depth = 1;
		texture_desc.mipCount = 1;
		texture_desc.format = kBackBufferFormat;

		ID3D12Resource2* resource = nullptr;
		D3D12_ASSERT_RETURN_NULL(swap_chain->GetBuffer(i, IID_PPV_ARGS(&resource)));

		back_buffers[i] = D3D12Texture::createBackBuffer(d3d12_render_interface, texture_desc, resource);
		ASSERT_MSG_RETURN_NULL(back_buffers[i] != nullptr, "D3D12SwapChain: failed to create back buffer texture");
		SET_RENDER_OBJECT_DEBUG_NAME(*back_buffers[i], "BackBuffer_{}", i);
	}

	return std::unique_ptr<D3D12SwapChain>(new D3D12SwapChain(
		d3d12_render_interface,
		std::move(back_buffers),
		swap_chain.Detach()));
}


D3D12SwapChain::D3D12SwapChain(
	D3D12RenderDevice& render_device,
	std::array<std::unique_ptr<ITexture>, kBackBufferCount>&& back_buffers,
	IDXGISwapChain1* swap_chain)
	: ISwapChain(render_device, std::move(back_buffers))
	, swap_chain_(swap_chain)
{
}


D3D12SwapChain::~D3D12SwapChain()
{
	D3D12_SAFE_RELEASE(swap_chain_);
}


void D3D12SwapChain::present()
{
	swap_chain_->Present(1, 0);
	updateCurrentBackBufferIndex();
}


void D3D12SwapChain::updateCurrentBackBufferIndex()
{
	Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_chain3;
	if (SUCCEEDED(swap_chain_->QueryInterface(IID_PPV_ARGS(&swap_chain3))))
	{
		current_back_buffer_index_ = swap_chain3->GetCurrentBackBufferIndex();
	}
}


void D3D12SwapChain::setGpuDebugName()
{
}


void D3D12SwapChain::resize(uint32_t width, uint32_t height)
{
	if (!swap_chain_ || width == 0 || height == 0)
	{
		return;
	}

	// Release old back buffers before resize
	for (auto& back_buffer : back_buffers_)
	{
		back_buffer.reset();
	}

	// Resize buffers
	D3D12_ASSERT_RETURN_VOID(swap_chain_->ResizeBuffers(
		kBackBufferCount,
		width,
		height,
		internal::translateTextureFormatToDxgiFormat(kBackBufferFormat),
		0));

	updateCurrentBackBufferIndex();

	for (uint32_t i = 0; i < kBackBufferCount; i++)
	{
		ITexture::Desc texture_desc = {};
		texture_desc.width = width;
		texture_desc.height = height;
		texture_desc.depth = 1;
		texture_desc.mipCount = 1;
		texture_desc.format = kBackBufferFormat;

		ID3D12Resource2* resource = nullptr;
		D3D12_ASSERT_RETURN_VOID(swap_chain_->GetBuffer(i, IID_PPV_ARGS(&resource)));
		SET_D3D12_RESOURCE_DEBUG_NAME(resource, "BackBuffer_%u", i);

		back_buffers_[i] = D3D12Texture::createBackBuffer(static_cast<D3D12RenderDevice&>(render_device_), texture_desc, resource);
		ASSERT_MSG_RETURN_VOID(back_buffers_[i] != nullptr, "D3D12SwapChain: failed to recreate back buffer texture");
	}
}

}  // namespace render
