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

std::unique_ptr<DeferredRenderer> DeferredRenderer::create(IRenderDevice& render_device)
{
	std::unique_ptr<DeferredRenderer> deferred_renderer;
	deferred_renderer.reset(new DeferredRenderer(render_device));
	deferred_renderer->initPipelineStates();
	deferred_renderer->initGeometry();
	return deferred_renderer;
}


void DeferredRenderer::render(const scene::Scene& scene, const scene::Camera& camera, IDeviceContext& ctx, ISwapChain& swap_chain)
{
	GBufferPass(scene, camera, ctx, swap_chain);
}


DeferredRenderer::StaticMeshBucket DeferredRenderer::buildStaticMeshBucket(const scene::Scene& scene) const
{
	StaticMeshBucket bucket;
	const auto& renderables = scene.getRenderableActors();
	bucket.reserve(renderables.size());

	for (const auto& renderable : renderables)
	{
		if (!renderable || !renderable->isVisible())
		{
			continue;
		}

		auto static_mesh_actor = std::dynamic_pointer_cast<scene::StaticMeshActor>(renderable);
		if (!static_mesh_actor)
		{
			continue;
		}

		bucket.push_back(static_mesh_actor.get());
	}

	return bucket;
}


const DeferredRenderer::RenderMesh* DeferredRenderer::getRenderMeshes(const scene::StaticMeshActor& actor)
{
	const scene::MeshHandle& mesh = actor.getMesh();
	if (!mesh.hasResourcePath())
	{
		return nullptr;
	}

	auto it = render_meshes_.find(mesh.resource_path);
	if (it != render_meshes_.end())
	{
		return &it->second;
	}

	asset::LoadedMeshData loaded_mesh = asset::loadStaticMeshFromGltf(mesh.resource_path);
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

	auto [inserted_it, inserted] = render_meshes_.emplace(mesh.resource_path, gpu_data);
	ASSERT(inserted);
	return &inserted_it->second;
}


void DeferredRenderer::drawStaticMeshBucket(const StaticMeshBucket& bucket, const scene::Camera& camera, IDeviceContext& ctx)
{
	ctx.setPipelineState(*base_pipeline_state_);
	ctx.setPrimitiveTopology(EPrimitiveTopology::kTriangle);

	for (const scene::StaticMeshActor* static_mesh_actor : bucket)
	{
		CameraObjectConstants constants{};
		constants.world = static_mesh_actor->getTransform().toMatrix4x4();
		constants.view_projection = camera.getViewProjectionMatrix();
		ctx.setGraphicsRoot32BitConstants(0, 32, &constants);

		const RenderMesh* imported_mesh = getRenderMeshes(*static_mesh_actor);
		if (imported_mesh != nullptr)
		{
			ctx.setVertexBuffer(*imported_mesh->vertex_buffer, imported_mesh->vertex_stride);
			ctx.setIndexBuffer(*imported_mesh->index_buffer);
			ctx.drawIndexed(imported_mesh->index_count, 0, 0);
			continue;
		}

		switch (static_mesh_actor->getMesh().primitive)
		{
		case scene::EMeshPrimitive::kTriangle:
			ctx.setVertexBuffer(*triangle_vertex_buffer_, sizeof(float) * 5);
			ctx.draw(3, 0);
			break;
		default:
			break;
		}
	}
}


void DeferredRenderer::GBufferPass(const scene::Scene& scene, const scene::Camera& camera, IDeviceContext& ctx, ISwapChain& swap_chain)
{
	ITexture& back_buffer = swap_chain.getCurrentBackBuffer();
	const ITexture::Desc& bb_desc = back_buffer.getDesc();

	ctx.transitionTexture(back_buffer, EResourceState::kPresent, EResourceState::kRenderTarget);
	ctx.setRenderTarget(back_buffer);
	ctx.clearRenderTarget(back_buffer, 0.1f, 0.1f, 0.1f, 1.0f);
	ctx.setViewport(0.0f, 0.0f, static_cast<float>(bb_desc.width), static_cast<float>(bb_desc.height));
	ctx.setScissorRect(0, 0, static_cast<int32_t>(bb_desc.width), static_cast<int32_t>(bb_desc.height));

	StaticMeshBucket static_mesh_bucket = buildStaticMeshBucket(scene);
	drawStaticMeshBucket(static_mesh_bucket, camera, ctx);

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
	GraphicsPipelineDesc desc = {};
	desc.shader_state.shader_name = "base";

	desc.input_layout_state.topology = EPrimitiveTopology::kTriangle;
	desc.input_layout_state.addAttribute(ESemanticType::kPosition, ETextureFormat::kRGB32_Float);
	desc.input_layout_state.addAttribute(ESemanticType::kTexCoord, ETextureFormat::kRG32_Float);

	// b0: float4x4 World + float4x4 ViewProjection (32 x 32-bit constants)
	desc.resource_layout_state[0] = RootParameter::makeConstants32Bits(0, 32, 0);

	desc.rasterizer_state.cull_mode = ECullMode::kNone;
	desc.rasterizer_state.depth_clip_enable = false;
	desc.rasterizer_state.fill_mode = EFillMode::kSolid;

	desc.output_merger_state.addRenderTarget(ETextureFormat::kRGBA8_Unorm);
	desc.output_merger_state.depth_stencil.depth_enable = false;
	desc.output_merger_state.depth_stencil.stencil_enable = false;

	base_pipeline_state_ = &pipeline_state_manager_.createPipelineState(desc);
}


void DeferredRenderer::initGeometry()
{
	struct Vertex
	{
		float x, y, z;
		float u, v;
	};

	const Vertex vertices[3] =
	{
		{  0.0f,  0.6666667f, 1.0f,  0.5f, 0.0f },
		{  0.5f, -0.3333333f, 1.0f,  1.0f, 1.0f },
		{ -0.5f, -0.3333333f, 1.0f,  0.0f, 1.0f },
	};

	IBuffer::Desc desc = {};
	desc.size = sizeof(vertices);

	triangle_vertex_buffer_ = resource_manager_.createBuffer(desc);
	triangle_vertex_buffer_->upload(vertices, sizeof(vertices));
}

}  // namespace render