#include "d3d12swapchain.hpp"
#include "d3d12common.hpp"
#include "d3d12renderdevice.hpp"
#include "d3d12devicecontext.hpp"
#include "d3d12resourcemanager.hpp"
#include "d3d12texture.hpp"

#include <engine/platform/window.h>


namespace render {

std::unique_ptr<D3D12SwapChain> D3D12SwapChain::create(const IRenderDevice& render_device, const platform::IWindow& window)
{
	const D3D12RenderDevice& d3d12_render_interface = static_cast<const D3D12RenderDevice&>(render_device);

	DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
	swap_chain_desc.BufferCount = kBackBufferCount;
	swap_chain_desc.Width = window.getWidth();
	swap_chain_desc.Height = window.getHeight();
	swap_chain_desc.Format = internal::translateTextureFormatToDxgiFormat(kBackBufferFormat);
	swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swap_chain_desc.SampleDesc.Count = 1;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> swap_chain;
	D3D12DeviceContext& d3d12_ctx = static_cast<D3D12DeviceContext&>(
		const_cast<IRenderDevice&>(render_device).getImmediateContext());

	D3D12_ASSERT_RETURN_NULL(
		d3d12_render_interface.getDxgiFactory()->CreateSwapChainForHwnd(
		d3d12_ctx.command_queue_,
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
		SET_D3D12_RESOURCE_DEBUG_NAME(resource, ("BackBuffer_" + std::to_string(i)).c_str());
		back_buffers[i].reset(new D3D12Texture(texture_desc, resource));
	}

	return std::unique_ptr<D3D12SwapChain>(new D3D12SwapChain(std::move(back_buffers), swap_chain.Detach()));
}


D3D12SwapChain::D3D12SwapChain(std::array<std::unique_ptr<ITexture>, kBackBufferCount>&& back_buffers, IDXGISwapChain1* swap_chain)
	: ISwapChain(std::move(back_buffers))
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
		SET_D3D12_RESOURCE_DEBUG_NAME(resource, ("BackBuffer_" + std::to_string(i)).c_str());
		back_buffers_[i].reset(new D3D12Texture(texture_desc, resource));
	}
}

}  // namespace render
