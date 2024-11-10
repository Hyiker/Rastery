#include "Math.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Rastery {

float4x4 lookAt(const float3& pos, const float3& target, const float3& up) { return glm::lookAtRH(pos, target, up); }

float4x4 perspective(float fovY, float asepctRatio, float near, float far) { return glm::perspectiveRH_ZO(fovY, asepctRatio, near, far); }

}  // namespace Rastery