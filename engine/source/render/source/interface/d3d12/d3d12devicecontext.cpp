#include "d3d12devicecontext.hpp"
#include "d3d12renderdevice.hpp"
#include "d3d12swapchain.hpp"
#include "d3d12texture.hpp"
#include "d3d12buffer.hpp"
#include "d3d12pipelinestate.hpp"
#include "d3d12fence.hpp"
#include "d3d12common.hpp"

#include "interface/devicecontext.h"
#include "interface/swapchain.h"


namespace render {


// Factory -------------------------------------------------------------------

template<ERenderAPI>
std::unique_ptr<IDeviceContext> IDeviceContext::create(IRenderDevice&) = delete;

template<>
std::unique_ptr<IDeviceContext> IDeviceContext::create<ERenderAPI::kD3D12>(IRenderDevice& render_device)
{
    return D3D12DeviceContext::create(render_device);
}


// Helpers -------------------------------------------------------------------

static D3D12_RESOURCE_STATES translateResourceState(EResourceState state)
{
    switch (state)
    {
        case EResourceState::kCommon:          return D3D12_RESOURCE_STATE_COMMON;
        case EResourceState::kRenderTarget:    return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case EResourceState::kPresent:         return D3D12_RESOURCE_STATE_PRESENT;
        case EResourceState::kCopyDest:        return D3D12_RESOURCE_STATE_COPY_DEST;
        case EResourceState::kCopySrc:         return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case EResourceState::kGenericRead:     return D3D12_RESOURCE_STATE_GENERIC_READ;
        case EResourceState::kDepthWrite:      return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case EResourceState::kDepthRead:       return D3D12_RESOURCE_STATE_DEPTH_READ;
        case EResourceState::kShaderResource:  return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
        case EResourceState::kUnorderedAccess: return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        default:                               return D3D12_RESOURCE_STATE_COMMON;
    }
}


// Create --------------------------------------------------------------------

std::unique_ptr<D3D12DeviceContext> D3D12DeviceContext::create(IRenderDevice& render_device)
{
    D3D12RenderDevice& d3d12_device = static_cast<D3D12RenderDevice&>(render_device);
    ID3D12Device9* device = d3d12_device.getDevice();

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
    D3D12_ASSERT_RETURN_NULL(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));
    SET_D3D12_RESOURCE_DEBUG_NAME(command_queue.Get(), "D3D12DeviceContext::command_queue_");

    std::array<D3D12DeviceContext::FrameContext, kBackBufferCount> frame_contexts = {};
    for (D3D12DeviceContext::FrameContext& frame_context : frame_contexts)
    {
        D3D12_ASSERT_RETURN_NULL(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&frame_context.command_allocator)));
        SET_D3D12_RESOURCE_DEBUG_NAME(frame_context.command_allocator, "D3D12DeviceContext::command_allocator");
    }

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
    D3D12_ASSERT_RETURN_NULL(device->CreateCommandList(
        0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        frame_contexts[0].command_allocator, nullptr,
        IID_PPV_ARGS(&command_list)));
    SET_D3D12_RESOURCE_DEBUG_NAME(command_list.Get(), "D3D12DeviceContext::command_list_");

    // Command list starts in recording state, close it immediately
    D3D12_ASSERT_RETURN_NULL(command_list->Close());

    std::unique_ptr<IFence> fence = IFence::create<ERenderAPI::kD3D12>(render_device);

    return std::unique_ptr<D3D12DeviceContext>(new D3D12DeviceContext(
        render_device,
        command_queue.Detach(),
        command_list.Detach(),
        std::move(frame_contexts),
        std::move(fence)));
}


// Frame ---------------------------------------------------------------------

void D3D12DeviceContext::begin()
{
    ASSERT_MSG_RETURN_VOID(!is_recording_, "DeviceContext: begin() called while already recording");

    FrameContext& frame_ctx = frame_contexts_[current_frame_index_];

    // Wait for GPU to finish previous use of this frame slot
    if (fence_->getCompletedValue() < frame_ctx.fence_value)
    {
        fence_->waitValueOnCpu(frame_ctx.fence_value);
    }

    D3D12_ASSERT_RETURN_VOID(frame_ctx.command_allocator->Reset());
    D3D12_ASSERT_RETURN_VOID(command_list_->Reset(frame_ctx.command_allocator, nullptr));

    is_recording_ = true;
}


void D3D12DeviceContext::end()
{
    ASSERT_MSG_RETURN_VOID(is_recording_, "DeviceContext: end() called without begin()");

    D3D12_ASSERT_RETURN_VOID(command_list_->Close());
    is_recording_ = false;
}


