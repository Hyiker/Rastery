#pragma once
#include <cstdint>
#include <memory>
#include <vector>

#include "Core/Macros.h"
namespace Rastery {

enum class ShaderStage {
    Vertex,
    Geometry,
    TessellationControl,
    TessellationEval,
    Fragment,
    Compute,
    Count
};

struct RASTERY_API ShaderProgram {
    using SharedPtr = std::shared_ptr<ShaderProgram>;
    uint32_t id;
};

RASTERY_API ShaderProgram::SharedPtr createShaderProgram(
    const std::vector<std::pair<ShaderStage, std::string>> descs);

}  // namespace Rastery