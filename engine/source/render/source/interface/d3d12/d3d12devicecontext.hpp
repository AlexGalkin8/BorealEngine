#pragma once

#include "interface/devicecontext.h"
#include "interface/fence.hpp"
#include "d3d12common.hpp"

#include <array>
#include <memory>


struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandQueue;

namespace render {
class IRenderDevice;
class D3D12RenderDevice;
}  // namespace render


namespace render {

class D3D12DeviceContext final : public IDeviceContext
{
    friend class IDeviceContext;
    friend class D3D12SwapChain;

    struct FrameContext
    {
        ID3D12CommandAllocator* command_allocator = nullptr;
        uint64_t fence_value = 0;
    };

public:
    static std::unique_ptr<D3D12DeviceContext> create(IRenderDevice& render_device);

    // Frame
    void begin() override;
    void end() override;
    void flush() override;
    void waitIdle() override;

    // Resource barriers
    void transitionTexture(ITexture& texture, EResourceState from, EResourceState to) override;

    // Output
    void setRenderTarget(ITexture& render_target, ITexture* depth_stencil = nullptr) override;
    void clearRenderTarget(ITexture& render_target, float r, float g, float b, float a) override;
    void clearDepthStencil(ITexture& depth_stencil, float depth = 1.0f, uint8_t stencil = 0) override;
    void setViewport(float x, float y, float width, float height) override;
    void setScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;

    // Pipeline
    void setPipelineState(IPipelineState& pipeline_state) override;
    void setPrimitiveTopology(EPrimitiveTopology topology) override;
    void setGraphicsRoot32BitConstants(uint32_t root_parameter_index, uint32_t num_32bit_values, const void* data, uint32_t dest_offset_32bit_values = 0) override;

    // Input
    void setVertexBuffer(const IBuffer& buffer, uint32_t stride, uint32_t offset = 0) override;
    void setIndexBuffer(const IBuffer& buffer, uint32_t offset = 0) override;

    // Draw
    void draw(uint32_t vertex_count, uint32_t vertex_start_offset) override;
    void drawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) override;
    void drawInstanced(uint32_t vertex_count_per_instance, uint32_t instance_count,
        uint32_t start_vertex_location, uint32_t start_instance_location) override;
    void drawIndexedInstanced(uint32_t index_count_per_instance, uint32_t instance_count,
        uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location) override;

    // Present
    void present(ISwapChain& swap_chain) override;

    ~D3D12DeviceContext() override;

private:
    D3D12DeviceContext(
        IRenderDevice& render_device,
        ID3D12CommandQueue* command_queue,
        ID3D12GraphicsCommandList* command_list,
        std::array<FrameContext, kBackBufferCount>&& frame_contexts,
        std::unique_ptr<IFence>&& fence);

    D3D12RenderDevice& getRenderDevice();

private:
    IRenderDevice& render_device_;

    ID3D12CommandQueue* command_queue_;
    ID3D12GraphicsCommandList* command_list_;
    std::array<FrameContext, kBackBufferCount> frame_contexts_;
    std::unique_ptr<IFence> fence_;

    uint32_t current_frame_index_ = 0;
    bool is_recording_ = false;
};

}  // namespace render
