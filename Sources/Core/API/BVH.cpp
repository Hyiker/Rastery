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
    const uint32_t kSliceSize = std::ceil(float(n) / BVHNode::kMaxChildrenCount);
    for (uint32_t i = 0; i < nodes.size(); i += kSliceSize) {
        int child = recursiveBVHBuild(nodes.subspan(i, std::min<int>(kSliceSize, nodes.size() - i)), target, depth + 1);
        node.aabb |= target[child].aabb;
        node.leafCnt += target[child].leafCnt;
        node.children.push_back(child);
    }

    int index = target.size();
    target.push_back(node);
    return index;
}

BVHNode& BVH::getLeafNodeByVaoOffset(int index) { return mNodes[mVaoOffsetToLeaf[index]]; }

const BVHNode& BVH::getRootNode() const { return mNodes.back(); }

BVHNode& BVH::getRootNode() { return mNodes.back(); }

void BVH::build(const CpuVao::SharedPtr& pVao) {
    std::vector<BVHLeafNode> leaves(pVao->indexData.size() / 3);
    logInfo("Start BVH build for {} leaves, {} children for each node...", leaves.size(), BVHNode::kMaxChildrenCount);
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

    for (int child : node.children) {
        node.viewportAABB |= recursiveUpdateViewportData(nodes, child);
    }

    std::sort(node.children.begin(), node.children.end(), [&](int a, int b) {
        auto &n0 = nodes[a], n1 = nodes[b];
        if (!n0.isCulledLastFrame) {
            return true;
        } else if (!n1.isCulledLastFrame) {
            return false;
        }
        return n0.viewportAABB.minPoint.z < n1.viewportAABB.minPoint.z;
    });

    return node.viewportAABB;
}

void BVH::updateViewportData() { recursiveUpdateViewportData(mNodes, mNodes.size() - 1); }

void BVH::reset() {
    gMaxDepth = 0;
    mNodes.clear();
}

}  // namespace Rastery
