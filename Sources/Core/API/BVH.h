#pragma once

#include <memory>
#include <vector>

#include "Core/AABB.h"
#include "Core/API/Vao.h"
#include "Core/Macros.h"
#include "Core/Math.h"

namespace Rastery {

struct RASTERY_API BVHNode {
    int left = -1;        ///< Index into left node
    int right = -1;       ///< Index into right node
    int father = -1;      ///<
    int primOffset = -1;  ///< Primitive offset, set after vertex shader
    int vaoOffset = -1;   ///< VAO offset into index buffer, divided by 3
    int leafCnt = 0;

    AABB aabb;          ///< AABB in world space, update only when object moved
    AABB viewportAABB;  ///< view port(screen space aabb)

    bool isLeaf() const { return vaoOffset != -1; }
    bool isPrimitiveValid() const { return primOffset != -1; }
    bool hasLeft() const { return left != -1; }
    bool hasRight() const { return right != -1; }
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

    void updateViewportData();

    void reset();

   private:
    std::vector<uint32_t> mVaoOffsetToLeaf;
    std::vector<BVHNode> mNodes;
};
}  // namespace Rastery