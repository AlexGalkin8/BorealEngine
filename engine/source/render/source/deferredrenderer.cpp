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

#include <algorithm>
#include <array>


namespace render {

static constexpr ETextureFormat kGBufferBaseColorFormat = ETextureFormat::kRGBA8_Unorm_Srgb;
static constexpr ETextureFormat kGBufferNormalFormat = ETextureFormat::kRGBA16_Float;
static constexpr uint32_t kGBufferMaterialTextureCount = 3;
static constexpr ETextureFormat kGBufferMaterialFormat = ETextureFormat::kRG8_Unorm;
static constexpr ETextureFormat kSceneLightingFormat = ETextureFormat::kRGBA16_Float;

struct DirectionalLightConstants
{
	float direction_to_light[3];
	ELightingModel lighting_model;
	float light_gain[3];
	uint32_t has_directional_light;
};

struct PointLightConstants
{
	float position[3];
	float radius;
	float light_gain[3];
	float padding;
};

struct LightingConstants
{
	DirectionalLightConstants directional;
	math::Matrix4x4 inverse_view_projection;
	uint32_t point_light_count;
	float camera_position[3];
	PointLightConstants point_lights[scene::kMaxPointLights];
};

static_assert(sizeof(LightingConstants) == 60 * sizeof(uint32_t));

// Invert the rigid view transform and the camera's D3D perspective projection.
static math::Matrix4x4 buildInverseViewProjection(const scene::Camera& camera)
{
	const math::Matrix4x4& view = camera.getViewMatrix();
	const math::Matrix4x4& projection = camera.getProjectionMatrix();
	const math::Vector3& eye = camera.getPosition();
	const math::Matrix4x4 inverse_view = {
		view.m00, view.m10, view.m20, eye.x,
		view.m01, view.m11, view.m21, eye.y,
		view.m02, view.m12, view.m22, eye.z,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	const math::Matrix4x4 inverse_projection = {
		1.0f / projection.m00, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f / projection.m11, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,
		0.0f, 0.0f, 1.0f / projection.m23, -projection.m22 / projection.m23
	};
	return inverse_view * inverse_projection;
}


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
	deferredLightingPass(scene, camera, depth_texture);
	postprocessPass(output_target);
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

	constexpr std::array<ETextureFormat, 5> formats = {
		ETextureFormat::D32_Float, kGBufferBaseColorFormat, kGBufferNormalFormat, kGBufferMaterialFormat, kSceneLightingFormat };
	const std::array<ITexture**, formats.size()> targets = {
		&depth_texture_, &gbuffer_base_color_, &gbuffer_normal_, &gbuffer_material_, &scene_lighting_hdr_ };
	std::array<ITexture*, formats.size()> replacements = {};
	ITexture::Desc desc = {};
	desc.width = width;
	desc.height = height;
	for (size_t index = 0; index < formats.size(); ++index)
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
	SET_RENDER_OBJECT_DEBUG_NAME(*replacements[3], "GBufferMaterial");
	SET_RENDER_OBJECT_DEBUG_NAME(*replacements[4], "SceneLightingHDR");
	for (size_t index = 0; index < targets.size(); ++index)
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
	gbuffer_desc.input_layout_state.addAttribute(ESemanticType::kTexCoord, ETextureFormat::kRG32_Float);
	gbuffer_desc.resource_layout_state[1] = RootParameter::makeDescriptorTable({ DescriptorRange{ DescriptorType::kSRV, 0, kGBufferMaterialTextureCount } });
	gbuffer_desc.resource_layout_state[2] = RootParameter::makeConstants32Bits(1, sizeof(MaterialConstants) / sizeof(uint32_t), 0);
	gbuffer_desc.output_merger_state.addRenderTarget(kGBufferBaseColorFormat);
	gbuffer_desc.output_merger_state.addRenderTarget(kGBufferNormalFormat);
	gbuffer_desc.output_merger_state.addRenderTarget(kGBufferMaterialFormat);
	gbuffer_desc.output_merger_state.depth_stencil.depth_write = false;
	gbuffer_desc.output_merger_state.depth_stencil.depth_func = EDepthFunc::kEqual;
	gbuffer_pipeline_state_ = &pipeline_state_manager_.createPipelineState(gbuffer_desc);

	GraphicsPipelineDesc lighting_desc = {};
	lighting_desc.shader_state.shader_name = "deferred_lighting";
	lighting_desc.resource_layout_state[0] = RootParameter::makeDescriptorTable({ DescriptorRange{ DescriptorType::kSRV, 0, 4 } });
	lighting_desc.resource_layout_state[1] = RootParameter::makeConstants32Bits(0, sizeof(LightingConstants) / sizeof(uint32_t));
	lighting_desc.output_merger_state.addRenderTarget(kSceneLightingFormat);
	lighting_desc.output_merger_state.depth_stencil.depth_enable = false;
	lighting_desc.output_merger_state.depth_stencil.depth_write = false;
	deferred_lighting_pipeline_state_ = &pipeline_state_manager_.createPipelineState(lighting_desc);

	GraphicsPipelineDesc postprocess_desc = {};
	postprocess_desc.shader_state.shader_name = "postprocess";
	postprocess_desc.resource_layout_state[0] = RootParameter::makeDescriptorTable({ DescriptorRange{ DescriptorType::kSRV, 0, 4 } });
	postprocess_desc.resource_layout_state[1] = RootParameter::makeConstants32Bits(0, 2);
	postprocess_desc.output_merger_state.addRenderTarget(kBackBufferFormat);
	postprocess_desc.output_merger_state.depth_stencil.depth_enable = false;
	postprocess_desc.output_merger_state.depth_stencil.depth_write = false;
	postprocess_pipeline_state_ = &pipeline_state_manager_.createPipelineState(postprocess_desc);
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
	command_list.transitionTexture(*gbuffer_material_, EResourceState::kShaderResource, EResourceState::kRenderTarget);
	command_list.setRenderTargets({ gbuffer_base_color_, gbuffer_normal_, gbuffer_material_ }, &depth_texture);
	command_list.clearRenderTarget(*gbuffer_base_color_, 0.0f, 0.0f, 0.0f, 0.0f);
	command_list.clearRenderTarget(*gbuffer_normal_, 0.0f, 0.0f, 0.0f, 0.0f);
	command_list.clearRenderTarget(*gbuffer_material_, 0.0f, 0.0f, 0.0f, 0.0f);
	command_list.setPipelineState(*gbuffer_pipeline_state_);
	drawStaticMeshes(command_list, bucket, true);
	command_list.transitionTexture(*gbuffer_base_color_, EResourceState::kRenderTarget, EResourceState::kShaderResource);
	command_list.transitionTexture(*gbuffer_normal_, EResourceState::kRenderTarget, EResourceState::kShaderResource);
	command_list.transitionTexture(*gbuffer_material_, EResourceState::kRenderTarget, EResourceState::kShaderResource);
	render_device_.submitCommandList(command_list);
}


void DeferredRenderer::deferredLightingPass(const scene::Scene& scene, const scene::Camera& camera, ITexture& depth_texture)
{
	const scene::DirectionalLight& light = scene.getDirectionalLight();
	LightingConstants constants = {};
	constants.directional.lighting_model = render_settings_.lighting_model;
	if (light.enabled && light.intensity > 0.0f && light.direction_to_light.lengthSq() > 0.0f)
	{
		const math::Vector3 direction = light.direction_to_light.normalize();
		constants.directional = {
			{ direction.x, direction.y, direction.z }, render_settings_.lighting_model,
			{ std::max(light.color.x, 0.0f) * light.intensity,
			  std::max(light.color.y, 0.0f) * light.intensity,
			  std::max(light.color.z, 0.0f) * light.intensity }, 1,
		};
	}

	constants.inverse_view_projection = buildInverseViewProjection(camera);
	const math::Vector3& camera_position = camera.getPosition();
	constants.camera_position[0] = camera_position.x;
	constants.camera_position[1] = camera_position.y;
	constants.camera_position[2] = camera_position.z;
	for (const scene::PointLight& point : scene.getPointLights())
	{
		if (constants.point_light_count == scene::kMaxPointLights)
		{
			break;
		}
		if (!point.enabled || point.radius <= 0.0f || point.intensity <= 0.0f)
		{
			continue;
		}

		constants.point_lights[constants.point_light_count++] = {
			{ point.position.x, point.position.y, point.position.z }, point.radius,
			{ std::max(point.color.x, 0.0f) * point.intensity,
			  std::max(point.color.y, 0.0f) * point.intensity,
			  std::max(point.color.z, 0.0f) * point.intensity }, 0.0f,
		};
	}

	ITexture& lighting_target = *scene_lighting_hdr_;
	ICommandList& command_list = render_device_.acquireCommandList(ECommandListType::kDirect, "DeferredLighting");
	command_list.transitionTexture(depth_texture, EResourceState::kDepthWrite, EResourceState::kShaderResource);
	command_list.transitionTexture(lighting_target, EResourceState::kShaderResource, EResourceState::kRenderTarget);
	command_list.setRenderTarget(lighting_target);
	command_list.setPipelineState(*deferred_lighting_pipeline_state_);
	setFullscreenViewport(command_list, lighting_target);
	command_list.setDescriptorSet(0, { DescriptorResource::textureSRV(gbuffer_base_color_),
		DescriptorResource::textureSRV(gbuffer_normal_), DescriptorResource::textureSRV(&depth_texture),
		DescriptorResource::textureSRV(gbuffer_material_) });
	command_list.setGraphicsRoot32BitConstants(1, sizeof(LightingConstants) / sizeof(uint32_t), &constants);
	command_list.draw(3, 0);
	command_list.transitionTexture(lighting_target, EResourceState::kRenderTarget, EResourceState::kShaderResource);
	command_list.transitionTexture(depth_texture, EResourceState::kShaderResource, EResourceState::kDepthWrite);
	render_device_.submitCommandList(command_list);
}


void DeferredRenderer::postprocessPass(ITexture& output_target)
{
	struct PostprocessConstants
	{
		float exposure;
		EDebugRenderView debug_view;
	};
	static_assert(sizeof(PostprocessConstants) == 2 * sizeof(uint32_t));
	const PostprocessConstants constants = { 1.0f, render_settings_.debug_view };
	ICommandList& command_list = render_device_.acquireCommandList(ECommandListType::kDirect, "Postprocess");
	command_list.transitionTexture(output_target, EResourceState::kPresent, EResourceState::kRenderTarget);
	command_list.setRenderTarget(output_target);
	command_list.setPipelineState(*postprocess_pipeline_state_);
	setFullscreenViewport(command_list, output_target);
	command_list.setDescriptorSet(0, { DescriptorResource::textureSRV(scene_lighting_hdr_),
		DescriptorResource::textureSRV(gbuffer_base_color_), DescriptorResource::textureSRV(gbuffer_normal_),
		DescriptorResource::textureSRV(gbuffer_material_) });
	command_list.setGraphicsRoot32BitConstants(1, 2, &constants);
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
				DescriptorResource::textureSRV(draw.mesh.normal_texture),
				DescriptorResource::textureSRV(draw.mesh.metallic_roughness_texture) });
			command_list.setGraphicsRoot32BitConstants(2, sizeof(MaterialConstants) / sizeof(uint32_t), &draw.mesh.material);
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

		const math::Matrix4x4 actor_world = static_mesh_actor->getTransform().toMatrix4x4();
		for (const scene::MeshSlot& slot : static_mesh_actor->getMeshRenderer().getMeshSlots())
		{
			const RenderMesh* mesh = getOrCreateRenderMesh(slot.mesh);
			if (mesh)
			{
				ASSERT_MSG_RETURN(mesh->base_color_texture != nullptr, bucket, "Base color texture is not initialized");
				const CameraObjectConstants constants = { actor_world * slot.local_transform, view_projection };
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
			const int32_t linear_texture_indices[] = { material.normal_texture_index, material.metallic_roughness_texture_index };
			for (const int32_t index : linear_texture_indices)
			{
				if (index >= 0 && static_cast<size_t>(index) < loaded_textures.size())
				{
					ITexture*& texture = loaded_textures[index].linear;
					if (!texture)
					{
						texture = createTextureFromLoadedTextureData(render_device_, loaded_model.textures[index],
							ETextureFormat::kRGBA8_Unorm);
					}
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
	gpu_data.metallic_roughness_texture = default_white_texture_;

	auto materials_it = file_materials_cache_.find(mesh.resource_path);
	auto textures_it = file_textures_cache_.find(mesh.resource_path);
	if (materials_it != file_materials_cache_.end() && textures_it != file_textures_cache_.end())
	{
		const std::vector<asset::LoadedMaterialData>& materials = materials_it->second;
		const std::vector<MaterialTextures>& textures = textures_it->second;

		if (primitive_it->material_index >= 0 && static_cast<uint32_t>(primitive_it->material_index) < materials.size())
		{
			const asset::LoadedMaterialData& material = materials[primitive_it->material_index];
			std::copy_n(material.base_color_factor, 4, gpu_data.material.base_color_factor);
			gpu_data.material.roughness_factor = material.roughness_factor;
			gpu_data.material.metallic_factor = material.metallic_factor;
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
				ITexture* texture = textures[material.normal_texture_index].linear;
				if (texture)
				{
					gpu_data.normal_texture = texture;
					gpu_data.material.normal_scale = material.normal_scale;
				}
			}
			const int32_t metallic_roughness_index = material.metallic_roughness_texture_index;
			if (metallic_roughness_index >= 0 && static_cast<size_t>(metallic_roughness_index) < textures.size())
			{
				ITexture* texture = textures[metallic_roughness_index].linear;
				if (texture)
				{
					gpu_data.metallic_roughness_texture = texture;
					gpu_data.material.has_metallic_roughness_texture = 1;
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
