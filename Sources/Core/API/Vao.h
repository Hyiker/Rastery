#pragma once
#include <filesystem>
#include <memory>
#include <vector>

#include "Core/Macros.h"
#include "Core/Math.h"
namespace Rastery {
// Use Predefined Vertex layout for now
struct Vertex {
    float3 position;
    float3 normal;
    float2 texCoord;
};

struct RASTERY_API CpuVao {
    using SharedPtr = std::shared_ptr<CpuVao>;
    std::vector<Vertex> vertexData;
    std::vector<uint32_t> indexData;
};
// TODO move the importer part to Utils/Importer.h
CpuVao::SharedPtr createFromFile(const std::filesystem::path& p);

}  // namespace Rastery