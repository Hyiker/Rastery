#include "Shader.h"

#include <glad.h>

#include <algorithm>

#include "Utils/Logger.h"
namespace Rastery {

static void checkCompileErrors(uint32_t shader, bool isProgram) {
    int success;
    char infoLog[1024];
    if (isProgram) {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            logFatal("Shader compilation error:\n{}", infoLog);
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, nullptr, infoLog);

            glGetShaderInfoLog(shader, 1024, nullptr, infoLog);
            logFatal("Shader link error:\n {}", infoLog);
        }
    }
}

static int mapShaderStage(ShaderStage stage) {
    switch (stage) {
        case ShaderStage::Vertex:
            return GL_VERTEX_SHADER;
        case ShaderStage::Geometry:
            return GL_GEOMETRY_SHADER;
        case ShaderStage::TessellationControl:
            return GL_TESS_CONTROL_SHADER;
        case ShaderStage::TessellationEval:
            return GL_TESS_EVALUATION_SHADER;
        case ShaderStage::Fragment:
            return GL_FRAGMENT_SHADER;
        case ShaderStage::Compute:
            break;
    }
    logFatal("Unknown stage {}", int(stage));
}

ShaderProgram::SharedPtr createShaderProgram(
    const std::vector<std::pair<ShaderStage, std::string>> descs) {
    ShaderProgram::SharedPtr pShaderProgram = std::make_shared<ShaderProgram>();
    uint32_t program = glCreateProgram();

    std::vector<uint32_t> shaders;
    for (const auto& desc : descs) {
        unsigned int shader = glCreateShader(GL_VERTEX_SHADER);

        const char* src = desc.second.c_str();
        glShaderSource(shader, 1, (const GLchar* const*)src, nullptr);
        glCompileShader(shader);
        checkCompileErrors(shader, false);

        glAttachShader(program, shader);
        shaders.push_back(shader);
    }

    glLinkProgram(program);
    checkCompileErrors(program, true);

    for (uint32_t shader : shaders) {
        glDeleteShader(shader);
    }
    pShaderProgram->id = program;
    
    return pShaderProgram;
}
}  // namespace Rastery