#include "interface/renderdevice.h"

#include "interface/commandlist.h"
#include "interface/common.h"
#include "interface/swapchain.h"

#include "d3d12/d3d12renderdevice.hpp"

#include "pipelinestatemanager.hpp"

#include <memory>


namespace render {


template<ERenderAPI>
std::unique_ptr<IRenderDevice> IRenderDevice::create(const CreationDesc&) = delete;


template<>
std::unique_ptr<IRenderDevice> IRenderDevice::create<ERenderAPI::kD3D12>(const CreationDesc& creation_desc)
{
    std::unique_ptr<IRenderDevice> render_device = D3D12RenderDevice::create(creation_desc);
    render_device->pipeline_state_manager_ = IPipelineStateManager::create<ERenderAPI::kD3D12>(*render_device);
    return render_device;
}


IRenderDevice::IRenderDevice()
    : render_info_()
{
}


IPipelineStateManager& IRenderDevice::getPipelineStateManager()
{
    return *pipeline_state_manager_;
}


IRenderDevice::~IRenderDevice()
{

}

}  // namespace render
