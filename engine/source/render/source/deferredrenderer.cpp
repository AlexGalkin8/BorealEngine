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

#include <array>


namespace render {

static constexpr ETextureFormat kGBufferBaseColorFormat = ETextureFormat::kRGBA8_Unorm_Srgb;
static constexpr ETextureFormat kGBufferNormalFormat = ETextureFormat::kRGBA16_Float;
static constexpr ETextureFormat kSceneLightingFormat = ETextureFormat::kRGBA16_Float;
// Enable after both lighting shaders are available.
static constexpr bool kEnableDeferredLighting = false;

struct DirectionalLightConstants
{
	float direction_to_light[3];
	float direction_padding;
	float light_gain[3];
	float gain_padding;
};

static_assert(sizeof(DirectionalLightConstants) == 8 * sizeof(uint32_t));


static void setFullscreenViewport(ICommandList& command_list, const ITexture& target)
{
	const uint32_t width = target.getWidth();
	const uint32_t height = target.getHeight();
	command_list.setViewport(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));
	command_list.setScissorRect(0, 0, static_cast<int32_t>(width), static_cast<int32_t>(height));
	command_list.setPrimitiveTopology(EPrimitiveTopology::kTriangle);
}


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
	const asset::LoadedTextureData& loaded_texture,
	ETextureFormat format)
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
	texture_desc.format = format;

	ITexture* texture = render_device.getResourceManager().createTexture(texture_desc);
	ASSERT_MSG_RETURN_NULL(texture != nullptr, "Failed to create material texture");

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
	gBufferPass(bucket, depth_texture);
	if constexpr (kEnableDeferredLighting)
	{
		deferredLightingPass();
	}
	previewPass(output_target);
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
		LOG_WARNING("DeferredRenderer", "Resize to {}x{} skipped: failed to allocate the replacement frame targets.",
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

	constexpr std::array<ETextureFormat, 4> formats = {
		ETextureFormat::D32_Float, kGBufferBaseColorFormat, kGBufferNormalFormat, kSceneLightingFormat };
	const std::array<ITexture**, formats.size()> targets = {
		&depth_texture_, &gbuffer_base_color_, &gbuffer_normal_, &scene_lighting_hdr_ };
	constexpr size_t target_count = kEnableDeferredLighting ? formats.size() : formats.size() - 1;
	std::array<ITexture*, formats.size()> replacements = {};
	ITexture::Desc desc = {};
	desc.width = width;
	desc.height = height;
	for (size_t index = 0; index < target_count; ++index)
	{
		desc.format = formats[index];
		desc.render_target = index != 0;
		replacements[index] = resource_manager.createTexture(desc);
		if (!replacements[index])
		{
			for (size_t created = 0; created < index; ++created)
			{
				resource_manager.releaseTexture(*replacements[created]);
			}
			return;
		}
	}

	SET_RENDER_OBJECT_DEBUG_NAME(*replacements[0], "DepthPrepassDepth");
	SET_RENDER_OBJECT_DEBUG_NAME(*replacements[1], "GBufferAlbedo");
	SET_RENDER_OBJECT_DEBUG_NAME(*replacements[2], "GBufferWorldNormal");
	if constexpr (kEnableDeferredLighting)
	{
		SET_RENDER_OBJECT_DEBUG_NAME(*replacements[3], "SceneLightingHDR");
	}
	for (size_t index = 0; index < target_count; ++index)
	{
		ITexture*& target = *targets[index];
		if (target)
		{
			resource_manager.releaseTexture(*target);
		}
		target = replacements[index];
	}
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

	GraphicsPipelineDesc gbuffer_desc = depth_desc;
	gbuffer_desc.shader_state.shader_name = "gbuffer";
	gbuffer_desc.shader_state.shader_stages = EGraphicsShaderStage::kVertex | EGraphicsShaderStage::kPixel;
	gbuffer_desc.input_layout_state.addAttribute(ESemanticType::kTexCoord, ETextureFormat::kRG32_Float);
	gbuffer_desc.input_layout_state.addAttribute(ESemanticType::kColor, ETextureFormat::kRGBA32_Float);
	gbuffer_desc.input_layout_state.addAttribute(ESemanticType::kNormal, ETextureFormat::kRGB32_Float);
	gbuffer_desc.input_layout_state.addAttribute(ESemanticType::kTangent, ETextureFormat::kRGBA32_Float);
	gbuffer_desc.input_layout_state.addAttribute(ESemanticType::kTexCoord, ETextureFormat::kRG32_Float);
	gbuffer_desc.resource_layout_state[1] = RootParameter::makeDescriptorTable({ DescriptorRange{ DescriptorType::kSRV, 0, 2 } });
	gbuffer_desc.resource_layout_state[2] = RootParameter::makeConstants32Bits(1, 1, 0);
	gbuffer_desc.output_merger_state.addRenderTarget(kGBufferBaseColorFormat);
	gbuffer_desc.output_merger_state.addRenderTarget(kGBufferNormalFormat);
	gbuffer_desc.output_merger_state.depth_stencil.depth_write = false;
	gbuffer_desc.output_merger_state.depth_stencil.depth_func = EDepthFunc::kEqual;
	gbuffer_pipeline_state_ = &pipeline_state_manager_.createPipelineState(gbuffer_desc);

	GraphicsPipelineDesc preview_desc = {};
	preview_desc.shader_state.shader_name = kEnableDeferredLighting ? "lighting_preview" : "gbuffer_base_color";
	preview_desc.resource_layout_state[0] = RootParameter::makeDescriptorTable({ DescriptorRange{ DescriptorType::kSRV, 0, 1 } });
	preview_desc.output_merger_state.addRenderTarget(ETextureFormat::kRGBA8_Unorm);
	preview_desc.output_merger_state.depth_stencil.depth_enable = false;
	preview_desc.output_merger_state.depth_stencil.depth_write = false;
	preview_pipeline_state_ = &pipeline_state_manager_.createPipelineState(preview_desc);

	if constexpr (kEnableDeferredLighting)
	{
		GraphicsPipelineDesc lighting_desc = {};
		lighting_desc.shader_state.shader_name = "deferred_lighting";
		lighting_desc.resource_layout_state[0] = RootParameter::makeDescriptorTable({ DescriptorRange{ DescriptorType::kSRV, 0, 2 } });
		lighting_desc.resource_layout_state[1] = RootParameter::makeConstants32Bits(0, 8, 0);
		lighting_desc.output_merger_state.addRenderTarget(kSceneLightingFormat);
		lighting_desc.output_merger_state.depth_stencil.depth_enable = false;
		lighting_desc.output_merger_state.depth_stencil.depth_write = false;
		deferred_lighting_pipeline_state_ = &pipeline_state_manager_.createPipelineState(lighting_desc);
	}
}


