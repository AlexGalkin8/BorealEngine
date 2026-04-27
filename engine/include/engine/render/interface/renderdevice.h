#pragma once

#include "common.h"

#include <cstdint>
#include <memory>


namespace platform {
class IWindow;
}  // namespace platform

namespace render {
class ISwapChain;
class IDeviceContext;
class IResourceManager;
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

    IResourceManager& getResourceManager();
    IPipelineStateManager& getPipelineStateManager();
    IDeviceContext& getImmediateContext();

    virtual std::unique_ptr<ISwapChain> createSwapChain(const platform::IWindow& window) = 0;

    virtual ~IRenderDevice();

protected:
    IRenderDevice();

protected:
    Info render_info_;
    std::unique_ptr<IResourceManager> resource_manager_;
    std::unique_ptr<IPipelineStateManager> pipeline_state_manager_;
    std::unique_ptr<IDeviceContext> immediate_context_;
};

}  // namespace render
