#pragma once

#include <engine/math/matrix.h>

#include <cstdint>
#include <string>
#include <vector>


namespace asset {

struct LoadedMeshData
{
	struct Vertex
	{
		float position[3];
		float texcoord[2];
		float color[4];
		float normal[3];
		float tangent[4];
		float normal_texcoord[2];
		float metallic_roughness_texcoord[2];
	};

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

struct LoadedTextureData
{
	uint32_t texture_index = 0;
	int32_t image_index = -1;
	std::string name;
	std::string uri;
	int32_t width = 0;
	int32_t height = 0;
	int32_t component_count = 0;
	std::vector<uint8_t> pixel_data;
};

struct LoadedMaterialData
{
	uint32_t material_index = 0;
	std::string name;
	int32_t base_color_texture_index = -1;
	int32_t normal_texture_index = -1;
	int32_t metallic_roughness_texture_index = -1;
	float normal_scale = 1.0f;
	float roughness_factor = 1.0f;
	float metallic_factor = 1.0f;
	float base_color_factor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct LoadedPrimitiveData
{
	uint32_t mesh_index = 0;
	uint32_t primitive_index = 0;
	int32_t material_index = -1;
	LoadedMeshData mesh_data;
};

struct LoadedMeshInstance
{
	uint32_t mesh_index;
	math::Matrix4x4 transform;
};

struct LoadedModelData
{
	std::vector<LoadedTextureData> textures;
	std::vector<LoadedMaterialData> materials;
	std::vector<LoadedPrimitiveData> primitives;
	std::vector<LoadedMeshInstance> mesh_instances;
};

extern LoadedMeshData loadStaticMeshFromGltf(const std::string& file_path);
extern LoadedMeshData loadStaticMeshPrimitiveFromGltf(const std::string& file_path, uint32_t mesh_index, uint32_t primitive_index);
extern LoadedModelData loadModelFromGltf(const std::string& file_path);

}  // namespace asset
