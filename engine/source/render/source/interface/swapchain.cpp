#include "interface/swapchain.h"
#include "interface/texture.h"
#include "interface/renderdevice.h"


namespace render {

uint32_t ISwapChain::getWidth() const
{
	const std::unique_ptr<ITexture>& back_buffer = back_buffers_.front();
	return back_buffer ? back_buffer->getWidth() : 0;
}


uint32_t ISwapChain::getHeight() const
{
	const std::unique_ptr<ITexture>& back_buffer = back_buffers_.front();
	return back_buffer ? back_buffer->getHeight() : 0;
}


ISwapChain::~ISwapChain()
{
}


ISwapChain::ISwapChain(IRenderDevice& render_device, std::array<std::unique_ptr<ITexture>, kBackBufferCount>&& back_buffers)
	: render_device_(render_device)
	, back_buffers_(std::move(back_buffers))
{
}

}  // namespace render
