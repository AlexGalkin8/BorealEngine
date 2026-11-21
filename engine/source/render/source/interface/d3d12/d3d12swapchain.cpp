#include "d3d12swapchain.hpp"
#include "d3d12common.hpp"
#include "d3d12renderdevice.hpp"
#include "d3d12texture.hpp"

#include <engine/core/utility/log.h>
#include <engine/platform/window.h>


namespace render {

std::unique_ptr<D3D12SwapChain> D3D12SwapChain::create(D3D12RenderDevice& render_device, const platform::IWindow& window)
{
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
		render_device.getD3D12DxgiFactory()->CreateSwapChainForHwnd(
			render_device.getDirectCommandQueue(),
			static_cast<HWND>(window.getHandle()),
			&swap_chain_desc, nullptr, nullptr, &swap_chain));

	std::unique_ptr<D3D12SwapChain> result(new D3D12SwapChain(render_device, swap_chain.Get()));
	swap_chain.Detach();
	ASSERT_MSG_RETURN_NULL(result->acquireBackBuffers(), "Failed to acquire initial swapchain back buffers");
	return result;
}


void D3D12SwapChain::resize(uint32_t width, uint32_t height)
{
	ASSERT_MSG_RETURN_VOID(swap_chain_ != nullptr, "Resize requires a native swapchain");
	ASSERT_MSG_RETURN_VOID(width > 0 && height > 0, "Swapchain resize requires a nonzero size");

	DXGI_SWAP_CHAIN_DESC1 previous_desc = {};
	const HRESULT describe_result = swap_chain_->GetDesc1(&previous_desc);
	if (FAILED(describe_result))
	{
		LOG_WARNING("D3D12SwapChain", "Resize to {}x{} skipped: GetDesc1 failed (HRESULT 0x{:08X}).",
			width, height, static_cast<uint32_t>(describe_result));
	}
	D3D12_ASSERT_RETURN_VOID(describe_result);

	if (previous_desc.Width == width && previous_desc.Height == height)
	{
		if (!back_buffers_.front()) acquireBackBuffers();
		return;
	}

	// DXGI requires all application references to the old back buffers to be released.
	back_buffers_ = {};
	const HRESULT resize_result = swap_chain_->ResizeBuffers(kBackBufferCount, width, height, previous_desc.Format, previous_desc.Flags);
	if (FAILED(resize_result))
	{
		LOG_WARNING("D3D12SwapChain", "ResizeBuffers to {}x{} failed (HRESULT 0x{:08X}). Reacquiring previous buffers.",
			width, height, static_cast<uint32_t>(resize_result));
	}
	D3D12_ASSERT(resize_result);

	const bool buffers_acquired = acquireBackBuffers();
	if (buffers_acquired || FAILED(resize_result))
	{
		return;
	}

	const HRESULT restore_result = swap_chain_->ResizeBuffers(kBackBufferCount,
		previous_desc.Width, previous_desc.Height, previous_desc.Format, previous_desc.Flags);
	if (FAILED(restore_result))
	{
		LOG_ERROR("D3D12SwapChain", "ResizeBuffers failed while restoring {}x{} (HRESULT 0x{:08X}).",
			previous_desc.Width, previous_desc.Height, static_cast<uint32_t>(restore_result));
	}
	D3D12_ASSERT_RETURN_VOID(restore_result);
	acquireBackBuffers();
}


void D3D12SwapChain::present()
{
	D3D12_ASSERT_RETURN_VOID(swap_chain_->Present(1, 0));
	updateCurrentBackBufferIndex();
}


D3D12SwapChain::~D3D12SwapChain()
{
	D3D12_SAFE_RELEASE(swap_chain_);
}


D3D12SwapChain::D3D12SwapChain(D3D12RenderDevice& render_device, IDXGISwapChain1* swap_chain)
	: ISwapChain(render_device, {})
	, swap_chain_(swap_chain)
{
}


void D3D12SwapChain::updateCurrentBackBufferIndex()
{
	Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_chain3;
	D3D12_ASSERT_RETURN_VOID(swap_chain_->QueryInterface(IID_PPV_ARGS(&swap_chain3)));
	current_back_buffer_index_ = swap_chain3->GetCurrentBackBufferIndex();
}


bool D3D12SwapChain::acquireBackBuffers()
{
	std::array<std::unique_ptr<ITexture>, kBackBufferCount> back_buffers;
	for (uint32_t i = 0; i < kBackBufferCount; i++)
	{
		Microsoft::WRL::ComPtr<ID3D12Resource2> resource;
		const HRESULT acquire_result = swap_chain_->GetBuffer(i, IID_PPV_ARGS(&resource));
		if (FAILED(acquire_result))
		{
			LOG_ERROR("D3D12SwapChain", "GetBuffer failed for back buffer {} (HRESULT 0x{:08X}).",
				i, static_cast<uint32_t>(acquire_result));
		}
		D3D12_ASSERT_RETURN(acquire_result, false);

		const D3D12_RESOURCE_DESC buffer_desc = resource->GetDesc();
		ITexture::Desc texture_desc = {};
		texture_desc.width = static_cast<uint32_t>(buffer_desc.Width);
		texture_desc.height = buffer_desc.Height;
		texture_desc.format = kBackBufferFormat;

		back_buffers[i] = D3D12Texture::createBackBuffer(
			static_cast<D3D12RenderDevice&>(render_device_), texture_desc, resource.Get());
		if (!back_buffers[i])
		{
			LOG_ERROR("D3D12SwapChain", "Failed to create views for back buffer {}.", i);
		}
		ASSERT_MSG_RETURN(back_buffers[i] != nullptr, false, "Failed to create views for back buffer %u", i);
		resource.Detach();
		SET_RENDER_OBJECT_DEBUG_NAME(*back_buffers[i], "BackBuffer_{}", i);
	}

	back_buffers_ = std::move(back_buffers);
	updateCurrentBackBufferIndex();
	return true;
}


void D3D12SwapChain::setGpuDebugName()
{
}

}  // namespace render
