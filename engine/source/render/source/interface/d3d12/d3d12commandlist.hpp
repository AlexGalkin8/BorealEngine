#pragma once

#include "interface/commandlist.h"
#include "interface/fence.hpp"
#include "interface/resourcelayoutstate.hpp"
#include "d3d12descriptorheap.hpp"

#include <bitset>
#include <memory>
#include <initializer_list>
#include <vector>


struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList;
struct ID3D12CommandQueue;
struct ID3D12RootSignature;

namespace render {
class IRenderDevice;
class D3D12RenderDevice;
}  // namespace render


namespace render {

class D3D12CommandList final : public ICommandList
{
	friend class ICommandList;
	template<ECommandListType> friend class D3D12CommandQueue;
	template<ECommandListType> friend class D3D12CommandListPool;

public:
	static std::unique_ptr<D3D12CommandList> create(IRenderDevice& render_device, ECommandListType type);

	void beginRecording();
	void endRecording();

	ECommandListType getType() const { return type_; }
	ID3D12GraphicsCommandList* getD3D12CommandList() const { return command_list_; }

	// Resource barriers
	void transitionTexture(ITexture& texture, EResourceState from, EResourceState to) override;

	// Output
	void setRenderTarget(ITexture& render_target, ITexture* depth_stencil = nullptr) override;
	void setRenderTargets(std::initializer_list<ITexture*> render_targets, ITexture* depth_stencil = nullptr) override;
	void setDepthStencil(ITexture& depth_stencil) override;
	void clearRenderTarget(ITexture& render_target, float r, float g, float b, float a) override;
	void clearDepthStencil(ITexture& depth_stencil, float depth = 1.0f, uint8_t stencil = 0) override;
	void setViewport(float x, float y, float width, float height) override;
	void setScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) override;

	// Pipeline
	void setPipelineState(IPipelineState& pipeline_state) override;
	void setPrimitiveTopology(EPrimitiveTopology topology) override;
	void setGraphicsRoot32BitConstants(
		uint32_t root_parameter_index,
		uint32_t num_32bit_values,
		const void* data,
		uint32_t dest_offset_32bit_values = 0) override;

	void setDescriptorSet(uint32_t root_parameter_index, std::initializer_list<DescriptorResource> resources) override;

	// Input
	void setVertexBuffer(const IBuffer& buffer, uint32_t stride, uint32_t offset = 0) override;
	void setIndexBuffer(const IBuffer& buffer, uint32_t offset = 0) override;

	// Draw
	void draw(uint32_t vertex_count, uint32_t vertex_start_offset) override;
	void drawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) override;
	void drawInstanced(
		uint32_t vertex_count_per_instance,
		uint32_t instance_count,
		uint32_t start_vertex_location,
		uint32_t start_instance_location) override;
	void drawIndexedInstanced(
		uint32_t index_count_per_instance,
		uint32_t instance_count,
		uint32_t start_index_location,
		int32_t base_vertex_location,
		uint32_t start_instance_location) override;

	~D3D12CommandList() override;

private:
	D3D12CommandList(
		IRenderDevice& render_device,
		ECommandListType type,
		ID3D12GraphicsCommandList* command_list,
		ID3D12CommandAllocator* command_allocator);

	D3D12RenderDevice& getRenderDevice();
	// Keep descriptor tables allocated until the fence for this submission completes.
	void recordDescriptorTableUsage(FenceValue fence_value);

	void setGpuDebugName() override;

private:
	ID3D12GraphicsCommandList* command_list_;
	ID3D12CommandAllocator* command_allocator_;
	FenceValue fence_value_;
	std::vector<D3D12ShaderVisibleDescriptorHeap::Range> descriptor_tables_;
	ID3D12RootSignature* graphics_root_signature_ = nullptr;
	std::bitset<kMaxRootParameters> failed_descriptor_tables_;
	bool descriptor_exhaustion_logged_ = false;
};

}  // namespace render
