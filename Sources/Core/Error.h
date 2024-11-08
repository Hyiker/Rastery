#pragma once
#include <cassert>
#include <source_location>

#include "Core/Macros.h"
#include "Utils/Logger.h"

#define RASTERY_ASSERT(x) assert(x)

#define RASTERY_UNREACHABLE() logFatal("Unreachable code!")

// We don't have Debug callback in OpenGL 4.1
// So we have to spread this macro all over the place
#define RASTERY_CHECK_GL_ERROR()                                              \
    do {                                                                      \
        if (int error = glGetError(); error != GL_NO_ERROR) {                 \
            ::Rastery::reportGlError(error, std::source_location::current()); \
        }                                                                     \
    } while (0)

namespace Rastery {

RASTERY_API void reportGlError(int code, std::source_location loc);

}  // namespace Rastery
