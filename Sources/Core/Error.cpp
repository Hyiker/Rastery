#include "Error.h"

#include <glad.h>

#include <source_location>

#include "Utils/Logger.h"
namespace Rastery {
void reportGlError(int code, std::source_location loc) {
    std::string errorStr;
    switch (code) {
        case GL_INVALID_ENUM:
            errorStr = "GL_INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            errorStr = "GL_INVALID_VALUE";
            break;
        case GL_INVALID_OPERATION:
            errorStr = "GL_INVALID_OPERATION";
            break;
        case GL_STACK_OVERFLOW:
            errorStr = "GL_STACK_OVERFLOW";
            break;
        case GL_STACK_UNDERFLOW:
            errorStr = "GL_STACK_UNDERFLOW";
            break;
        case GL_OUT_OF_MEMORY:
            errorStr = "GL_OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            errorStr = "GL_INVALID_FRAMEBUFFER_OPERATION";
            break;
        default:
            errorStr = "Unknown Error";
            break;
    }
    // Use log fatal to throw an error for stacktrace
    logFatal("{}:{} {}", loc.file_name(), loc.line(), errorStr);
}
}  // namespace Rastery