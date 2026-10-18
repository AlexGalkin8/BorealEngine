#include "d3d12commandlist.hpp"
#include "d3d12renderdevice.hpp"
#include "d3d12swapchain.hpp"
#include "d3d12texture.hpp"
#include "d3d12buffer.hpp"
#include "d3d12pipelinestate.hpp"
#include "d3d12fence.hpp"
#include "d3d12common.hpp"

#include "interface/commandlist.h"
#include "interface/swapchain.h"


namespace render {

// Static Helpers -------------------------------------------------------------------

static constexpr D3D12_RESOURCE_STATES translateResourceState(EResourceState state)
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

static constexpr D3D12_COMMAND_LIST_TYPE kD3D12CommandListTypeMap[] = {
	D3D12_COMMAND_LIST_TYPE_DIRECT,   // ECommandListType::kDirect
	D3D12_COMMAND_LIST_TYPE_COMPUTE,  // ECommandListType::kCompute
	D3D12_COMMAND_LIST_TYPE_COPY      // ECommandListType::kCopy
};


// Create --------------------------------------------------------------------

std::unique_ptr<D3D12CommandList> D3D12CommandList::create(IRenderDevice& render_device, ECommandListType type)
{
	D3D12RenderDevice& d3d12_device = static_cast<D3D12RenderDevice&>(render_device);
	ID3D12Device9* device = d3d12_device.getD3D12Device();

	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> command_allocator;
	D3D12_ASSERT_RETURN_NULL(device->CreateCommandAllocator(
		translateCommandListType(type), IID_PPV_ARGS(&command_allocator)));

	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> command_list;
	D3D12_ASSERT_RETURN_NULL(device->CreateCommandList(
		0, translateCommandListType(type),
		command_allocator.Get(), nullptr,
		IID_PPV_ARGS(&command_list)));

	// Command list starts in recording state, close it immediately
	D3D12_ASSERT_RETURN_NULL(command_list->Close());

	return std::unique_ptr<D3D12CommandList>(new D3D12CommandList(render_device, type, command_list.Detach(), command_allocator.Detach()));
}


// Frame ---------------------------------------------------------------------

void D3D12CommandList::beginRecording(DescriptorRange descriptor_range)
{
	ASSERT_MSG_RETURN_VOID(!is_recording_, "CommandList: begin() called while already recording");
	ASSERT_MSG_RETURN_VOID(descriptor_range.isValid(), "CommandList: descriptor range is invalid");

	D3D12_ASSERT_RETURN_VOID(command_allocator_->Reset());
	D3D12_ASSERT_RETURN_VOID(command_list_->Reset(command_allocator_, nullptr));

	reserved_srv_range_ = descriptor_range;
	reserved_srv_range_.offset = 0;

	D3D12RenderDevice& render_device = getRenderDevice();
	ID3D12DescriptorHeap* descriptor_heaps[] = {
		render_device.getCbvSrvUavDescriptorHeap()->getHeap()
	};
	command_list_->SetDescriptorHeaps(_countof(descriptor_heaps), descriptor_heaps);

	is_recording_ = true;
}


void D3D12CommandList::endRecording()
{
	ASSERT_MSG_RETURN_VOID(is_recording_, "CommandList: end() called without begin()");
	D3D12_ASSERT_RETURN_VOID(command_list_->Close());
	is_recording_ = false;
	reserved_srv_range_.reset();
}


// Resource barriers ---------------------------------------------------------

void D3D12CommandList::transitionTexture(ITexture& texture, EResourceState from, EResourceState to)
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

void D3D12CommandList::setRenderTarget(ITexture& render_target, ITexture* depth_stencil)
{
	D3D12Texture& d3d12_texture = static_cast<D3D12Texture&>(render_target);
	ASSERT_MSG_RETURN_VOID(d3d12_texture.hasRtv(), "Render target texture has no RTV descriptor");

	D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = d3d12_texture.getRtvHandle();

	if (depth_stencil != nullptr)
	{
		D3D12Texture& d3d12_depth_texture = static_cast<D3D12Texture&>(*depth_stencil);
		ASSERT_MSG_RETURN_VOID(d3d12_depth_texture.hasDsv(), "Depth texture has no DSV descriptor");

		D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle = d3d12_depth_texture.getDsvHandle();
		command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE, &dsv_handle);
		return;
	}

	command_list_->OMSetRenderTargets(1, &rtv_handle, FALSE, nullptr);
}


