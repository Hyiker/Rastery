#include "BVH.h"

#include <span>
#include <vector>

#include "Utils/Logger.h"

namespace Rastery {

BVH::BVH() {}

struct BVHLeafNode {
    int vaoOffset;
    AABB aabb;
};

int gMaxDepth = 0;

/** Recursive BVH build, nodes sequence in target should be ordered bottom-top.
 */
static int recursiveBVHBuild(std::span<BVHLeafNode> nodes, std::vector<BVHNode>& target, int depth) {
    gMaxDepth = std::max(depth, gMaxDepth);
    uint32_t n = nodes.size();
    if (n == 1) {
        int index = target.size();
        BVHNode node;
        node.vaoOffset = nodes[0].vaoOffset;
        node.aabb = nodes[0].aabb;
        node.leafCnt = 1;
        target.push_back(node);
        return index;
    }
    BVHNode node;
    std::sort(nodes.begin(), nodes.end(),
              [depth](const BVHLeafNode& n0, const BVHLeafNode& n1) { return n0.aabb.center()[depth % 3] < n1.aabb.center()[depth % 3]; });

    uint32_t nLeft = n >> 1;
    uint32_t nRight = n - nLeft;

    if (nLeft >= 1) {
        node.left = recursiveBVHBuild(nodes.subspan(0, nLeft), target, depth + 1);
        node.aabb |= target[node.left].aabb;
        node.leafCnt += target[node.left].leafCnt;
    }

    if (nRight >= 1) {
        node.right = recursiveBVHBuild(nodes.subspan(nLeft, nRight), target, depth + 1);
        node.aabb |= target[node.right].aabb;
        node.leafCnt += target[node.right].leafCnt;
    }

    int index = target.size();
    if (nLeft >= 1) {
        target[node.left].father = index;
    }

    if (nRight >= 1) {
        target[node.right].father = index;
    }

    target.push_back(node);
    return index;
}

BVHNode& BVH::getLeafNodeByVaoOffset(int index) { return mNodes[mVaoOffsetToLeaf[index]]; }

const BVHNode& BVH::getRootNode() const { return mNodes.back(); }

BVHNode& BVH::getRootNode() { return mNodes.back(); }

void BVH::build(const CpuVao::SharedPtr& pVao) {
    std::vector<BVHLeafNode> leaves(pVao->indexData.size() / 3);
    logInfo("Start BVH build[{}]...", leaves.size());
    for (int i = 0; i < leaves.size(); ++i) {
        leaves[i].vaoOffset = i;
        AABB aabb;
        aabb |= pVao->vertexData[pVao->indexData[i * 3]].position;
        aabb |= pVao->vertexData[pVao->indexData[i * 3 + 1]].position;
        aabb |= pVao->vertexData[pVao->indexData[i * 3 + 2]].position;
        leaves[i].aabb = aabb;
    }

    if (leaves.empty()) return;

    recursiveBVHBuild(leaves, mNodes, 0);

    mVaoOffsetToLeaf.resize(leaves.size());

    for (int i = 0; i < mNodes.size(); i++) {
        auto& node = mNodes[i];
        if (node.isLeaf()) {
            mVaoOffsetToLeaf[node.vaoOffset] = i;
        }
    }

    logInfo("BVH::build statistics: nodes={}, depth={}, leaves = {}", mNodes.size(), gMaxDepth + 1, getRootNode().leafCnt);
}

AABB recursiveUpdateViewportData(std::vector<BVHNode>& nodes, int nodeIndex) {
    auto& node = nodes[nodeIndex];
    if (node.isLeaf()) return node.viewportAABB;
    node.viewportAABB = AABB();

    if (node.hasLeft()) {
        node.viewportAABB |= recursiveUpdateViewportData(nodes, node.left);
    }

    if (node.hasRight()) {
        node.viewportAABB |= recursiveUpdateViewportData(nodes, node.right);
    }

    return node.viewportAABB;
}

void BVH::updateViewportData() { recursiveUpdateViewportData(mNodes, mNodes.size() - 1); }

void BVH::reset() {
    gMaxDepth = 0;
    mNodes.clear();
}

}  // namespace Rastery
