#include "deferredrenderer.hpp"
#include "resourcemanager.hpp"
#include "pipelinestatemanager.hpp"

#include <engine/scene/scene.h>
#include <engine/scene/camera.h>
#include <engine/scene/actor.h>

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


void DeferredRenderer::drawStaticMeshBucket(const StaticMeshBucket& bucket, const scene::Camera& camera, IDeviceContext& ctx)
{
	ctx.setPipelineState(*base_pipeline_state_);
	ctx.setPrimitiveTopology(EPrimitiveTopology::kTriangle);
	ctx.setVertexBuffer(*triangle_vertex_buffer_, sizeof(float) * 5);

	for (const scene::StaticMeshActor* static_mesh_actor : bucket)
	{
		switch (static_mesh_actor->getMesh().primitive)
		{
		case scene::EMeshPrimitive::kTriangle:
		{
			CameraObjectConstants constants{};
			constants.world = static_mesh_actor->getTransform().toMatrix4x4();
			constants.view_projection = camera.getViewProjectionMatrix();
			ctx.setGraphicsRoot32BitConstants(0, 32, &constants.world[0][0]);
			ctx.draw(3, 0);
			break;
		}
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
	desc.rasterizer_state.depth_clip_enable = true;
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

	// Triangle centered at local origin, in front of camera (LH +Z forward)
	// Centroid is at (0, 0, 1), so world rotation spins around its own axis.
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