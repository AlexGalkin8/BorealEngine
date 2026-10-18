#pragma once

#include "common.h"

#include <array>
#include <memory>


namespace platform {
class IWindow;
}  // namespace platform


namespace render {

class IRenderDevice;
class ITexture;


class ISwapChain : public internal::RenderObject
{
public:
	ITexture& getCurrentBackBuffer() { return *back_buffers_.at(current_back_buffer_index_).get(); };

	virtual void resize(uint32_t width, uint32_t height) = 0;
	virtual void present() = 0;

	virtual ~ISwapChain();

protected:
	ISwapChain(IRenderDevice& render_device, std::array<std::unique_ptr<ITexture>, kBackBufferCount>&& back_buffers);

protected:
	IRenderDevice& render_device_;
	std::array<std::unique_ptr<ITexture>, kBackBufferCount> back_buffers_;
	uint32_t current_back_buffer_index_ = 0;
};

}  // namespace render
