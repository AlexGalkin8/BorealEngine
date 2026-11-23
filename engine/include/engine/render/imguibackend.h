#pragma once

#include <engine/render/interface/common.h>

#include <array>
#include <memory>


struct ImGuiContext;

namespace platform {
class IInput;
}  // namespace platform


namespace render {

class IBuffer;
class IRenderDevice;
class ICommandList;
class IPipelineState;
class ITexture;


class ImGuiBackend final : public utility::NonCopyable
{
public:
	static std::unique_ptr<ImGuiBackend> create(IRenderDevice& render_device);

	void newFrame(const platform::IInput& input, uint32_t width, uint32_t height, float delta_time);
	bool wantsMouse() const;
	bool wantsKeyboard() const;
	void draw(ITexture& target);

	~ImGuiBackend();

private:
	struct FrameBuffers
	{
		IBuffer* vertices = nullptr;
		IBuffer* indices = nullptr;
		FenceValue fence = 0;
	};

	explicit ImGuiBackend(IRenderDevice& render_device);

	bool init();
	bool ensureBuffer(IBuffer*& buffer, size_t required_size, bool index_buffer);

	IRenderDevice& render_device_;
	ImGuiContext* context_ = nullptr;
	ITexture* font_texture_ = nullptr;
	IPipelineState* pipeline_state_ = nullptr;
	std::array<FrameBuffers, kBackBufferCount> frames_ = {};
	size_t frame_index_ = 0;
};

}  // namespace render
