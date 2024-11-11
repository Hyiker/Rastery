#pragma once

#include <fmt/format.h>

#include <algorithm>
#include <cstdint>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

namespace Rastery {
using uint = uint32_t;
using uint2 = glm::uvec2;
using uint3 = glm::uvec3;
using uint4 = glm::uvec4;

using int2 = glm::ivec2;
using int3 = glm::ivec3;
using int4 = glm::ivec4;

using bool2 = glm::bvec2;
using bool3 = glm::bvec3;
using bool4 = glm::bvec4;

using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;

using float3x3 = glm::mat3x3;
using float4x4 = glm::mat4x4;
using float3x4 = glm::mat3x4;

inline float3 cross(const float3& v0, const float3& v1) { return glm::cross(v0, v1); }

template <typename T>
float dot(const T& v0, const T& v1) {
    return glm::dot(v0, v1);
}

template <typename T>
T saturate(T value) {
    return std::clamp<T>(value, 0, 1);
}

template <typename T>
T radians(T deg) {
    return glm::radians(deg);
}

template <typename T>
T degrees(T rad) {
    return glm::degrees(rad);
}

template <typename T>
T lerp(const T& left, const T& right, const T& t) {
    return std::lerp(left, right, t);
}

template <typename T>
T lerp(const T& left, const T& right, float t) {
    return left * (1.0f - t) + right * t;
}

template <typename T>
bool any(const T& v) {
    return glm::any(v);
}

template <typename T>
bool all(const T& v) {
    return glm::all(v);
}

// Transform functions use Right-handed, y-up coordinates

/**
 * Builds a right-handed look-at view matrix.
 * This matrix defines a view transformation where the camera is positioned at `pos`,
 * looking towards `target`, with the `up` vector specifying the camera's orientation.
 *
 * @param pos The position of the camera in world space coordinates.
 * @param target The point in world space coordinates that the camera is looking at.
 * @param up The direction vector representing the upward orientation of the camera.
 * @return A 4x4 transformation matrix representing the view matrix.
 */
float4x4 lookAt(const float3& pos, const float3& target, const float3& up);

/**
 * Creates a matrix for a right-handed, symmetric perspective-view frustum.
 * This matrix is useful for projecting 3D scenes onto a 2D surface, typically used
 * in graphics rendering for creating perspective depth.
 * The frustum defines a visible area based on the field of view and aspect ratio,
 * with near and far clipping planes in Direct3D clip volume coordinates (0 to +1).
 *
 * @param fovY Field of view in the Y axis, in radians. This defines the vertical angle of the view.
 * @param aspectRatio The aspect ratio of the view window, defined as width divided by height.
 * @param near The distance to the near clipping plane, which must be positive.
 * @param far The distance to the far clipping plane, which must be greater than `near`.
 * @return A 4x4 transformation matrix representing the perspective projection matrix.
 */
float4x4 perspective(float fovY, float aspectRatio, float near, float far);

template <typename T>
T transpose(T mat) {
    return glm::transpose(mat);
}

template <typename T>
T inverse(T mat) {
    return glm::inverse(mat);
}

}  // namespace Rastery
