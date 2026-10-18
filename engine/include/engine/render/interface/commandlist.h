#pragma once

#include "common.h"

#include <cstdint>
#include <memory>
#include <initializer_list>


namespace render {
class ITexture;
class IBuffer;
class IPipelineState;
class ISwapChain;
class IRenderDevice;
}  // namespace render


namespace render {

struct DescriptorResource
{
	enum class EResourceType { kTexture, kBuffer };
	enum class EViewType { kSRV, kCBV, kUAV };

	EResourceType resource_type;
	EViewType     view_type = EViewType::kSRV;

	union
	{
		const ITexture* texture;
		const IBuffer* buffer;
	};

	static DescriptorResource textureSRV(const ITexture* t)
	{
		DescriptorResource r;
		r.resource_type = EResourceType::kTexture;
		r.view_type = EViewType::kSRV;
		r.texture = t;
		return r;
	}

	static DescriptorResource bufferSRV(const IBuffer* b)
	{
		DescriptorResource r;
		r.resource_type = EResourceType::kBuffer;
		r.view_type = EViewType::kSRV;
		r.buffer = b;
		return r;
	}

	static DescriptorResource bufferCBV(const IBuffer* b)
	{
		DescriptorResource r;
		r.resource_type = EResourceType::kBuffer;
		r.view_type = EViewType::kCBV;
		r.buffer = b;
		return r;
	}

	static DescriptorResource bufferUAV(const IBuffer* b)
	{
		DescriptorResource r;
		r.resource_type = EResourceType::kBuffer;
		r.view_type = EViewType::kUAV;
		r.buffer = b;
		return r;
	}
};


enum class EResourceState
{
	kCommon = 0,
	kRenderTarget,
	kPresent,
	kCopyDest,
	kCopySrc,
	kGenericRead,
	kDepthWrite,
	kDepthRead,
	kShaderResource,
	kUnorderedAccess,
};


class ICommandList : public internal::RenderObject
{
public:
	// Resource barriers
	virtual void transitionTexture(ITexture& texture, EResourceState from, EResourceState to) = 0;

	// Output
	virtual void setRenderTarget(ITexture& render_target, ITexture* depth_stencil = nullptr) = 0;
	virtual void clearRenderTarget(ITexture& render_target, float r, float g, float b, float a) = 0;
	virtual void clearDepthStencil(ITexture& depth_stencil, float depth = 1.0f, uint8_t stencil = 0) = 0;
	virtual void setViewport(float x, float y, float width, float height) = 0;
	virtual void setScissorRect(int32_t left, int32_t top, int32_t right, int32_t bottom) = 0;

	// Pipeline
	virtual void setPipelineState(IPipelineState& pipeline_state) = 0;
	virtual void setPrimitiveTopology(EPrimitiveTopology topology) = 0;
	virtual void setGraphicsRoot32BitConstants(uint32_t root_parameter_index, uint32_t num_32bit_values, const void* data, uint32_t dest_offset_32bit_values = 0) = 0;
	virtual void setDescriptorSet(uint32_t root_parameter_index, std::initializer_list<DescriptorResource> resources) = 0;

	// Input
	virtual void setVertexBuffer(const IBuffer& buffer, uint32_t stride, uint32_t offset = 0) = 0;
	virtual void setIndexBuffer(const IBuffer& buffer, uint32_t offset = 0) = 0;

	// Draw
	virtual void draw(uint32_t vertex_count, uint32_t vertex_start_offset) = 0;
	virtual void drawIndexed(uint32_t index_count, uint32_t start_index_location, int32_t base_vertex_location) = 0;
	virtual void drawInstanced(uint32_t vertex_count_per_instance, uint32_t instance_count,
		uint32_t start_vertex_location, uint32_t start_instance_location) = 0;
	virtual void drawIndexedInstanced(uint32_t index_count_per_instance, uint32_t instance_count,
		uint32_t start_index_location, int32_t base_vertex_location, uint32_t start_instance_location) = 0;

	virtual ~ICommandList();

protected:
	ICommandList(IRenderDevice& render_device, ECommandListType type);

protected:
	IRenderDevice& render_device_;
	bool is_recording_;
	ECommandListType type_;
};

}  // namespace render
