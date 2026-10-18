#include "interface/swapchain.h"
#include "interface/texture.h"
#include "interface/renderdevice.h"


namespace render {

ISwapChain::~ISwapChain()
{
}


ISwapChain::ISwapChain(IRenderDevice& render_device, std::array<std::unique_ptr<ITexture>, kBackBufferCount>&& back_buffers)
	: render_device_(render_device)
	, back_buffers_(std::move(back_buffers))
{
}

}  // namespace render
