#pragma once

#include "common.h"

#include "texture.h"
#include "buffer.h"

#include <cstdint>
#include <memory>


namespace platform {
class IWindow;
}  // namespace platform

namespace render {
class ISwapChain;
class ICommandList;
class IPipelineStateManager;
}  // namespace render


namespace render {

class IRenderDevice : public utility::NonCopyable
{
public:
	struct Info
	{
		bool is_ray_tracing_support = false;
	};

	struct CreationDesc
	{
		bool enable_debug_validation_layer = true;
	};

public:
	template<ERenderAPI render_api>
	static std::unique_ptr<IRenderDevice> create(const CreationDesc& creation_desc);

	IPipelineStateManager& getPipelineStateManager();

	// Resource creation
	virtual std::unique_ptr<ISwapChain> createSwapChain(const platform::IWindow& window) = 0;
	virtual std::unique_ptr<ITexture> createTexture(const ITexture::Desc& desc) = 0;
	virtual std::unique_ptr<IBuffer> createBuffer(const IBuffer::Desc& desc) = 0;

	// Commamd list management
	virtual ICommandList& acquireCommandList(ECommandListType type, const std::string& name) = 0;
	virtual void submitCommandList(ICommandList& command_list) = 0;
	virtual void executeCommandLists(ECommandListType type) = 0;

	// GPU synchronization
	virtual void waitGpuIdle() = 0;

	virtual ~IRenderDevice();

protected:
	IRenderDevice();

protected:
	Info render_info_;
	std::unique_ptr<IPipelineStateManager> pipeline_state_manager_;
};

}  // namespace render
