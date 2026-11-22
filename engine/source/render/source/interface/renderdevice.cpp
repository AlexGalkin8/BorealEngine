#include "interface/renderdevice.h"
#include "interface/gpuresourcemanager.h"

#include "interface/commandlist.h"
#include "interface/common.h"
#include "interface/swapchain.h"

#include "d3d12/d3d12renderdevice.hpp"

#include "pipelinestatemanager.hpp"

#include <engine/core/utility/log.h>

#include <memory>
#include <utility>


namespace render {


template<ERenderAPI>
std::unique_ptr<IRenderDevice> IRenderDevice::create(const CreationDesc&) = delete;


template<>
std::unique_ptr<IRenderDevice> IRenderDevice::create<ERenderAPI::kD3D12>(const CreationDesc& creation_desc)
{
    std::unique_ptr<IGpuFrameCapturer> frame_capturer = IGpuFrameCapturer::create(creation_desc.frame_capture);

    std::unique_ptr<IRenderDevice> render_device = D3D12RenderDevice::create(creation_desc);
    if (!render_device) return nullptr;
    render_device->resource_manager_.reset(new GpuResourceManager(*render_device));
    render_device->pipeline_state_manager_ = IPipelineStateManager::create<ERenderAPI::kD3D12>(*render_device);
    render_device->frame_capturer_ = std::move(frame_capturer);
    return render_device;
}


IPipelineStateManager& IRenderDevice::getPipelineStateManager()
{
    return *pipeline_state_manager_;
}


GpuResourceManager& IRenderDevice::getResourceManager()
{
    return *resource_manager_;
}

void IRenderDevice::requestFrameCapture()
{
    if (!frame_capturer_)
    {
        LOG_WARNING("Capture", "No GPU frame capturer is available.");
        return;
    }
    frame_capturer_->requestFrameCapture();
}


void IRenderDevice::beginFrameCapture()
{
    if (frame_capturer_)
    {
        frame_capturer_->beginFrameCapture();
    }
}


void IRenderDevice::endFrameCapture()
{
    if (frame_capturer_)
    {
        frame_capturer_->endFrameCapture();
    }
}


IRenderDevice::~IRenderDevice() = default;


IRenderDevice::IRenderDevice()
    : render_info_()
{
}

}  // namespace render
