#pragma once

#include "renderer.h"

#include <engine/math/matrix.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace scene {
class Scene;
class Camera;
class RenderableActor;
class StaticMeshActor;
}  // namespace scene

namespace render {
class IPipelineState;
class IDeviceContext;
class ISwapChain;
class IBuffer;
}  // namespace render


namespace render {

class DeferredRenderer final : public IRenderer
{
public:
	static std::unique_ptr<DeferredRenderer> create(IRenderDevice& render_device);

	void render(const scene::Scene& scene, const scene::Camera& camera, IDeviceContext& ctx, ISwapChain& swap_chain) override;

	~DeferredRenderer() override;

private:
	struct CameraObjectConstants
	{
		math::Matrix4x4 world;
		math::Matrix4x4 view_projection;
	};

	struct RenderMesh
	{
		IBuffer* vertex_buffer = nullptr;
		IBuffer* index_buffer = nullptr;
		uint32_t vertex_stride = 0;
		uint32_t index_count = 0;
	};

	using StaticMeshBucket = std::vector<const scene::StaticMeshActor*>;

private:
	DeferredRenderer(IRenderDevice& render_device);

	void initPipelineStates();
	void initGeometry();

	void GBufferPass(const scene::Scene& scene, const scene::Camera& camera, IDeviceContext& ctx, ISwapChain& swap_chain);
	StaticMeshBucket buildStaticMeshBucket(const scene::Scene& scene) const;
	void drawStaticMeshBucket(const StaticMeshBucket& bucket, const scene::Camera& camera, IDeviceContext& ctx);
	const RenderMesh* getRenderMeshes(const scene::StaticMeshActor& actor);

	IPipelineState* base_pipeline_state_ = nullptr;
	IBuffer* triangle_vertex_buffer_ = nullptr;
	std::unordered_map<std::string, RenderMesh> render_meshes_;
};

}  // namespace render
