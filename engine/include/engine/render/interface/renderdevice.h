#pragma once

#include "common.h"

#include "texture.h"
#include "buffer.h"
#include "gpuframecapturer.h"

#include <cstdint>
#include <memory>


namespace platform {
class IWindow;
}  // namespace platform

namespace render {
class ISwapChain;
class ICommandList;
class IPipelineStateManager;
class GpuResourceManager;
}  // namespace render


namespace render {

class IRenderDevice : public utility::NonCopyable
{
	friend class GpuResourceManager;
public:
	struct Info
	{
		bool is_ray_tracing_support = false;
	};

	struct CreationDesc
	{
		bool enable_debug_validation_layer = true;
		FrameCaptureSettings frame_capture;
	};

public:
	template<ERenderAPI render_api>
	static std::unique_ptr<IRenderDevice> create(const CreationDesc& creation_desc);

	IPipelineStateManager& getPipelineStateManager();
	GpuResourceManager& getResourceManager();

	// Resource creation
	virtual std::unique_ptr<ISwapChain> createSwapChain(const platform::IWindow& window) = 0;

	// Commamd list management
	virtual ICommandList& acquireCommandList(ECommandListType type, const std::string& name) = 0;
	virtual void submitCommandList(ICommandList& command_list) = 0;
	virtual void executeCommandLists(ECommandListType type) = 0;

	// GPU synchronization
	virtual void waitGpuIdle() = 0;

	void requestFrameCapture();
	void beginFrameCapture();
	void endFrameCapture();

	virtual ~IRenderDevice();

protected:
	IRenderDevice();

	// Low-level factories transfer ownership only to the resource manager.
	virtual std::unique_ptr<ITexture> createTexture(const ITexture::Desc& desc) = 0;
	virtual std::unique_ptr<IBuffer> createBuffer(const IBuffer::Desc& desc) = 0;

protected:
	Info render_info_;
	std::unique_ptr<GpuResourceManager> resource_manager_;
	std::unique_ptr<IPipelineStateManager> pipeline_state_manager_;

private:
	std::unique_ptr<IGpuFrameCapturer> frame_capturer_;
};

}  // namespace render