void D3D12CommandList::clearRenderTarget(ITexture& render_target, float r, float g, float b, float a)
{
	D3D12Texture& d3d12_texture = static_cast<D3D12Texture&>(render_target);
	ASSERT_MSG_RETURN_VOID(d3d12_texture.hasRtv(), "Render target texture has no RTV descriptor");

	const float clear_color[] = { r, g, b, a };
	command_list_->ClearRenderTargetView(d3d12_texture.getRtvHandle(), clear_color, 0, nullptr);
}


void D3D12CommandList::clearDepthStencil(ITexture& depth_stencil, float depth, uint8_t stencil)
{
	D3D12Texture& d3d12_texture = static_cast<D3D12Texture&>(depth_stencil);
	ASSERT_MSG_RETURN_VOID(d3d12_texture.hasDsv(), "CommandList: depth texture has no DSV descriptor");

	command_list_->ClearDepthStencilView(d3d12_texture.getDsvHandle(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, depth, stencil, 0, nullptr);
}


void D3D12CommandList::setViewport(float x, float y, float width, float height)
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


void D3D12CommandList::setScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom)
{
	D3D12_RECT scissor_rect = {};
	scissor_rect.left = left;
	scissor_rect.top = top;
	scissor_rect.right = right;
	scissor_rect.bottom = bottom;

	command_list_->RSSetScissorRects(1, &scissor_rect);
}


// Pipeline ------------------------------------------------------------------

void D3D12CommandList::setPipelineState(IPipelineState& pipeline_state)
{
	D3D12GraphicsPipelineState& d3d12_pso = static_cast<D3D12GraphicsPipelineState&>(pipeline_state);
	command_list_->SetGraphicsRootSignature(d3d12_pso.getRootSignature());
	command_list_->SetPipelineState(d3d12_pso.getPipelineState());
}


void D3D12CommandList::setPrimitiveTopology(EPrimitiveTopology topology)
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


void D3D12CommandList::setGraphicsRoot32BitConstants(uint32_t root_parameter_index, uint32_t num_32bit_values, const void* data, uint32_t dest_offset_32bit_values)
{
	ASSERT_MSG_RETURN_VOID(is_recording_, "Called while not recording");
	ASSERT_MSG_RETURN_VOID(data != nullptr, "data is null");

	command_list_->SetGraphicsRoot32BitConstants(
		root_parameter_index,
		num_32bit_values,
		data,
		dest_offset_32bit_values);
}


void D3D12CommandList::setDescriptorSet(uint32_t root_parameter_index, std::initializer_list<DescriptorResource> resources)
{
	ASSERT_MSG_RETURN_VOID(is_recording_, "Called while not recording");
	ASSERT_MSG_RETURN_VOID(!resources.size() || reserved_srv_range_.isValid(), "Descriptor range was not reserved");
	ASSERT_MSG_RETURN_VOID(reserved_srv_range_.offset + resources.size() <= reserved_srv_range_.capacity, "Reserved descriptor range exhausted");

	D3D12RenderDevice& render_device = getRenderDevice();
	D3D12DescriptorRingBuffer* descriptor_ring = render_device.getCbvSrvUavDescriptorHeap();
	const uint32_t descriptor_size = descriptor_ring->getDescriptorSize();
	const uint32_t descriptor_offset = reserved_srv_range_.offset;

	for (const DescriptorResource& res : resources)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE src_handle = {};

		if (res.resource_type == DescriptorResource::EResourceType::kTexture)
		{
			ASSERT_MSG_RETURN_VOID(res.texture != nullptr, "Descriptor set contains null texture");
			const D3D12Texture& tex = static_cast<const D3D12Texture&>(*res.texture);
			ASSERT_MSG_RETURN_VOID(tex.hasSrv(), "Texture has no SRV descriptor");
			src_handle = tex.getSrvHandle();
		}
		else if (res.resource_type == DescriptorResource::EResourceType::kBuffer)
		{
			ASSERT_MSG_RETURN_VOID(res.buffer != nullptr, "Descriptor set contains null buffer");
			const D3D12Buffer& buf = static_cast<const D3D12Buffer&>(*res.buffer);

			switch (res.view_type)
			{
			case DescriptorResource::EViewType::kSRV:
				ASSERT_MSG_RETURN_VOID(buf.hasSrv(), "Descriptor set contains buffer with no SRV descriptor");
				src_handle = buf.getSrvHandle();
				break;
			case DescriptorResource::EViewType::kCBV:
				ASSERT_MSG_RETURN_VOID(buf.hasCbv(), "Descriptor set contains buffer with no CBV descriptor");
				src_handle = buf.getCbvHandle();
				break;
			case DescriptorResource::EViewType::kUAV:
				ASSERT_MSG_RETURN_VOID(buf.hasUav(), "Descriptor set contains buffer with no UAV descriptor");
				src_handle = buf.getUavHandle();
				break;
			default:
				ASSERT_FAIL("Descriptor set contains unknown view type");
				return;
			}
		}

		D3D12_CPU_DESCRIPTOR_HANDLE dst_handle = reserved_srv_range_.allocation.cpu;
		dst_handle.ptr += static_cast<SIZE_T>(reserved_srv_range_.offset) * descriptor_size;

		render_device.getD3D12Device()->CopyDescriptorsSimple(
			1, dst_handle, src_handle,
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		++reserved_srv_range_.offset;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = reserved_srv_range_.allocation.gpu;
	gpu_handle.ptr += static_cast<UINT64>(descriptor_offset) * descriptor_size;
	command_list_->SetGraphicsRootDescriptorTable(root_parameter_index, gpu_handle);
}


// Input ---------------------------------------------------------------------

void D3D12CommandList::setVertexBuffer(const IBuffer& buffer, uint32_t stride, uint32_t offset)
{
	const D3D12Buffer& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);

	D3D12_VERTEX_BUFFER_VIEW vbv = {};
	vbv.BufferLocation = d3d12_buffer.getResource()->GetGPUVirtualAddress() + offset;
	vbv.SizeInBytes = static_cast<UINT>(d3d12_buffer.getDesc().size - offset);
	vbv.StrideInBytes = stride;

	command_list_->IASetVertexBuffers(0, 1, &vbv);
}


