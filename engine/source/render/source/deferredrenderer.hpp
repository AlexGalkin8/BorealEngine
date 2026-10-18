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
class ICommandList;
class ISwapChain;
class IBuffer;
class ITexture;
}  // namespace render


namespace render {

class DeferredRenderer final : public IRenderer
{
public:
	static std::unique_ptr<DeferredRenderer> create(IRenderDevice& render_device, const RenderSettings& render_settings);

	void render(const scene::Scene& scene, const scene::Camera& camera, ITexture& output_target) override;

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
		ITexture* base_color_texture = nullptr;
		uint32_t vertex_stride = 0;
		uint32_t index_count = 0;
	};

	struct PrimitiveCacheEntry
	{
		uint32_t mesh_index = 0;
		uint32_t primitive_index = 0;
		int32_t material_index = -1;
		asset::LoadedMeshData mesh_data;
	};

private:
	DeferredRenderer(IRenderDevice& render_device, const RenderSettings& render_settings);

	void initTextures();
	void initPipelineStates();

	void GBufferPass(const scene::Scene& scene, const scene::Camera& camera, ITexture& output_target);

	using StaticMeshBucket = std::vector<const scene::StaticMeshActor*>;
	StaticMeshBucket buildStaticMeshBucket(const scene::Scene& scene) const;
	const RenderMesh* getOrCreateRenderMesh(const scene::MeshHandle& mesh);

private:
	IPipelineState* mesh_pipeline_state_ = nullptr;

	ITexture* depth_texture_ = nullptr;
	ITexture* default_white_texture_ = nullptr;

	std::unordered_map<std::string, RenderMesh> render_meshes_;
	std::unordered_map<std::string, std::vector<PrimitiveCacheEntry>> file_primitives_cache_;
	std::unordered_map<std::string, std::vector<asset::LoadedMaterialData>> file_materials_cache_;
	std::unordered_map<std::string, std::vector<ITexture*>> file_textures_cache_;
};

}  // namespace render