void D3D12DeviceContext::flush()
{
    ASSERT_MSG_RETURN_VOID(!is_recording_, "DeviceContext: flush() called while recording. Call end() first.");

    ID3D12CommandList* command_lists[] = { command_list_ };
    command_queue_->ExecuteCommandLists(1, command_lists);

    FrameContext& frame_ctx = frame_contexts_[current_frame_index_];
    frame_ctx.fence_value = static_cast<D3D12Fence*>(fence_.get())->signalOnGpu(command_queue_);

    current_frame_index_ = (current_frame_index_ + 1) % kBackBufferCount;
}


void D3D12DeviceContext::waitIdle()
{
    uint64_t fence_value = static_cast<D3D12Fence*>(fence_.get())->signalOnGpu(command_queue_);
    fence_->waitValueOnCpu(fence_value);
}


// Resource barriers ---------------------------------------------------------

void D3D12DeviceContext::transitionTexture(ITexture& texture, EResourceState from, EResourceState to)
{
    D3D12Texture& d3d12_texture = static_cast<D3D12Texture&>(texture);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = d3d12_texture.getResource();
    barrier.Transition.StateBefore = translateResourceState(from);
    barrier.Transition.StateAfter = translateResourceState(to);
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    command_list_->ResourceBarrier(1, &barrier);
}


// Output --------------------------------------------------------------------

void D3D12DeviceContext::setRenderTarget(ITexture& render_target, ITexture* depth_stencil)
{
    D3D12Texture& d3d12_texture = static_cast<D3D12Texture&>(render_target);

    D3D12RenderDevice& device = getRenderDevice();
    ID3D12DescriptorHeap* rtv_heap = device.getRtvHeap();

    // Use the texture's resource ID as offset into the RTV heap
    const UINT rtv_descriptor_size = device.getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    const UINT heap_index = static_cast<UINT>(d3d12_texture.getID() % kD3D12RTVDescriptorCount);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
    rtv_handle.ptr += static_cast<SIZE_T>(heap_index) * rtv_descriptor_size;

    device.getDevice()->CreateRenderTargetView(d3d12_texture.getResource(), nullptr, rtv_handle);

    if (depth_stencil != nullptr)
    {
        D3D12Texture& d3d12_depth_texture = static_cast<D3D12Texture&>(*depth_stencil);

        D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = device.getDsvHeap()->GetCPUDescriptorHandleForHeapStart();
        device.getDevice()->CreateDepthStencilView(d3d12_depth_texture.getResource(), nullptr, dsv_handle);

        command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
        return;
    }

    command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);
}


void D3D12DeviceContext::clearRenderTarget(ITexture& render_target, float r, float g, float b, float a)
{
    D3D12Texture& d3d12_texture = static_cast<D3D12Texture&>(render_target);

    D3D12RenderDevice& device = getRenderDevice();
    ID3D12DescriptorHeap* rtv_heap = device.getRtvHeap();

    const UINT rtv_descriptor_size = device.getDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    const UINT heap_index = static_cast<UINT>(d3d12_texture.getID() % kD3D12RTVDescriptorCount);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = rtv_heap->GetCPUDescriptorHandleForHeapStart();
    rtv_handle.ptr += static_cast<SIZE_T>(heap_index) * rtv_descriptor_size;

    const float clear_color[] = { r, g, b, a };
    command_list_->ClearRenderTargetView(rtv_handle, clear_color, 0, nullptr);
}


void D3D12DeviceContext::clearDepthStencil(ITexture& depth_stencil, float depth, uint8_t stencil)
{
    D3D12Texture& d3d12_texture = static_cast<D3D12Texture&>(depth_stencil);
    D3D12RenderDevice& device = getRenderDevice();

    D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = device.getDsvHeap()->GetCPUDescriptorHandleForHeapStart();
    device.getDevice()->CreateDepthStencilView(d3d12_texture.getResource(), nullptr, dsv_handle);

    command_list_->ClearDepthStencilView(dsv_handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}


void D3D12DeviceContext::setViewport(float x, float y, float width, float height)
{
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    command_list_->RSSetViewports(1, &viewport);
}


void D3D12DeviceContext::setScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
    D3D12_RECT scissor_rect = {};
    scissor_rect.left = left;
    scissor_rect.top = top;
    scissor_rect.right = right;
    scissor_rect.bottom = bottom;

    command_list_->RSSetScissorRects(1, &scissor_rect);
}


// Pipeline ------------------------------------------------------------------

void D3D12DeviceContext::setPipelineState(IPipelineState& pipeline_state)
{
    D3D12GraphicsPipelineState& d3d12_pso = static_cast<D3D12GraphicsPipelineState&>(pipeline_state);
    command_list_->SetGraphicsRootSignature(d3d12_pso.getRootSignature());
    command_list_->SetPipelineState(d3d12_pso.getPipelineState());
}