void D3D12CommandList::setIndexBuffer(const IBuffer& buffer, uint32_t offset)
{
	const D3D12Buffer& d3d12_buffer = static_cast<const D3D12Buffer&>(buffer);

	D3D12_INDEX_BUFFER_VIEW ibv = {};
	ibv.BufferLocation = d3d12_buffer.getResource()->GetGPUVirtualAddress() + offset;
	ibv.SizeInBytes = static_cast<UINT>(d3d12_buffer.getDesc().size - offset);
	ibv.Format = DXGI_FORMAT_R32_UINT;

	command_list_->IASetIndexBuffer(&ibv);
}


// Draw ----------------------------------------------------------------------

void D3D12CommandList::draw(uint32_t vertex_count, uint32_t vertex_start_offset)
{
	drawInstanced(vertex_count, 1, vertex_start_offset, 0);
}


void D3D12CommandList::drawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location)
{
	drawIndexedInstanced(index_count, 1, start_index_location, base_vertex_location, 0);
}


void D3D12CommandList::drawInstanced(uint32_t vertex_count_per_instance, uint32_t instance_count,
	uint32_t start_vertex_location, uint32_t start_instance_location)
{
	command_list_->DrawInstanced(vertex_count_per_instance, instance_count, start_vertex_location, start_instance_location);
}


void D3D12CommandList::drawIndexedInstanced(uint32_t index_count_per_instance, uint32_t instance_count,
	uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location)
{
	command_list_->DrawIndexedInstanced(index_count_per_instance, instance_count,
		start_index_location, base_vertex_location, start_instance_location);
}


// Constructor / Destructor --------------------------------------------------

D3D12CommandList::~D3D12CommandList()
{
	D3D12_SAFE_RELEASE(command_list_);
	D3D12_SAFE_RELEASE(command_allocator_);
}


D3D12CommandList::D3D12CommandList(
	IRenderDevice& render_device,
	ECommandListType type,
	ID3D12GraphicsCommandList* command_list,
	ID3D12CommandAllocator* command_allocator)
	: ICommandList(render_device, type)
	, command_list_(command_list)
	, command_allocator_(command_allocator)
	, fence_value_(0)
	, reserved_srv_range_({})
{}


// Helpers -------------------------------------------------------------------

D3D12RenderDevice& D3D12CommandList::getRenderDevice()
{
	return static_cast<D3D12RenderDevice&>(render_device_);
}


void D3D12CommandList::setGpuDebugName()
{
	SET_D3D12_RESOURCE_DEBUG_NAME(command_list_, getName());
	SET_D3D12_RESOURCE_DEBUG_NAME(command_allocator_, ("CommandAllocator_" + getName()).c_str());
}


}  // namespace render
