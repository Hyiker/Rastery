#include "Math.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Rastery {

float4x4 lookAt(const float3& pos, const float3& target, const float3& up) { return glm::lookAtRH(pos, target, up); }

float4x4 perspective(float fovY, float asepctRatio, float near, float far) { return glm::perspectiveRH_ZO(fovY, asepctRatio, near, far); }

float2 toSpherical(const float3& v) {
    float theta = std::acos(v.y);
    float phi = std::atan2(v.z, v.x);
    return float2(theta, phi);
}

float3 toCartesian(const float2& tp) { return float3(std::sin(tp.x) * std::cos(tp.y), std::cos(tp.x), std::sin(tp.x) * std::sin(tp.y)); }

quatf quatFromRotationBetweenVectors(float3 orig, float3 dest) {
    if (1.0 - absDot(orig, dest) < 1e-5) {
        // return unit quaternion for parallel vectors
        return quatf(1.0, float3(0.0));
    }
    float3 axis = normalize(cross(orig, dest));

    float cosTheta = glm::dot(orig, dest);
    float angle = glm::acos(cosTheta);
    return glm::angleAxis(angle, axis);
}

float3x3 matrixFromQuat(const quatf& q) { return glm::toMat3(q); }

}  // namespace Rastery