void D3D12DeviceContext::setPrimitiveTopology(EPrimitiveTopology topology)
{
    D3D12_PRIMITIVE_TOPOLOGY d3d12_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    switch (topology)
    {
        case EPrimitiveTopology::kPoint:    d3d12_topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;    break;
        case EPrimitiveTopology::kLine:     d3d12_topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;     break;
        case EPrimitiveTopology::kTriangle: d3d12_topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
        case EPrimitiveTopology::kPatch:    d3d12_topology = D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST; break;
    }
    command_list_->IASetPrimitiveTopology(d3d12_topology);
}


void D3D12DeviceContext::setGraphicsRoot32BitConstants(uint32_t root_parameter_index, uint32_t num_32bit_values, const void* data, uint32_t dest_offset_32bit_values)
{
    ASSERT_MSG_RETURN_VOID(is_recording_, "DeviceContext: setGraphicsRoot32BitConstants() called while not recording");
    ASSERT_MSG_RETURN_VOID(data != nullptr, "DeviceContext: setGraphicsRoot32BitConstants() data is null");

    command_list_->SetGraphicsRoot32BitConstants(
        root_parameter_index,
        num_32bit_values,
        data,
        dest_offset_32bit_values);
}


// Input ---------------------------------------------------------------------

void D3D12DeviceContext::setVertexBuffer(const IBuffer& buffer, uint32_t stride, uint32_t offset)
{
    const D3D12Buffer& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);

    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = d3d12_buffer.getResource()->GetGPUVirtualAddress() + offset;
    vbv.SizeInBytes = static_cast<UINT>(d3d12_buffer.getDesc().size - offset);
    vbv.StrideInBytes = stride;

    command_list_->IASetVertexBuffers(0, 1, &vbv);
}


void D3D12DeviceContext::setIndexBuffer(const IBuffer& buffer, uint32_t offset)
{
    const D3D12Buffer& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);

    D3D12_INDEX_BUFFER_VIEW ibv = {};
    ibv.BufferLocation = d3d12_buffer.getResource()->GetGPUVirtualAddress() + offset;
    ibv.SizeInBytes = static_cast<UINT>(d3d12_buffer.getDesc().size - offset);
    ibv.Format = DXGI_FORMAT_R32_UINT;

    command_list_->IASetIndexBuffer(&ibv);
}


// Draw ----------------------------------------------------------------------

void D3D12DeviceContext::draw(uint32_t vertex_count, uint32_t vertex_start_offset)
{
    drawInstanced(vertex_count, 1, vertex_start_offset, 0);
}


void D3D12DeviceContext::drawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
    drawIndexedInstanced(index_count, 1, start_index_location, base_vertex_location, 0);
}


void D3D12DeviceContext::drawInstanced(uint32_t vertex_count_per_instance, uint32_t instance_count,
    uint32_t start_vertex_location, uint32_t start_instance_location)
{
    command_list_->DrawInstanced(vertex_count_per_instance, instance_count, start_vertex_location, start_instance_location);
}


void D3D12DeviceContext::drawIndexedInstanced(uint32_t index_count_per_instance, uint32_t instance_count,
    uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location)
{
    command_list_->DrawIndexedInstanced(index_count_per_instance, instance_count,
        start_index_location, base_vertex_location, start_instance_location);
}


// Present -------------------------------------------------------------------

void D3D12DeviceContext::present(ISwapChain& swap_chain)
{
    D3D12SwapChain& d3d12_swap_chain = static_cast<D3D12SwapChain&>(swap_chain);
    d3d12_swap_chain.present();
    d3d12_swap_chain.updateCurrentBackBufferIndex();
}


// Constructor / Destructor --------------------------------------------------

D3D12DeviceContext::D3D12DeviceContext(
    IRenderDevice& render_device,
    ID3D12CommandQueue* command_queue,
    ID3D12GraphicsCommandList* command_list,
    std::array<FrameContext, kBackBufferCount>&& frame_contexts,
    std::unique_ptr<IFence>&& fence)
    : render_device_(render_device)
    , command_queue_(command_queue)
    , command_list_(command_list)
    , frame_contexts_(std::move(frame_contexts))
    , fence_(std::move(fence))
    , current_frame_index_(0)
    , is_recording_(false)
{
}


D3D12RenderDevice& D3D12DeviceContext::getRenderDevice()
{
    return static_cast<D3D12RenderDevice&>(render_device_);
}


D3D12DeviceContext::~D3D12DeviceContext()
{
    // Wait for all frames to complete
    for (uint32_t i = 0; i < kBackBufferCount; ++i)
    {
        if (fence_->getCompletedValue() < frame_contexts_[i].fence_value)
        {
            fence_->waitValueOnCpu(frame_contexts_[i].fence_value);
        }
    }

    D3D12_SAFE_RELEASE(command_list_);

    for (FrameContext& frame_context : frame_contexts_)
    {
        D3D12_SAFE_RELEASE(frame_context.command_allocator);
    }

    D3D12_SAFE_RELEASE(command_queue_);
}

}  // namespace render
