#pragma once
#include <cassert>

#include "Utils/Logger.h"
namespace Rastery {
#define RASTERY_ASSERT(x) assert(x)

#define RASTERY_UNREACHABLE() logFatal("Unreachable code!")
}  // namespace Rastery
