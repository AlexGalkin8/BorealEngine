#include "interface/renderdevice.h"

#include "interface/devicecontext.h"
#include "interface/common.h"
#include "interface/swapchain.h"

#include "d3d12/d3d12renderdevice.hpp"

#include "pipelinestatemanager.hpp"
#include "resourcemanager.hpp"

#include <memory>


namespace render {


template<ERenderAPI>
std::unique_ptr<IRenderDevice> IRenderDevice::create(const CreationDesc&) = delete;


template<>
std::unique_ptr<IRenderDevice> IRenderDevice::create<ERenderAPI::kD3D12>(const CreationDesc& creation_desc)
{
    std::unique_ptr<IRenderDevice> render_device = D3D12RenderDevice::create(creation_desc);
    render_device->resource_manager_ = IResourceManager::create<ERenderAPI::kD3D12>(*render_device);
    render_device->pipeline_state_manager_ = IPipelineStateManager::create<ERenderAPI::kD3D12>(*render_device);
    render_device->immediate_context_ = IDeviceContext::create<ERenderAPI::kD3D12>(*render_device);
    return render_device;
}


IRenderDevice::IRenderDevice()
    : render_info_()
{
}


IResourceManager& IRenderDevice::getResourceManager()
{
    return *resource_manager_;
}


IPipelineStateManager& IRenderDevice::getPipelineStateManager()
{
    return *pipeline_state_manager_;
}


IDeviceContext& IRenderDevice::getImmediateContext()
{
    return *immediate_context_;
}


IRenderDevice::~IRenderDevice()
{

}

}  // namespace render
