#include "deferredrenderer.hpp"
#include "resourcemanager.hpp"
#include "pipelinestatemanager.hpp"

#include <engine/scene/scene.h>
#include <engine/scene/camera.h>
#include <engine/scene/actor.h>
#include <engine/core/utility/assert.h>
#include <engine/core/asset/gltfloader.h>

#include "interface/pipelinestate.hpp"
#include "interface/renderdevice.h"
#include "interface/devicecontext.h"
#include "interface/swapchain.h"
#include "interface/texture.h"
#include "interface/buffer.h"


namespace render {

static IBuffer* createDebugTriangleVertexBuffer(IResourceManager& resource_manager)
{
	static constexpr float kTriangleVertices[] = {
		// position.xyz            // color.rgba
		0.0f,   0.8f,  0.0f,      1.0f, 0.0f, 0.0f, 1.0f,
		1.5f,  -0.8f,  0.0f,      0.0f, 1.0f, 0.0f, 1.0f,
		-1.5f, -0.8f,  0.0f,      0.0f, 0.0f, 1.0f, 1.0f,
	};

	IBuffer::Desc triangle_vertex_desc{};
	triangle_vertex_desc.size = sizeof(kTriangleVertices);
	IBuffer* triangle_vertex_buffer = resource_manager.createBuffer(triangle_vertex_desc);
	triangle_vertex_buffer->upload(kTriangleVertices, triangle_vertex_desc.size);
	return triangle_vertex_buffer;
}

std::unique_ptr<DeferredRenderer> DeferredRenderer::create(IRenderDevice& render_device)
{
	std::unique_ptr<DeferredRenderer> deferred_renderer;
	deferred_renderer.reset(new DeferredRenderer(render_device));
	deferred_renderer->initPipelineStates();
	deferred_renderer->triangle_vertex_buffer_ = createDebugTriangleVertexBuffer(deferred_renderer->resource_manager_);
	return deferred_renderer;
}


void DeferredRenderer::render(const scene::Scene& scene, const scene::Camera& camera, IDeviceContext& ctx, ISwapChain& swap_chain)
{
	GBufferPass(scene, camera, ctx, swap_chain);
}


DeferredRenderer::StaticMeshBucket DeferredRenderer::buildStaticMeshBucket(const scene::Scene& scene) const
{
	StaticMeshBucket bucket;
	const auto& static_mesh_actors = scene.getStaticMeshActors();
	bucket.reserve(static_mesh_actors.size());

	for (const auto& static_mesh_actor : static_mesh_actors)
	{
		if (!static_mesh_actor || !static_mesh_actor->isVisible())
		{
			continue;
		}

		bucket.push_back(static_mesh_actor.get());
	}

	return bucket;
}


const DeferredRenderer::RenderMesh* DeferredRenderer::getOrCreateRenderMesh(const scene::MeshHandle& mesh)
{
	if (!mesh.hasResourcePath())
	{
		return nullptr;
	}

	const std::string mesh_cache_key = mesh.resource_path + "#m" + std::to_string(mesh.mesh_index) + "p" + std::to_string(mesh.primitive_index);
	auto it = render_meshes_.find(mesh_cache_key);
	if (it != render_meshes_.end())
	{
		return &it->second;
	}

	auto file_cache_it = file_primitives_cache_.find(mesh.resource_path);
	if (file_cache_it == file_primitives_cache_.end())
	{
		const asset::LoadedModelData loaded_model = asset::loadModelFromGltf(mesh.resource_path);
		std::vector<PrimitiveCacheEntry> cached_primitives;
		cached_primitives.reserve(loaded_model.primitives.size());
		for (const asset::LoadedPrimitiveData& primitive_data : loaded_model.primitives)
		{
			PrimitiveCacheEntry entry{};
			entry.mesh_index = primitive_data.mesh_index;
			entry.primitive_index = primitive_data.primitive_index;
			entry.mesh_data = primitive_data.mesh_data;
			cached_primitives.push_back(std::move(entry));
		}

		file_cache_it = file_primitives_cache_.emplace(mesh.resource_path, std::move(cached_primitives)).first;
	}

	const std::vector<PrimitiveCacheEntry>& cached_primitives = file_cache_it->second;
	auto primitive_it = std::find_if(cached_primitives.begin(), cached_primitives.end(), [&mesh](const PrimitiveCacheEntry& entry)
	{
		return entry.mesh_index == mesh.mesh_index && entry.primitive_index == mesh.primitive_index;
	});
	ASSERT_MSG_RETURN(primitive_it != cached_primitives.end(), nullptr, "Requested primitive was not found in glTF cache");

	const asset::LoadedMeshData& loaded_mesh = primitive_it->mesh_data;
	ASSERT_MSG_RETURN(!loaded_mesh.vertices.empty() && !loaded_mesh.indices.empty(), nullptr, "Failed to load mesh data from glTF file");

	RenderMesh gpu_data{};
	gpu_data.vertex_stride = sizeof(asset::LoadedMeshData::Vertex);
	gpu_data.index_count = static_cast<uint32_t>(loaded_mesh.indices.size());

	IBuffer::Desc vertex_desc{};
	vertex_desc.size = sizeof(asset::LoadedMeshData::Vertex) * loaded_mesh.vertices.size();
	gpu_data.vertex_buffer = resource_manager_.createBuffer(vertex_desc);
	gpu_data.vertex_buffer->upload(loaded_mesh.vertices.data(), vertex_desc.size);

	IBuffer::Desc index_desc{};
	index_desc.size = sizeof(uint32_t) * loaded_mesh.indices.size();
	gpu_data.index_buffer = resource_manager_.createBuffer(index_desc);
	gpu_data.index_buffer->upload(loaded_mesh.indices.data(), index_desc.size);

	auto [inserted_it, inserted] = render_meshes_.emplace(mesh_cache_key, gpu_data);
	ASSERT(inserted);
	return &inserted_it->second;
}


void DeferredRenderer::drawStaticMeshBucket(const StaticMeshBucket& bucket, const scene::Camera& camera, IDeviceContext& ctx)
{
	ctx.setPipelineState(*mesh_pipeline_state_);
	ctx.setPrimitiveTopology(EPrimitiveTopology::kTriangle);

	for (const scene::StaticMeshActor* static_mesh_actor : bucket)
	{
		CameraObjectConstants constants{};
		constants.world = static_mesh_actor->getTransform().toMatrix4x4();
		constants.view_projection = camera.getViewProjectionMatrix();
		ctx.setGraphicsRoot32BitConstants(0, 32, &constants);

		const auto& mesh_slots = static_mesh_actor->getMeshRenderer().getMeshSlots();
		for (const scene::MeshSlot& slot : mesh_slots)
		{
			const RenderMesh* imported_mesh = getOrCreateRenderMesh(slot.mesh);
			if (imported_mesh != nullptr)
			{
				ctx.setVertexBuffer(*imported_mesh->vertex_buffer, imported_mesh->vertex_stride);
				ctx.setIndexBuffer(*imported_mesh->index_buffer);
				ctx.drawIndexed(imported_mesh->index_count, 0, 0);
				continue;
			}

			switch (slot.mesh.primitive)
			{
			case scene::EMeshPrimitive::kTriangle:
				ctx.setVertexBuffer(*triangle_vertex_buffer_, sizeof(float) * 7);
				ctx.draw(3, 0);
				break;
			default:
				break;
			}
		}
	}
}

void DeferredRenderer::drawDebugTriangle(IDeviceContext& ctx, float aspect)
{
	ASSERT_MSG_RETURN_VOID(triangle_vertex_buffer_ != nullptr, "Triangle vertex buffer is not initialized");

	ctx.setPipelineState(*triangle_pipeline_state_);
	ctx.setPrimitiveTopology(EPrimitiveTopology::kTriangle);

	CameraObjectConstants constants{};
	constants.world = math::Matrix4x4::identity();
	if (aspect > 0.0f)
	{
		constants.world.m00 = 1.0f / aspect;
	}
	constants.view_projection = math::Matrix4x4::identity();
	ctx.setGraphicsRoot32BitConstants(0, 32, &constants);

	ctx.setVertexBuffer(*triangle_vertex_buffer_, sizeof(float) * 7);
	ctx.draw(3, 0);
}


void DeferredRenderer::ensureDepthTexture(const ITexture::Desc& target_desc)
{
	if (depth_texture_ != nullptr)
	{
		const ITexture::Desc& depth_desc = depth_texture_->getDesc();
		if (depth_desc.width == target_desc.width && depth_desc.height == target_desc.height)
		{
			return;
		}
	}

	ITexture::Desc depth_desc = {};
	depth_desc.width = target_desc.width;
	depth_desc.height = target_desc.height;
	depth_desc.depth = 1;
	depth_desc.mipCount = 1;
	depth_desc.format = ETextureFormat::D32_Float;
	depth_texture_ = resource_manager_.createTexture(depth_desc);
}


void DeferredRenderer::GBufferPass(const scene::Scene& scene, const scene::Camera& camera, IDeviceContext& ctx, ISwapChain& swap_chain)
{
	ITexture& back_buffer = swap_chain.getCurrentBackBuffer();
	const ITexture::Desc& bb_desc = back_buffer.getDesc();

	ensureDepthTexture(bb_desc);

	ctx.transitionTexture(back_buffer, EResourceState::kPresent, EResourceState::kRenderTarget);
	ctx.setRenderTarget(back_buffer, depth_texture_);
	ctx.clearRenderTarget(back_buffer, 0.1f, 0.1f, 0.1f, 1.0f);
	ctx.clearDepthStencil(*depth_texture_);
	ctx.setViewport(0.0f, 0.0f, static_cast<float>(bb_desc.width), static_cast<float>(bb_desc.height));
	ctx.setScissorRect(0, 0, static_cast<int32_t>(bb_desc.width), static_cast<int32_t>(bb_desc.height));
	
#if 0
	// Debug: clear back buffer with a solid color
	const float aspect = static_cast<float>(bb_desc.width) / static_cast<float>(bb_desc.height);
	drawDebugTriangle(ctx, aspect);
#else
	StaticMeshBucket static_mesh_bucket = buildStaticMeshBucket(scene);
	drawStaticMeshBucket(static_mesh_bucket, camera, ctx);
#endif

	ctx.transitionTexture(back_buffer, EResourceState::kRenderTarget, EResourceState::kPresent);
}


DeferredRenderer::~DeferredRenderer()
{

}


DeferredRenderer::DeferredRenderer(IRenderDevice& render_device)
	: IRenderer(render_device)
{
}


void DeferredRenderer::initPipelineStates()
{
	GraphicsPipelineDesc mesh_desc = {};
	mesh_desc.shader_state.shader_name = "base";
	mesh_desc.shader_state.addDefine(L"USE_MESH_VERTEX_COLOR", L"1");
	mesh_desc.input_layout_state.topology = EPrimitiveTopology::kTriangle;
	mesh_desc.input_layout_state.addAttribute(ESemanticType::kPosition, ETextureFormat::kRGB32_Float);
	mesh_desc.input_layout_state.addAttribute(ESemanticType::kTexCoord, ETextureFormat::kRG32_Float);
	mesh_desc.input_layout_state.addAttribute(ESemanticType::kColor, ETextureFormat::kRGBA32_Float);
	mesh_desc.resource_layout_state[0] = RootParameter::makeConstants32Bits(0, 32, 0);
	mesh_desc.rasterizer_state.cull_mode = ECullMode::kNone;
	mesh_desc.rasterizer_state.depth_clip_enable = true;
	mesh_desc.rasterizer_state.fill_mode = EFillMode::kWireframe;
	mesh_desc.output_merger_state.addRenderTarget(ETextureFormat::kRGBA8_Unorm);
	mesh_desc.output_merger_state.depth_stencil.format = ETextureFormat::D32_Float;
	mesh_desc.output_merger_state.depth_stencil.depth_enable = true;
	mesh_desc.output_merger_state.depth_stencil.depth_write = true;
	mesh_desc.output_merger_state.depth_stencil.depth_func = EDepthFunc::kLess;
	mesh_desc.output_merger_state.depth_stencil.stencil_enable = false;
	mesh_pipeline_state_ = &pipeline_state_manager_.createPipelineState(mesh_desc);

	GraphicsPipelineDesc triangle_desc = mesh_desc;
	triangle_desc.shader_state.addDefine(L"USE_VERTEX_COLOR", L"1");
	triangle_desc.input_layout_state.vertex_attributes.clear();
	triangle_desc.input_layout_state.addAttribute(ESemanticType::kPosition, ETextureFormat::kRGB32_Float);
	triangle_desc.input_layout_state.addAttribute(ESemanticType::kColor, ETextureFormat::kRGBA32_Float);
	triangle_desc.rasterizer_state.fill_mode = EFillMode::kSolid;
	triangle_pipeline_state_ = &pipeline_state_manager_.createPipelineState(triangle_desc);
}

}  // namespace render