#include "deferredrenderer.hpp"
#include "interface/gpuresourcemanager.h"
#include "pipelinestatemanager.hpp"

#include <engine/scene/scene.h>
#include <engine/scene/camera.h>
#include <engine/scene/actor.h>
#include <engine/core/utility/assert.h>
#include <engine/core/utility/log.h>
#include <engine/core/asset/gltfloader.h>

#include "interface/pipelinestate.hpp"
#include "interface/renderdevice.h"
#include "interface/commandlist.h"
#include "interface/texture.h"
#include "interface/buffer.h"


namespace render {


static IBuffer* createDebugTriangleVertexBuffer(IRenderDevice& render_device)
{
	static constexpr float kTriangleVertices[] = {
		// position.xyz            // color.rgba
		0.0f,   0.8f,  0.0f,      1.0f, 0.0f, 0.0f, 1.0f,
		1.5f,  -0.8f,  0.0f,      0.0f, 1.0f, 0.0f, 1.0f,
		-1.5f, -0.8f,  0.0f,      0.0f, 0.0f, 1.0f, 1.0f,
	};

	IBuffer::Desc triangle_vertex_desc{};
	triangle_vertex_desc.size = sizeof(kTriangleVertices);
	IBuffer* triangle_vertex_buffer = render_device.getResourceManager().createBuffer(triangle_vertex_desc);
	triangle_vertex_buffer->upload(kTriangleVertices, triangle_vertex_desc.size);
	return triangle_vertex_buffer;
}


static void drawDebugTriangle(
	ICommandList& command_list,
	IPipelineState& triangle_pipeline_state,
	IBuffer& triangle_vertex_buffer,
	ITexture& output_target,
	const math::Matrix4x4& view_projection,
	float aspect)
{
	command_list.setPipelineState(triangle_pipeline_state);
	command_list.setPrimitiveTopology(EPrimitiveTopology::kTriangle);

	struct
	{
		math::Matrix4x4 world;
		math::Matrix4x4 view_projection;
	} constants;

	constants.world = math::Matrix4x4::identity();
	if (aspect > 0.0f)
	{
		constants.world.m00 = 1.0f / aspect;
	}

	constants.view_projection = math::Matrix4x4::identity();
	command_list.setGraphicsRoot32BitConstants(0, 32, &constants);

	command_list.setVertexBuffer(triangle_vertex_buffer, sizeof(float) * 7);
	command_list.draw(3, 0);
}


static ITexture* createTextureFromLoadedTextureData(
	IRenderDevice& render_device,
	const asset::LoadedTextureData& loaded_texture)
{
	if (loaded_texture.width <= 0 || loaded_texture.height <= 0)
	{
		return nullptr;
	}

	if (loaded_texture.component_count != 3 && loaded_texture.component_count != 4)
	{
		return nullptr;
	}

	const uint32_t width = static_cast<uint32_t>(loaded_texture.width);
	const uint32_t height = static_cast<uint32_t>(loaded_texture.height);
	const uint32_t source_channels = static_cast<uint32_t>(loaded_texture.component_count);
	const size_t expected_source_size = static_cast<size_t>(width) * static_cast<size_t>(height) * static_cast<size_t>(source_channels);
	if (loaded_texture.pixel_data.size() < expected_source_size)
	{
		return nullptr;
	}

	ITexture::Desc texture_desc = {};
	texture_desc.width = width;
	texture_desc.height = height;
	texture_desc.depth = 1;
	texture_desc.mipCount = 1;
	texture_desc.format = ETextureFormat::kRGBA8_Unorm;

	ITexture* texture = render_device.getResourceManager().createTexture(texture_desc);

	const uint32_t pixel_count = width * height;
	std::vector<uint8_t> rgba_pixels(static_cast<size_t>(pixel_count) * 4ull, 255);
	for (uint32_t i = 0; i < pixel_count; ++i)
	{
		const size_t src_base = static_cast<size_t>(i) * source_channels;
		const size_t dst_base = static_cast<size_t>(i) * 4ull;
		rgba_pixels[dst_base + 0] = loaded_texture.pixel_data[src_base + 0];
		rgba_pixels[dst_base + 1] = loaded_texture.pixel_data[src_base + 1];
		rgba_pixels[dst_base + 2] = loaded_texture.pixel_data[src_base + 2];
		rgba_pixels[dst_base + 3] = (source_channels == 4) ? loaded_texture.pixel_data[src_base + 3] : 255;
	}

	texture->uploadPixels(rgba_pixels.data(), static_cast<uint32_t>(rgba_pixels.size()));
	return texture;
}


std::unique_ptr<DeferredRenderer> DeferredRenderer::create(IRenderDevice& render_device, const RenderSettings& render_settings)
{
	std::unique_ptr<DeferredRenderer> deferred_renderer(new DeferredRenderer(render_device, render_settings));
	deferred_renderer->initTextures();
	ASSERT_MSG_RETURN_NULL(deferred_renderer->depth_texture_ && deferred_renderer->default_white_texture_,
		"Failed to initialize renderer textures");
	deferred_renderer->initPipelineStates();
	return deferred_renderer;
}


void DeferredRenderer::render(const scene::Scene& scene, const scene::Camera& camera, ITexture& output_target)
{
	ITexture& depth_texture = *depth_texture_;
	ASSERT_MSG_RETURN_VOID(output_target.getWidth() == depth_texture.getWidth() && output_target.getHeight() == depth_texture.getHeight(),
		"Depth and color targets must have the same dimensions");

	const StaticMeshBucket bucket = buildStaticMeshBucket(scene, camera);
	depthPrepass(bucket, depth_texture);
	baseColorPass(bucket, depth_texture, output_target);
	render_device_.executeCommandLists(ECommandListType::kDirect);
}


void DeferredRenderer::resize(uint32_t width, uint32_t height)
{
	ASSERT_MSG_RETURN_VOID(width > 0 && height > 0, "Renderer resize requires a nonzero output size");

	if (render_settings_.render_width == width && render_settings_.render_height == height)
	{
		return;
	}

	initResizableTextures(width, height);
	if (render_settings_.render_width != width || render_settings_.render_height != height)
	{
		LOG_WARNING("DeferredRenderer", "Resize to {}x{} skipped: failed to allocate the replacement depth texture or its DSV.",
			width, height);
	}
}


DeferredRenderer::~DeferredRenderer()
{}


DeferredRenderer::DeferredRenderer(IRenderDevice& render_device, const RenderSettings& render_settings)
	: IRenderer(render_device, render_settings)
{}


void DeferredRenderer::initTextures()
{
	GpuResourceManager& resource_manager = render_device_.getResourceManager();
	ITexture::Desc white_desc = {};
	white_desc.width = 1;
	white_desc.height = 1;
	white_desc.format = ETextureFormat::kRGBA8_Unorm;
	ITexture* white_texture = resource_manager.createTexture(white_desc);
	if (!white_texture)
	{
		return;
	}

	initResizableTextures(render_settings_.render_width, render_settings_.render_height);
	if (!depth_texture_)
	{
		resource_manager.releaseTexture(*white_texture);
		return;
	}

	static constexpr uint8_t kWhitePixel[4] = { 255, 255, 255, 255 };
	white_texture->uploadPixels(kWhitePixel, sizeof(kWhitePixel));
	default_white_texture_ = white_texture;
}


void DeferredRenderer::initResizableTextures(uint32_t width, uint32_t height)
{
	GpuResourceManager& resource_manager = render_device_.getResourceManager();

	ITexture::Desc depth_desc = {};
	depth_desc.width = width;
	depth_desc.height = height;
	depth_desc.format = ETextureFormat::D32_Float;
	ITexture* replacement_depth = resource_manager.createTexture(depth_desc);
	ASSERT_MSG_RETURN_VOID(replacement_depth != nullptr, "Failed to create the depth texture");

	if (depth_texture_)
	{
		resource_manager.releaseTexture(*depth_texture_);
	}

	depth_texture_ = replacement_depth;
	render_settings_.render_width = width;
	render_settings_.render_height = height;
}


void DeferredRenderer::initPipelineStates()
{
	GraphicsPipelineDesc depth_desc = {};
	depth_desc.shader_state.shader_name = "depth_prepass";
	depth_desc.shader_state.shader_stages = EGraphicsShaderStage::kVertex;
	depth_desc.input_layout_state.addAttribute(ESemanticType::kPosition, ETextureFormat::kRGB32_Float);
	depth_desc.resource_layout_state[0] = RootParameter::makeConstants32Bits(0, 32, 0);
	depth_desc.output_merger_state.depth_stencil.format = ETextureFormat::D32_Float;
	depth_prepass_pipeline_state_ = &pipeline_state_manager_.createPipelineState(depth_desc);

	GraphicsPipelineDesc color_desc = depth_desc;
	color_desc.shader_state.shader_name = "base";
	color_desc.shader_state.shader_stages = EGraphicsShaderStage::kVertex | EGraphicsShaderStage::kPixel;
	color_desc.shader_state.addDefine(L"USE_TEXTURE", L"1");
	color_desc.input_layout_state.addAttribute(ESemanticType::kTexCoord, ETextureFormat::kRG32_Float);
	color_desc.input_layout_state.addAttribute(ESemanticType::kColor, ETextureFormat::kRGBA32_Float);
	color_desc.resource_layout_state[1] = RootParameter::makeDescriptorTable({ DescriptorRange{ DescriptorType::kSRV, 0, 1 } });
	color_desc.output_merger_state.addRenderTarget(ETextureFormat::kRGBA8_Unorm);
	color_desc.output_merger_state.depth_stencil.depth_write = false;
	color_desc.output_merger_state.depth_stencil.depth_func = EDepthFunc::kEqual;
	base_color_pipeline_state_ = &pipeline_state_manager_.createPipelineState(color_desc);
}


void DeferredRenderer::depthPrepass(const StaticMeshBucket& bucket, ITexture& depth_texture)
{
	ICommandList& command_list = render_device_.acquireCommandList(ECommandListType::kDirect, "DepthPrepass");
	command_list.setDepthStencil(depth_texture);
	command_list.clearDepthStencil(depth_texture);
	command_list.setPipelineState(*depth_prepass_pipeline_state_);
	drawStaticMeshes(command_list, bucket, false);

	command_list.transitionTexture(depth_texture, EResourceState::kDepthWrite, EResourceState::kDepthRead);
	render_device_.submitCommandList(command_list);
}


void DeferredRenderer::baseColorPass(const StaticMeshBucket& bucket, ITexture& depth_texture, ITexture& output_target)
{
	ICommandList& command_list = render_device_.acquireCommandList(ECommandListType::kDirect, "BaseColorPass");
	command_list.transitionTexture(output_target, EResourceState::kPresent, EResourceState::kRenderTarget);
	command_list.setRenderTarget(output_target, &depth_texture);
	command_list.clearRenderTarget(output_target, 0.1f, 0.1f, 0.1f, 1.0f);
	command_list.setPipelineState(*base_color_pipeline_state_);
	drawStaticMeshes(command_list, bucket, true);

	command_list.transitionTexture(depth_texture, EResourceState::kDepthRead, EResourceState::kDepthWrite);
	command_list.transitionTexture(output_target, EResourceState::kRenderTarget, EResourceState::kPresent);
	render_device_.submitCommandList(command_list);
}


void DeferredRenderer::drawStaticMeshes(ICommandList& command_list, const StaticMeshBucket& bucket, bool bind_base_color) const
{
	command_list.setViewport(0.0f, 0.0f, static_cast<float>(render_settings_.render_width), static_cast<float>(render_settings_.render_height));
	command_list.setScissorRect(0, 0, static_cast<int32_t>(render_settings_.render_width), static_cast<int32_t>(render_settings_.render_height));
	command_list.setPrimitiveTopology(EPrimitiveTopology::kTriangle);

	for (const StaticMeshDraw& draw : bucket)
	{
		command_list.setGraphicsRoot32BitConstants(0, 32, &draw.constants);
		if (bind_base_color)
		{
			command_list.setDescriptorSet(1, { DescriptorResource::textureSRV(draw.mesh.base_color_texture) });
		}
		command_list.setVertexBuffer(*draw.mesh.vertex_buffer, draw.mesh.vertex_stride);
		command_list.setIndexBuffer(*draw.mesh.index_buffer);
		command_list.drawIndexed(draw.mesh.index_count, 0, 0);
	}
}


DeferredRenderer::StaticMeshBucket DeferredRenderer::buildStaticMeshBucket(const scene::Scene& scene, const scene::Camera& camera)
{
	StaticMeshBucket bucket;
	const std::vector<std::shared_ptr<scene::StaticMeshActor>>& static_mesh_actors = scene.getStaticMeshActors();
	bucket.reserve(static_mesh_actors.size());
	const math::Matrix4x4& view_projection = camera.getViewProjectionMatrix();

	for (const std::shared_ptr<scene::StaticMeshActor>& static_mesh_actor : static_mesh_actors)
	{
		if (!static_mesh_actor || !static_mesh_actor->isVisible())
		{
			continue;
		}

		const CameraObjectConstants constants = { static_mesh_actor->getTransform().toMatrix4x4(), view_projection };
		for (const scene::MeshSlot& slot : static_mesh_actor->getMeshRenderer().getMeshSlots())
		{
			const RenderMesh* mesh = getOrCreateRenderMesh(slot.mesh);
			if (mesh)
			{
				ASSERT_MSG_RETURN(mesh->base_color_texture != nullptr, bucket, "Base color texture is not initialized");
				bucket.push_back({ *mesh, constants });
			}
		}
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
			entry.material_index = primitive_data.material_index;
			entry.mesh_data = primitive_data.mesh_data;
			cached_primitives.push_back(std::move(entry));
		}
		file_cache_it = file_primitives_cache_.emplace(mesh.resource_path, std::move(cached_primitives)).first;

		file_materials_cache_.emplace(mesh.resource_path, loaded_model.materials);

		std::vector<ITexture*> loaded_textures;
		loaded_textures.reserve(loaded_model.textures.size());
		for (const asset::LoadedTextureData& loaded_texture : loaded_model.textures)
		{
			ITexture* texture = createTextureFromLoadedTextureData(render_device_, loaded_texture);
			loaded_textures.push_back(texture);
		}
		file_textures_cache_.emplace(mesh.resource_path, std::move(loaded_textures));
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
	gpu_data.base_color_texture = default_white_texture_;

	auto materials_it = file_materials_cache_.find(mesh.resource_path);
	auto textures_it = file_textures_cache_.find(mesh.resource_path);
	if (materials_it != file_materials_cache_.end() && textures_it != file_textures_cache_.end())
	{
		const std::vector<asset::LoadedMaterialData>& materials = materials_it->second;
		const std::vector<ITexture*>& textures = textures_it->second;

		if (primitive_it->material_index >= 0 && static_cast<uint32_t>(primitive_it->material_index) < materials.size())
		{
			const asset::LoadedMaterialData& material = materials[primitive_it->material_index];
			if (material.base_color_texture_index >= 0 && static_cast<uint32_t>(material.base_color_texture_index) < textures.size())
			{
				ITexture* texture = textures[material.base_color_texture_index];
				if (texture != nullptr)
				{
					gpu_data.base_color_texture = texture;
				}
			}
		}
	}

	IBuffer::Desc vertex_desc{};
	vertex_desc.size = sizeof(asset::LoadedMeshData::Vertex) * loaded_mesh.vertices.size();
	IBuffer* vertex_buffer = render_device_.getResourceManager().createBuffer(vertex_desc);
	vertex_buffer->upload(loaded_mesh.vertices.data(), vertex_desc.size);
	gpu_data.vertex_buffer = vertex_buffer;

	IBuffer::Desc index_desc{};
	index_desc.size = sizeof(uint32_t) * loaded_mesh.indices.size();
	IBuffer* index_buffer = render_device_.getResourceManager().createBuffer(index_desc);
	index_buffer->upload(loaded_mesh.indices.data(), index_desc.size);
	gpu_data.index_buffer = index_buffer;

	auto [inserted_it, inserted] = render_meshes_.emplace(mesh_cache_key, std::move(gpu_data));
	ASSERT(inserted);
	return &inserted_it->second;
}

}  // namespace render
