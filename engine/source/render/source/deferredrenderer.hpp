#pragma once

#include "renderer.h"

#include <engine/core/asset/gltfloader.h>
#include <engine/math/matrix.h>
#include <engine/render/interface/texture.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace scene {
class Scene;
class Camera;
class StaticMeshActor;
struct MeshHandle;
struct MeshSlot;
}  // namespace scene

namespace render {
class IPipelineState;
class IDeviceContext;
class ISwapChain;
class IBuffer;
class ITexture;
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

	struct PrimitiveCacheEntry
	{
		uint32_t mesh_index = 0;
		uint32_t primitive_index = 0;
		asset::LoadedMeshData mesh_data;
	};

private:
	DeferredRenderer(IRenderDevice& render_device);

	void initPipelineStates();
	void ensureDepthTexture(const ITexture::Desc& target_desc);

	void GBufferPass(const scene::Scene& scene, const scene::Camera& camera, IDeviceContext& ctx, ISwapChain& swap_chain);
	using StaticMeshBucket = std::vector<const scene::StaticMeshActor*>;
	StaticMeshBucket buildStaticMeshBucket(const scene::Scene& scene) const;
	void drawStaticMeshBucket(const StaticMeshBucket& bucket, const scene::Camera& camera, IDeviceContext& ctx);
	void drawDebugTriangle(IDeviceContext& ctx, float aspect);
	const RenderMesh* getOrCreateRenderMesh(const scene::MeshHandle& mesh);

	IPipelineState* mesh_pipeline_state_ = nullptr;
	IPipelineState* triangle_pipeline_state_ = nullptr;
	IBuffer* triangle_vertex_buffer_ = nullptr;
	ITexture* depth_texture_ = nullptr;
	std::unordered_map<std::string, RenderMesh> render_meshes_;
	std::unordered_map<std::string, std::vector<PrimitiveCacheEntry>> file_primitives_cache_;
};

}  // namespace render
