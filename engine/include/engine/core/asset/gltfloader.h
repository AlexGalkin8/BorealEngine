#pragma once

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
    };

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

extern LoadedMeshData loadStaticMeshFromGltf(const std::string& file_path);

}  // namespace asset
