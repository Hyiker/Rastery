#pragma once

#include <memory>
#include <vector>

#include "Core/AABB.h"
#include "Core/API/Vao.h"
#include "Core/Macros.h"
#include "Core/Math.h"

namespace Rastery {

struct RASTERY_API BVHNode {
    static constexpr int kMaxChildrenCount = 4;
    std::vector<int> children;  ///< Index into children node
    int primOffset = -1;        ///< Primitive offset, set after vertex shader
    int vaoOffset = -1;         ///< Leaf VAO offset into index buffer, divided by 3
    int leafCnt = 0;

    AABB aabb;                       ///< AABB in world space, update only when object moved
    AABB viewportAABB;               ///< view port(screen space aabb)
    bool isCulledLastFrame = false;  /// Is this node culled last frame

    bool isLeaf() const { return vaoOffset != -1; }
    bool isPrimitiveValid() const { return primOffset != -1; }
    bool hasChildren() const { return !children.empty(); }
};

class RASTERY_API BVH {
   public:
    using SharedPtr = std::shared_ptr<BVH>;
    BVH();

    void build(const CpuVao::SharedPtr& pVao);

    const BVHNode& getRootNode() const;
    BVHNode& getRootNode();

    BVHNode& getLeafNodeByVaoOffset(int index);

    const BVHNode& getNode(int index) const { return mNodes[index]; }
    BVHNode& getNode(int index) { return mNodes[index]; }

    void updateViewportData();

    void reset();

   private:
    std::vector<uint32_t> mVaoOffsetToLeaf;
    std::vector<BVHNode> mNodes;
};
}  // namespace Rastery