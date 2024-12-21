#pragma once
#include <limits>
#include <utility>

#include "Math.h"
#include "core/macros.h"
namespace Rastery {
/** Axis-aligned bounding box.
 */
struct RASTERY_API AABB {
    float3 minPoint;  ///< Minimum corner.
    float3 maxPoint;  ///< Maximum corner.

    struct iterator {
        float3 current;
        int index;

        iterator(float3 min, const float3& max, int idx) : current(min), index(idx) {
            if (index < 8) {
                if (index & 1) current.x = max.x;
                if (index & 2) current.y = max.y;
                if (index & 4) current.z = max.z;
            }
        }

        float3 operator*() const { return current; }

        iterator& operator++() {
            ++index;
            return *this;
        }

        bool operator!=(const iterator& other) const { return index != other.index; }
    };

    [[nodiscard]] iterator begin() const { return {minPoint, maxPoint, 0}; }
    [[nodiscard]] iterator end() const { return {minPoint, maxPoint, 8}; }

    /** Construct an empty AABB.
     */
    AABB() : minPoint(float3(std::numeric_limits<float>::infinity())), maxPoint(float3(-std::numeric_limits<float>::infinity())) {}
    /** Construct an AABB from two corners.
     * @param min Minimum corner.
     * @param max Maximum corner.
     */
    AABB(float3 min, float3 max) : minPoint(min), maxPoint(max) {}
    /** Construct an AABB from a point.
     * @param p Point.
     */
    explicit AABB(const float3& p) : minPoint(p), maxPoint(p) {}

    // AABB boolean operations
    /** Expand the AABB to include a point.
     * @param p Point.
     */
    [[nodiscard]] AABB include(const float3& p) const {
        AABB ret = *this;
        ret.minPoint = glm::min(minPoint, p);
        ret.maxPoint = glm::max(maxPoint, p);
        return ret;
    }
    /** Expand the AABB to include another AABB.
     * @param aabb AABB.
     */
    [[nodiscard]] AABB include(const AABB& aabb) const {
        AABB ret = *this;
        ret.minPoint = glm::min(minPoint, aabb.minPoint);
        ret.maxPoint = glm::max(maxPoint, aabb.maxPoint);
        return ret;
    }

    /** Get the intersection of two AABBs.
     */
    [[nodiscard]] AABB intersect(const AABB& aabb) const { return {glm::max(minPoint, aabb.minPoint), glm::min(maxPoint, aabb.maxPoint)}; }

    /** Check if the AABB is empty.
     */
    [[nodiscard]] bool isEmpty() const { return minPoint.x > maxPoint.x || minPoint.y > maxPoint.y || minPoint.z > maxPoint.z; }
    /** Get the center of the AABB.
     */
    [[nodiscard]] float3 center() const { return (minPoint + maxPoint) / 2.f; }
    /** Get the diagonal of the AABB.
     */
    [[nodiscard]] float3 diagonal() const { return maxPoint - minPoint; }
    /** Check if a point is inside the AABB.
     */
    [[nodiscard]] bool inside(const float3& p) const {
        return p.x >= minPoint.x && p.x <= maxPoint.x && p.y >= minPoint.y && p.y <= maxPoint.y && p.z >= minPoint.z && p.z <= maxPoint.z;
    }

    AABB operator|(const AABB& aabb) const { return include(aabb); }
    AABB operator|(const float3& p) const { return include(p); }
    AABB operator&(const AABB& aabb) const { return intersect(aabb); }
    AABB& operator|=(const AABB& aabb) {
        *this = include(aabb);
        return *this;
    }
    AABB& operator|=(const float3& p) {
        *this = include(p);
        return *this;
    }

    AABB& operator&=(const AABB& aabb) {
        *this = intersect(aabb);
        return *this;
    }

    [[nodiscard]] bool operator==(const AABB& aabb) const { return minPoint == aabb.minPoint && maxPoint == aabb.maxPoint; }
    [[nodiscard]] bool operator!=(const AABB& aabb) const { return !(aabb == *this); }
};
}  // namespace Rastery