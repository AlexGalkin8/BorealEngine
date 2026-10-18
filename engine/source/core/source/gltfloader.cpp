#include "engine/core/asset/gltfloader.h"
#include "engine/core/utility/assert.h"

#include <cmath>

#pragma warning(push)
#pragma warning(disable: 4996)
#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
#define TINYGLTF3_ENABLE_STB_IMAGE
#include "..\..\..\third_party\tinygltf\source\tiny_gltf_v3.h"
#pragma warning(pop)


namespace asset {

static const tg3_accessor* findAttributeAccessor(
    const tinygltf3::Model& model,
    const tg3_primitive& primitive,
    const char* attribute_name)
{
    for (uint32_t i = 0; i < primitive.attributes_count; ++i)
    {
        if (tg3_str_equals_cstr(primitive.attributes[i].key, attribute_name) != 0)
        {
            const int32_t accessor_index = primitive.attributes[i].value;
            if (accessor_index >= 0 && static_cast<uint32_t>(accessor_index) < model->accessors_count)
            {
                return &model->accessors[accessor_index];
            }
            return nullptr;
        }
    }

    return nullptr;
}


static bool sampleBaseColorTexture(
    const tinygltf3::Model& model,
    const tg3_primitive& primitive,
    float u,
    float v,
    float out_color[4])
{
    if (primitive.material < 0 || static_cast<uint32_t>(primitive.material) >= model->materials_count)
    {
        return false;
    }

    const tg3_material& material = model->materials[primitive.material];
    const tg3_texture_info& base_color_texture_info = material.pbr_metallic_roughness.base_color_texture;
    if (base_color_texture_info.index < 0 || static_cast<uint32_t>(base_color_texture_info.index) >= model->textures_count)
    {
        return false;
    }

    const tg3_texture& texture = model->textures[base_color_texture_info.index];
    if (texture.source < 0 || static_cast<uint32_t>(texture.source) >= model->images_count)
    {
        return false;
    }

    const tg3_image& image = model->images[texture.source];
    if (image.image.data == nullptr || image.width <= 0 || image.height <= 0 || image.component < 3)
    {
        return false;
    }

    u = u - std::floor(u);
    v = v - std::floor(v);
    const int32_t x = static_cast<int32_t>(u * static_cast<float>(image.width - 1));
    const int32_t y = static_cast<int32_t>((1.0f - v) * static_cast<float>(image.height - 1));
    const int32_t pixel_index = (y * image.width + x) * image.component;

    const uint8_t* pixels = image.image.data;
    out_color[0] = static_cast<float>(pixels[pixel_index + 0]) / 255.0f;
    out_color[1] = static_cast<float>(pixels[pixel_index + 1]) / 255.0f;
    out_color[2] = static_cast<float>(pixels[pixel_index + 2]) / 255.0f;
    out_color[3] = (image.component >= 4) ? static_cast<float>(pixels[pixel_index + 3]) / 255.0f : 1.0f;
    return true;
}

static const uint8_t* resolveAccessorData(const tinygltf3::Model& model, const tg3_accessor& accessor)
{
    if (accessor.buffer_view < 0 || static_cast<uint32_t>(accessor.buffer_view) >= model->buffer_views_count)
    {
        return nullptr;
    }

    const tg3_buffer_view& buffer_view = model->buffer_views[accessor.buffer_view];
    if (buffer_view.buffer < 0 || static_cast<uint32_t>(buffer_view.buffer) >= model->buffers_count)
    {
        return nullptr;
    }

    const tg3_buffer& buffer = model->buffers[buffer_view.buffer];
    return buffer.data.data + buffer_view.byte_offset + accessor.byte_offset;
}


static LoadedMeshData loadPrimitiveFromModel(const tinygltf3::Model& model, const tg3_primitive& primitive)
{
    ASSERT_MSG_RETURN(primitive.mode == TG3_MODE_TRIANGLES, LoadedMeshData{}, "Only triangle lists are supported");

    const tg3_accessor* position_accessor = findAttributeAccessor(model, primitive, "POSITION");
    ASSERT_MSG_RETURN(position_accessor != nullptr, LoadedMeshData{}, "Missing POSITION accessor in glTF primitive");
    ASSERT_MSG_RETURN(position_accessor->component_type == TG3_COMPONENT_TYPE_FLOAT && position_accessor->type == TG3_TYPE_VEC3,
        LoadedMeshData{}, "POSITION accessor must be float3");

    const tg3_accessor* texcoord_accessor = findAttributeAccessor(model, primitive, "TEXCOORD_0");

    const uint8_t* position_data = resolveAccessorData(model, *position_accessor);
    ASSERT_MSG_RETURN(position_data != nullptr, LoadedMeshData{}, "Failed to resolve POSITION data in glTF primitive");

    const uint8_t* texcoord_data = texcoord_accessor ? resolveAccessorData(model, *texcoord_accessor) : nullptr;

    const int32_t position_stride = tg3_accessor_byte_stride(position_accessor,
        &model->buffer_views[position_accessor->buffer_view]);
    const int32_t texcoord_stride = (texcoord_accessor && texcoord_accessor->buffer_view >= 0)
        ? tg3_accessor_byte_stride(texcoord_accessor, &model->buffer_views[texcoord_accessor->buffer_view])
        : 0;

    LoadedMeshData loaded_mesh{};
    loaded_mesh.vertices.resize(static_cast<size_t>(position_accessor->count));

    for (uint64_t i = 0; i < position_accessor->count; ++i)
    {
        const float* src_position = reinterpret_cast<const float*>(position_data + static_cast<size_t>(position_stride) * i);
        LoadedMeshData::Vertex& dst_vertex = loaded_mesh.vertices[static_cast<size_t>(i)];
        dst_vertex.position[0] = src_position[0];
        dst_vertex.position[1] = src_position[1];
        dst_vertex.position[2] = -src_position[2];

        if (texcoord_accessor != nullptr && texcoord_data != nullptr)
        {
            const float* src_texcoord = reinterpret_cast<const float*>(texcoord_data + static_cast<size_t>(texcoord_stride) * i);
            dst_vertex.texcoord[0] = src_texcoord[0];
            dst_vertex.texcoord[1] = src_texcoord[1];
        }
        else
        {
            dst_vertex.texcoord[0] = 0.0f;
            dst_vertex.texcoord[1] = 0.0f;
        }

        dst_vertex.color[0] = 1.0f;
        dst_vertex.color[1] = 1.0f;
        dst_vertex.color[2] = 1.0f;
        dst_vertex.color[3] = 1.0f;
    }

    if (primitive.indices >= 0)
    {
        const tg3_accessor& index_accessor = model->accessors[primitive.indices];
        const uint8_t* index_data = resolveAccessorData(model, index_accessor);
        ASSERT_MSG_RETURN(index_data != nullptr, LoadedMeshData{}, "Failed to resolve index data in glTF primitive");

        const int32_t index_stride = tg3_accessor_byte_stride(&index_accessor, &model->buffer_views[index_accessor.buffer_view]);
        loaded_mesh.indices.resize(static_cast<size_t>(index_accessor.count));

        for (uint64_t i = 0; i < index_accessor.count; ++i)
        {
            const uint8_t* src_index = index_data + static_cast<size_t>(index_stride) * i;
            switch (index_accessor.component_type)
            {
            case TG3_COMPONENT_TYPE_UNSIGNED_SHORT:
                loaded_mesh.indices[static_cast<size_t>(i)] = *reinterpret_cast<const uint16_t*>(src_index);
                break;
            case TG3_COMPONENT_TYPE_UNSIGNED_INT:
                loaded_mesh.indices[static_cast<size_t>(i)] = *reinterpret_cast<const uint32_t*>(src_index);
                break;
            case TG3_COMPONENT_TYPE_UNSIGNED_BYTE:
                loaded_mesh.indices[static_cast<size_t>(i)] = *reinterpret_cast<const uint8_t*>(src_index);
                break;
            default:
                ASSERT_FAIL("Unsupported index component type in glTF primitive");
                return LoadedMeshData{};
            }
        }
    }
    else
    {
        loaded_mesh.indices.resize(loaded_mesh.vertices.size());
        for (uint32_t i = 0; i < static_cast<uint32_t>(loaded_mesh.vertices.size()); ++i)
        {
            loaded_mesh.indices[i] = i;
        }
    }

    return loaded_mesh;
}


static bool parseModelFromFile(const std::string& file_path, tinygltf3::Model& out_model)
{
    tinygltf3::ErrorStack errors;
    const tg3_error_code parse_result = tinygltf3::parse_file(out_model, errors, file_path.c_str());
    ASSERT_MSG_RETURN(parse_result == TG3_OK, false, "Failed to parse glTF file: %s", file_path.c_str());
    ASSERT_MSG_RETURN(out_model->meshes_count != 0, false, "No meshes found in glTF file");
    return true;
}


LoadedMeshData loadStaticMeshFromGltf(const std::string& file_path)
{
 tinygltf3::Model model;
    ASSERT_MSG_RETURN(parseModelFromFile(file_path, model), LoadedMeshData{}, "Failed to parse glTF model");

    const tg3_mesh& mesh = model->meshes[0];
    ASSERT_MSG_RETURN(mesh.primitives_count != 0, LoadedMeshData{}, "No primitives found in glTF mesh");
    return loadPrimitiveFromModel(model, mesh.primitives[0]);
}


LoadedMeshData loadStaticMeshPrimitiveFromGltf(const std::string& file_path, uint32_t mesh_index, uint32_t primitive_index)
{
    tinygltf3::Model model;
    ASSERT_MSG_RETURN(parseModelFromFile(file_path, model), LoadedMeshData{}, "Failed to parse glTF model");
    ASSERT_MSG_RETURN(mesh_index < model->meshes_count, LoadedMeshData{}, "Mesh index out of range for glTF file");

    const tg3_mesh& mesh = model->meshes[mesh_index];
    ASSERT_MSG_RETURN(primitive_index < mesh.primitives_count, LoadedMeshData{}, "Primitive index out of range for glTF mesh");
    return loadPrimitiveFromModel(model, mesh.primitives[primitive_index]);
}


static void fillTexturesAndMaterials(const tinygltf3::Model& model, LoadedModelData& loaded_model)
{
	loaded_model.textures.reserve(model->textures_count);
	for (uint32_t texture_index = 0; texture_index < model->textures_count; ++texture_index)
	{
		const tg3_texture& texture = model->textures[texture_index];
		LoadedTextureData texture_data{};
		texture_data.texture_index = texture_index;
		texture_data.image_index = texture.source;
		texture_data.name = texture.name.data ? texture.name.data : "";

		if (texture.source >= 0 && static_cast<uint32_t>(texture.source) < model->images_count)
		{
			const tg3_image& image = model->images[texture.source];
			texture_data.uri = image.uri.data ? image.uri.data : "";
			texture_data.width = image.width;
			texture_data.height = image.height;
			texture_data.component_count = image.component;

			if (image.image.data != nullptr && image.width > 0 && image.height > 0 && image.component > 0)
			{
				const size_t pixel_data_size = static_cast<size_t>(image.width) * static_cast<size_t>(image.height) * static_cast<size_t>(image.component);
				texture_data.pixel_data.resize(pixel_data_size);
				memcpy(texture_data.pixel_data.data(), image.image.data, pixel_data_size);
			}
		}

		loaded_model.textures.push_back(std::move(texture_data));
	}

	loaded_model.materials.reserve(model->materials_count);
	for (uint32_t material_index = 0; material_index < model->materials_count; ++material_index)
	{
		const tg3_material& material = model->materials[material_index];
		LoadedMaterialData material_data{};
		material_data.material_index = material_index;
		material_data.name = material.name.data ? material.name.data : "";
		material_data.base_color_texture_index = material.pbr_metallic_roughness.base_color_texture.index;
		material_data.base_color_factor[0] = static_cast<float>(material.pbr_metallic_roughness.base_color_factor[0]);
		material_data.base_color_factor[1] = static_cast<float>(material.pbr_metallic_roughness.base_color_factor[1]);
		material_data.base_color_factor[2] = static_cast<float>(material.pbr_metallic_roughness.base_color_factor[2]);
		material_data.base_color_factor[3] = static_cast<float>(material.pbr_metallic_roughness.base_color_factor[3]);
		loaded_model.materials.push_back(std::move(material_data));
	}
}


LoadedModelData loadModelFromGltf(const std::string& file_path)
{
	tinygltf3::Model model;
	ASSERT_MSG_RETURN(parseModelFromFile(file_path, model), LoadedModelData{}, "Failed to parse glTF model");

	LoadedModelData loaded_model{};
	fillTexturesAndMaterials(model, loaded_model);
	for (uint32_t mesh_index = 0; mesh_index < model->meshes_count; ++mesh_index)
	{
		const tg3_mesh& mesh = model->meshes[mesh_index];
		for (uint32_t primitive_index = 0; primitive_index < mesh.primitives_count; ++primitive_index)
		{
			LoadedPrimitiveData primitive_data{};
			primitive_data.mesh_index = mesh_index;
			primitive_data.primitive_index = primitive_index;
			primitive_data.material_index = mesh.primitives[primitive_index].material;
			primitive_data.mesh_data = loadPrimitiveFromModel(model, mesh.primitives[primitive_index]);

			if (!primitive_data.mesh_data.vertices.empty() && !primitive_data.mesh_data.indices.empty())
			{
				loaded_model.primitives.push_back(std::move(primitive_data));
			}
		}
	}

	return loaded_model;
}




}  // namespace asset