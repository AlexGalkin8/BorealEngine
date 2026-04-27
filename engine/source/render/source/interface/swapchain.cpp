#include "interface/swapchain.h"
#include "interface/texture.h"


namespace render {

ISwapChain::~ISwapChain()
{
}


ISwapChain::ISwapChain(std::array<std::unique_ptr<ITexture>, kBackBufferCount>&& back_buffers)
	: back_buffers_(std::move(back_buffers))
{
}

}  // namespace render