void DeferredRenderer::depthPrepass(const StaticMeshBucket& bucket, ITexture& depth_texture)
{
	ICommandList& command_list = render_device_.acquireCommandList(ECommandListType::kDirect, "DepthPrepass");
	command_list.setDepthStencil(depth_texture);
	command_list.clearDepthStencil(depth_texture);
	command_list.setPipelineState(*depth_prepass_pipeline_state_);
	drawStaticMeshes(command_list, bucket, false);

	render_device_.submitCommandList(command_list);
}


void DeferredRenderer::gBufferPass(const StaticMeshBucket& bucket, ITexture& depth_texture)
{
	ICommandList& command_list = render_device_.acquireCommandList(ECommandListType::kDirect, "GBufferPass");
	command_list.transitionTexture(*gbuffer_base_color_, EResourceState::kShaderResource, EResourceState::kRenderTarget);
	command_list.transitionTexture(*gbuffer_normal_, EResourceState::kShaderResource, EResourceState::kRenderTarget);
	command_list.setRenderTargets({ gbuffer_base_color_, gbuffer_normal_ }, &depth_texture);
	command_list.clearRenderTarget(*gbuffer_base_color_, 0.0f, 0.0f, 0.0f, 0.0f);
	command_list.clearRenderTarget(*gbuffer_normal_, 0.0f, 0.0f, 0.0f, 0.0f);
	command_list.setPipelineState(*gbuffer_pipeline_state_);
	drawStaticMeshes(command_list, bucket, true);
	command_list.transitionTexture(*gbuffer_base_color_, EResourceState::kRenderTarget, EResourceState::kShaderResource);
	command_list.transitionTexture(*gbuffer_normal_, EResourceState::kRenderTarget, EResourceState::kShaderResource);
	render_device_.submitCommandList(command_list);
}


void DeferredRenderer::deferredLightingPass()
{
	static constexpr DirectionalLightConstants kLight = {
		{ 0.36f, 0.48f, -0.8f }, 0.0f,
		{ 2.0f, 2.0f, 2.0f }, 0.0f,
	};

	ITexture& lighting_target = *scene_lighting_hdr_;
	ICommandList& command_list = render_device_.acquireCommandList(ECommandListType::kDirect, "DeferredLighting");
	command_list.transitionTexture(lighting_target, EResourceState::kShaderResource, EResourceState::kRenderTarget);
	command_list.setRenderTarget(lighting_target);
	command_list.setPipelineState(*deferred_lighting_pipeline_state_);
	setFullscreenViewport(command_list, lighting_target);
	command_list.setDescriptorSet(0, { DescriptorResource::textureSRV(gbuffer_base_color_),
		DescriptorResource::textureSRV(gbuffer_normal_) });
	command_list.setGraphicsRoot32BitConstants(1, 8, &kLight);
	command_list.draw(3, 0);
	command_list.transitionTexture(lighting_target, EResourceState::kRenderTarget, EResourceState::kShaderResource);
	render_device_.submitCommandList(command_list);
}


