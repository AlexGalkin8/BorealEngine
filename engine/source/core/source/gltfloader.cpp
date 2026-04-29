#include "engine/core/asset/gltfloader.h"
#include "engine/core/utility/assert.h"

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#pragma warning(push)
#pragma warning(disable: 4996)
#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS
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


LoadedMeshData loadStaticMeshFromGltf(const std::string& file_path)
{
    tinygltf3::Model model;
    tinygltf3::ErrorStack errors;

    const tg3_error_code parse_result = tinygltf3::parse_file(model, errors, file_path.c_str());
    ASSERT_MSG_RETURN(parse_result == TG3_OK, LoadedMeshData{}, "Failed to parse glTF file: %s", file_path.c_str());
    ASSERT_MSG_RETURN(model->meshes_count != 0, LoadedMeshData{}, "No meshes found in glTF file");

    const tg3_mesh& mesh = model->meshes[0];
    ASSERT_MSG_RETURN(mesh.primitives_count != 0, LoadedMeshData{}, "No primitives found in glTF mesh");

    const tg3_primitive& primitive = mesh.primitives[0];
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
        dst_vertex.position[2] = src_position[2];

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
}  // namespace asset