void DeferredRenderer::previewPass(ITexture& output_target)
{
	ICommandList& command_list = render_device_.acquireCommandList(ECommandListType::kDirect,
		kEnableDeferredLighting ? "LightingPreview" : "GBufferBaseColorPreview");
	command_list.transitionTexture(output_target, EResourceState::kPresent, EResourceState::kRenderTarget);
	command_list.setRenderTarget(output_target);
	command_list.setPipelineState(*preview_pipeline_state_);
	setFullscreenViewport(command_list, output_target);
	command_list.setDescriptorSet(0, { DescriptorResource::textureSRV(kEnableDeferredLighting ? scene_lighting_hdr_ : gbuffer_base_color_) });
	command_list.draw(3, 0);
	command_list.transitionTexture(output_target, EResourceState::kRenderTarget, EResourceState::kPresent);
	render_device_.submitCommandList(command_list);
}


void DeferredRenderer::drawStaticMeshes(ICommandList& command_list, const StaticMeshBucket& bucket, bool bind_material) const
{
	command_list.setViewport(0.0f, 0.0f, static_cast<float>(render_settings_.render_width), static_cast<float>(render_settings_.render_height));
	command_list.setScissorRect(0, 0, static_cast<int32_t>(render_settings_.render_width), static_cast<int32_t>(render_settings_.render_height));
	command_list.setPrimitiveTopology(EPrimitiveTopology::kTriangle);

	for (const StaticMeshDraw& draw : bucket)
	{
		command_list.setGraphicsRoot32BitConstants(0, 32, &draw.constants);
		if (bind_material)
		{
			command_list.setDescriptorSet(1, { DescriptorResource::textureSRV(draw.mesh.base_color_texture),
				DescriptorResource::textureSRV(draw.mesh.normal_texture) });
			command_list.setGraphicsRoot32BitConstants(2, 1, &draw.mesh.normal_scale);
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

		std::vector<MaterialTextures> loaded_textures(loaded_model.textures.size());
		for (const asset::LoadedMaterialData& material : loaded_model.materials)
		{
			const int32_t base_color_index = material.base_color_texture_index;
			if (base_color_index >= 0 && static_cast<size_t>(base_color_index) < loaded_textures.size())
			{
				ITexture*& texture = loaded_textures[base_color_index].base_color;
				if (!texture)
				{
					texture = createTextureFromLoadedTextureData(render_device_, loaded_model.textures[base_color_index],
						ETextureFormat::kRGBA8_Unorm_Srgb);
				}
			}
			const int32_t normal_index = material.normal_texture_index;
			if (normal_index >= 0 && static_cast<size_t>(normal_index) < loaded_textures.size())
			{
				ITexture*& texture = loaded_textures[normal_index].normal;
				if (!texture)
				{
					texture = createTextureFromLoadedTextureData(render_device_, loaded_model.textures[normal_index],
						ETextureFormat::kRGBA8_Unorm);
				}
			}
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
	gpu_data.normal_texture = default_white_texture_;

	auto materials_it = file_materials_cache_.find(mesh.resource_path);
	auto textures_it = file_textures_cache_.find(mesh.resource_path);
	if (materials_it != file_materials_cache_.end() && textures_it != file_textures_cache_.end())
	{
		const std::vector<asset::LoadedMaterialData>& materials = materials_it->second;
		const std::vector<MaterialTextures>& textures = textures_it->second;

		if (primitive_it->material_index >= 0 && static_cast<uint32_t>(primitive_it->material_index) < materials.size())
		{
			const asset::LoadedMaterialData& material = materials[primitive_it->material_index];
			if (material.base_color_texture_index >= 0 && static_cast<uint32_t>(material.base_color_texture_index) < textures.size())
			{
				ITexture* texture = textures[material.base_color_texture_index].base_color;
				if (texture != nullptr)
				{
					gpu_data.base_color_texture = texture;
				}
			}
			if (material.normal_texture_index >= 0 && static_cast<size_t>(material.normal_texture_index) < textures.size())
			{
				ITexture* texture = textures[material.normal_texture_index].normal;
				if (texture)
				{
					gpu_data.normal_texture = texture;
					gpu_data.normal_scale = material.normal_scale;
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
