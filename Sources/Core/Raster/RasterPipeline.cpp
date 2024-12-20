#include "RasterPipeline.h"

#include <Utils/Algorithms.h>
#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Core/API/Texture.h"
#include "Core/API/Vao.h"
#include "Core/Color.h"
#include "Core/Error.h"
#include "Utils/Gui.h"
#include "Utils/Logger.h"
#include "Utils/Timer.h"

namespace Rastery {

VertexOut operator+(const VertexOut& lhs, const VertexOut& rhs) {
    VertexOut result;
    result.rasterPosition = lhs.rasterPosition + rhs.rasterPosition;
    result.position = lhs.position + rhs.position;
    result.normal = lhs.normal + rhs.normal;
    result.texCoord = lhs.texCoord + rhs.texCoord;
    return result;
}

VertexOut operator-(const VertexOut& lhs, const VertexOut& rhs) {
    VertexOut result;
    result.rasterPosition = lhs.rasterPosition + rhs.rasterPosition;
    result.position = lhs.position - rhs.position;
    result.normal = lhs.normal - rhs.normal;
    result.texCoord = lhs.texCoord - rhs.texCoord;
    return result;
}

VertexOut operator*(const VertexOut& vertex, float scalar) {
    VertexOut result;
    result.rasterPosition = vertex.rasterPosition * scalar;
    result.position = vertex.position * scalar;
    result.normal = vertex.normal * scalar;
    result.texCoord = vertex.texCoord * scalar;
    return result;
}

VertexOut operator*(float scalar, const VertexOut& vertex) { return vertex * scalar; }

RasterPipeline::RasterPipeline(const RasterDesc& desc, const CpuTexture::SharedPtr& pDepthTexture,
                               const CpuTexture::SharedPtr& pColorTexture)
    : mDesc(desc), mpDepthTexture(pDepthTexture), mpColorTexture(pColorTexture) {}

void RasterPipeline::beginFrame() {
    // Clear stats
    mStats.drawCallCount = 0;
    mStats.triangleCount = 0;
    mStats.rasterizeTime = 0.f;
}

void RasterPipeline::draw(const CpuVao& vao, VertexShader vertexShader, FragmentShader fragmentShader) {
    auto primitives = executeVertexShader(vao, vertexShader);

    executeRasterization(primitives, fragmentShader);
}
void RasterPipeline::renderUI() {
    dropdown("Cull Mode", mDesc.cullMode);

    dropdown("Raster Mode", mDesc.rasterMode);

    renderStats();
}

void RasterPipeline::renderStats() const {
    std::stringstream ss;

    ss << "Statistics:\n"
       << fmt::format("Rasterize time: {:.2f}ms\n", mStats.rasterizeTime) << "Draw call count: " << mStats.drawCallCount
       << "\nTriangle count: " << mStats.triangleCount;

    ImGui::Text("%s", ss.str().c_str());
}

static float3 clipToNDC(float4 clipCoord) { return clipCoord / clipCoord.w; }

static bool isClockwise(const TrianglePrimitive& primitive) {
    // RHS
    // Check if the primitive is defined clockwise in homogeneous coordinates:
    // https://en.wikipedia.org/wiki/Back-face_culling
    float3 v0v1 = clipToNDC(primitive.v1.rasterPosition) - clipToNDC(primitive.v0.rasterPosition);
    float3 v0v2 = clipToNDC(primitive.v2.rasterPosition) - clipToNDC(primitive.v0.rasterPosition);

    // If the cross product points the opposite direction to forward, then front faced
    return (v0v1.x * v0v2.y - v0v2.x * v0v1.y) > 0.f;
}

// TODO static std::vector<VertexOut> clipVerticesWithPlane(const std::vector<VertexOut>& inVertices, float3 normal, float3 p) {
//     std::vector<VertexOut> result;
//     // Iterate the polygon edges over the clip plane
//     for (size_t i = 0; i <= inVertices.size(); i++) {
//         int nextI = i % (inVertices.size());

//     }
// }

/** Clip the primitive.
 */
static void clipPrimitive(TrianglePrimitive prim, tbb::concurrent_vector<TrianglePrimitive>& out) {
    // Clip space coordinates
    out.push_back(prim);
    // std::vector<VertexOut> vertices{prim.v0, prim.v1, prim.v2};
    // Intersect the triangle with clip cube
}

tbb::concurrent_vector<TrianglePrimitive> RasterPipeline::executeVertexShader(const CpuVao& vao, VertexShader vertexShader) const {
    const auto& indexData = vao.indexData;
    const auto& vertexData = vao.vertexData;

    tbb::concurrent_vector<VertexOut> vertexResult(indexData.empty() ? vertexData.size() : indexData.size());

    if (indexData.empty()) {
        tbb::parallel_for(0, (int)vertexData.size(), [&](int index) { vertexResult[index] = vertexShader(vertexData[index]); });
    } else {
        tbb::parallel_for(0, (int)indexData.size(), [&](int i) { vertexResult[i] = vertexShader(vertexData[indexData[i]]); });
    }

    tbb::concurrent_vector<TrianglePrimitive> primitives;
    primitives.reserve(vertexResult.size() / 3);

    CullMode cullMode = mDesc.cullMode;
    auto cullFunc = [cullMode](bool frontFace) {
        return (cullMode == CullMode::None) || (cullMode == CullMode::FrontFace && !frontFace) ||
               (cullMode == CullMode::BackFace && frontFace);
    };

    tbb::parallel_for(0, (int)vertexResult.size() / 3, [&](int index) {
        TrianglePrimitive primitive;
        int vIndex = index * 3;
        RASTERY_ASSERT(vIndex + 2 < int(vertexResult.size()));
        primitive.v0 = vertexResult[vIndex + 0];
        primitive.v1 = vertexResult[vIndex + 1];
        primitive.v2 = vertexResult[vIndex + 2];
        primitive.id = uint32_t(index);

        if (!cullFunc(isClockwise(primitive))) {
            return;
        }

        // Clip primitive
        clipPrimitive(primitive, primitives);
    });

    primitives.shrink_to_fit();

    return primitives;
}

/** Convert from NDC((-1, -1, 0) - (1, 1, 1))
 * to screen space(0, 0) - (width, height)
 */
static float2 ndcToViewport(int width, int height, float3 ndcCoord) {
    float2 pixel = float2(ndcCoord.x, -ndcCoord.y);
    pixel = pixel * float2(0.5) + float2(0.5);
    pixel *= float2(width, height);
    return float3(pixel, ndcCoord.z);
}

static float3 computeBarycentricCoordinate(float2 v0, float2 v1, float2 v2, float2 p) {
    float2 v0v1 = v1 - v0;
    float2 v0v2 = v2 - v0;
    float2 v0p = p - v0;

    float det_v1v2 = v0v1.x * v0v2.y - v0v1.y * v0v2.x;
    float det_pv2 = v0p.x * v0v2.y - v0p.y * v0v2.x;
    float det_v1p = v0v1.x * v0p.y - v0v1.y * v0p.x;

    float a = det_pv2 / det_v1v2;
    float b = det_v1p / det_v1v2;
    return float3(1 - a - b, a, b);
}

static bool isInsidePrimitive(float3 baryCoord) { return baryCoord.x >= 0 && baryCoord.y >= 0 && baryCoord.z >= 0 && baryCoord.z <= 1; }

bool RasterPipeline::zbufferTest(float2 sample, float depth) {
    float fragDepth = mpDepthTexture->fetch<float>(uint2(sample));
    bool passed = depth < fragDepth;

    if (passed) {
        // RHS + ZO depth, the smaller the closer
        mpDepthTexture->fetch<float>(uint2(sample)) = depth;
    }
    return passed;
}

void RasterPipeline::rasterizePoint(int2 pixel, std::span<const float2, 3> viewportCrds, const TrianglePrimitive& primitive,
                                    FragmentShader fragmentShader, RasterizerDebugData* pDebugData) {
    int width = mDesc.width;
    int height = mDesc.height;

    if (any(glm::lessThan(pixel, int2(0))) || any(glm::greaterThanEqual(pixel, int2(width, height)))) return;

    float2 samplePoint = float2(pixel) + float2(0.5);
    float3 baryCoord = computeBarycentricCoordinate(viewportCrds[0], viewportCrds[1], viewportCrds[2], samplePoint);

    if (!isInsidePrimitive(baryCoord)) return;

    // Interpolated fragment data in clip space
    // FIXME interpolate in linear space
    VertexOut interpolateVertex = primitive.v0 * baryCoord.x + primitive.v1 * baryCoord.y + primitive.v2 * baryCoord.z;
    interpolateVertex.rasterPosition = float4(clipToNDC(interpolateVertex.rasterPosition), interpolateVertex.rasterPosition.w);

    if (interpolateVertex.rasterPosition.z <= 0 || interpolateVertex.rasterPosition.z > 1 ||
        !zbufferTest(samplePoint, interpolateVertex.rasterPosition.z)) {
        return;
    }

    // Prepare fragment and context data
    FragIn fragIn = interpolateVertex;
    GraphicsContextData context(primitive.id, samplePoint);
    if (pDebugData) {
        context.debugData = *pDebugData;
    }
    mpColorTexture->fetch<float4>(pixel) = fragmentShader(fragIn, context);
}

void RasterPipeline::executeRasterization(const tbb::concurrent_vector<TrianglePrimitive>& primitives, FragmentShader fragmentShader) {
    Timer timer;

    int width = mDesc.width;
    int height = mDesc.height;
    // Cull the pixels in screen space
    if (mDesc.rasterMode == RasterMode::Naive || mDesc.rasterMode == RasterMode::BoundedNaive) {
        for (const auto& primitive : primitives) {
            std::array<float2, 3> vpCrd;
            vpCrd[0] = ndcToViewport(width, height, clipToNDC(primitive.v0.rasterPosition));
            vpCrd[1] = ndcToViewport(width, height, clipToNDC(primitive.v1.rasterPosition));
            vpCrd[2] = ndcToViewport(width, height, clipToNDC(primitive.v2.rasterPosition));

            switch (mDesc.rasterMode) {
                case RasterMode::Naive: {
                    tbb::parallel_for(tbb::blocked_range2d<int>(0, height, 0, width), [&](tbb::blocked_range2d<int> r) {
                        for (int y = r.rows().begin(), y_end = r.rows().end(); y < y_end; y++) {
                            for (int x = r.cols().begin(), x_end = r.cols().end(); x < x_end; x++) {
                                rasterizePoint(int2(x, y), vpCrd, primitive, fragmentShader);
                            }
                        }
                    });

                } break;
                case RasterMode::BoundedNaive: {
                    // Create a copy of vpCrd for sorting
                    std::array<float2, 3> v = vpCrd;
                    // bubble sort vertices by y
                    sort3(v[0], v[1], v[2], [](const float2& v1, const float2& v2) { return v1.y < v2.y; });

                    float triangleHeight = v[2].y - v[0].y;
                    float upperHeight = v[1].y - v[0].y;
                    float2 vMid = lerp(v[0], v[2], upperHeight / triangleHeight);
                    // Ensure vMid on the right side
                    if (vMid.x < v[1].x) std::swap(vMid, v[1]);

                    // The triangle should look like
                    //      + v0
                    //    +  +
                    // v1+    + vMid
                    //    +    +
                    //      +   +
                    //        +  +
                    //           +  v2

                    // Upper triangle
                    // 3.5 - 0.5 produce 3.0, compensate it
                    int yCnt = std::ceil(v[1].y) - std::floor(v[0].y);
                    tbb::parallel_for(0, yCnt, [&](int yOffset) {
                        float y = std::floor(v[0].y) + float(yOffset);
                        // (y - y2) / (y1 - y2) = (x - x2) / (x1 - x2)
                        auto xLeft = (int)std::floor(std::min((y - v[1].y) / (v[0].y - v[1].y) * (v[0].x - v[1].x) + v[1].x,
                                                              (y + 1.f - v[1].y) / (v[0].y - v[1].y) * (v[0].x - v[1].x) + v[1].x));
                        auto xRight = (int)std::ceil(std::max((y - vMid.y) / (v[0].y - vMid.y) * (v[0].x - vMid.x) + vMid.x,
                                                              (y + 1.f - vMid.y) / (v[0].y - vMid.y) * (v[0].x - vMid.x) + vMid.x));
                        for (int x = xLeft; x <= xRight; x++) {
                            rasterizePoint(int2(x, y), vpCrd, primitive, fragmentShader);
                        }
                    });

                    // Lower triangle
                    yCnt = std::ceil(v[2].y) - std::floor(v[1].y);
                    tbb::parallel_for(0, yCnt, [&](int yOffset) {
                        float y = std::floor(v[1].y) + float(yOffset);
                        auto xLeft = (int)std::floor(std::min((y - v[2].y) / (v[1].y - v[2].y) * (v[1].x - v[2].x) + v[2].x,
                                                              (y + 1.f - v[2].y) / (v[1].y - v[2].y) * (v[1].x - v[2].x) + v[2].x));
                        auto xRight = (int)std::ceil(std::max((y - v[2].y) / (vMid.y - v[2].y) * (vMid.x - v[2].x) + v[2].x,
                                                              (y + 1.f - v[2].y) / (vMid.y - v[2].y) * (vMid.x - v[2].x) + v[2].x));

                        for (int x = xLeft; x <= xRight; x++) {
                            rasterizePoint(int2(x, y), vpCrd, primitive, fragmentShader);
                        }
                    });

                } break;
                default: {
                    RASTERY_UNREACHABLE();
                };
            }
        }
    } else if (mDesc.rasterMode == RasterMode::ScanLineZBuffer) {
        scanlineZBuffer(primitives, fragmentShader);
    }

    timer.end();
    mStats.drawCallCount++;
    mStats.triangleCount += (uint32_t)primitives.size();
    mStats.rasterizeTime += timer.elapsedMilliseconds();
}

struct PrimitiveItem {
    const TrianglePrimitive* pPrimitive;
    std::array<float2, 3> vpCrd;  ///< view port coordinates for barycentric coordinate compute
    int dy;
};

using ClassifiedPrimitiveTable = std::vector<std::vector<PrimitiveItem>>;
using ClassifiedEdgeTable = std::vector<std::unordered_map<const TrianglePrimitive*, std::vector<EdgeItem>>>;

struct ActiveEdgePairItem {
    std::pair<EdgeItem, EdgeItem> edgePair;
    float depth;
};

using ActivePrimitiveTable = std::vector<PrimitiveItem>;
using ActiveEdgePairTable = std::unordered_map<const TrianglePrimitive*, ActiveEdgePairItem>;

static bool prepareEdgeItem(const TrianglePrimitive& primitive, const float2& p0, const float2& p1, int width, int height,
                            ClassifiedEdgeTable& table) {
    EdgeItem item;
    item.pPrimitive = &primitive;
    item.x = p0.x;
    float k = (p1.y - p0.y) / (p1.x - p0.x);
    item.dx = 1.f / k;
    if (std::isinf(item.dx)) {
        item.dx = 0.0;
    }
    item.dy = std::ceil(p1.y) - std::floor(p0.y);
    int y = std::floor(p0.y);
    if (y >= height) {
        return false;
    }

    if (y < 0) {
        // clip y
        item.x += -y * item.dx;
        item.dy += y;
        y = 0;
    }
    item.x -= (p0.y - std::floor(p0.y)) * item.dx;
    item.x0 = item.x;
    table[y][&primitive].push_back(item);
    return true;
}

void RasterPipeline::scanlineZBuffer(const tbb::concurrent_vector<TrianglePrimitive>& primitives, FragmentShader fragmentShader) {
    int width = mDesc.width;
    int height = mDesc.height;

    ClassifiedPrimitiveTable cpt(height, {});
    ClassifiedEdgeTable cet(height, {});
    // 1. Build classified polygon&edge table
    for (const TrianglePrimitive& primitive : primitives) {
        std::array<float2, 3> vpCrd;
        vpCrd[0] = ndcToViewport(width, height, clipToNDC(primitive.v0.rasterPosition));
        vpCrd[1] = ndcToViewport(width, height, clipToNDC(primitive.v1.rasterPosition));
        vpCrd[2] = ndcToViewport(width, height, clipToNDC(primitive.v2.rasterPosition));
        PrimitiveItem item;
        item.vpCrd = vpCrd;
        // bubble sort vertices by y
        sort3(vpCrd[0], vpCrd[1], vpCrd[2], [](const float2& v1, const float2& v2) { return v1.y < v2.y; });

        int minY = std::floor(vpCrd[0].y);
        minY = std::max(0, minY);
        int maxY = std::ceil(vpCrd[2].y);
        if (minY < height && maxY >= 0) {
            // Update classified polygon table
            item.pPrimitive = &primitive;
            item.dy = maxY - minY;
            cpt[minY].push_back(item);

            // Update classified edge table
            prepareEdgeItem(primitive, vpCrd[0], vpCrd[1], width, height, cet);
            prepareEdgeItem(primitive, vpCrd[0], vpCrd[2], width, height, cet);
            prepareEdgeItem(primitive, vpCrd[1], vpCrd[2], width, height, cet);
        }
    }

    // 2. Do scanline rasterization
    ActivePrimitiveTable activePrims;
    ActiveEdgePairTable activeEdgePairs;
    for (int y = 0; y < height; y++) {
        // Update active table
        const auto& primTable = cpt[y];
        auto& edgeTable = cet[y];

        for (const auto& prim : primTable) {
            const auto* pId = prim.pPrimitive;
            if (edgeTable.find(pId) == edgeTable.end()) {
                logFatal("Can't find edge of primitive!");
            }

            auto& edgePair = edgeTable.at(prim.pPrimitive);
            if (edgePair.size() == 1 || edgePair.size() == 0) {
                logFatal("Invalid edge pair size = {}", edgePair.size());
            }

            activePrims.push_back(prim);

            auto addEdgePair = [&](EdgeItem it0, EdgeItem it1) {
                ActiveEdgePairItem activeEdgePair;
                if (it0.x > it1.x || (it0.x == it1.x && (it0.dy > it1.dy))) {
                    std::swap(it0, it1);
                }
                activeEdgePair.edgePair = {it0, it1};

                activeEdgePairs[pId] = activeEdgePair;
            };

            if (edgePair.size() == 3) {
                // 3 edges starts at the same place
                // Put in the tallest and shortest ones
                sort3(edgePair[0], edgePair[1], edgePair[2], [](const EdgeItem& it0, const EdgeItem& it1) { return it0.dy < it1.dy; });
                addEdgePair(edgePair[0], edgePair[2]);
            } else {
                addEdgePair(edgePair[0], edgePair[1]);
            }
        }

        ActivePrimitiveTable activePrimsNew;
        activePrimsNew.reserve(activePrims.size());

        for (auto& activePrim : activePrims) {
            const auto* pId = activePrim.pPrimitive;
            auto aepIt = activeEdgePairs.find(pId);
            if (aepIt == activeEdgePairs.end()) {
                logFatal("Can't find edge pair of primitive!");
            }
            auto& activeEdgePair = aepIt->second;
            auto& [edge0, edge1] = activeEdgePair.edgePair;

            if (edge0.x > edge1.x) {
                std::swap(edge0, edge1);
            }
            int upperX0 = std::floor(edge0.x), upperX1 = std::ceil(edge1.x);
            int lowerX0 = std::floor(edge0.x + edge0.dx), lowerX1 = std::ceil(edge1.x + edge1.dx);

            int xLeft = std::min<int>({upperX0, lowerX0, upperX1, lowerX1});
            int xRight = std::max<int>({upperX0, lowerX0, upperX1, lowerX1});
            // Rasterize point
            auto debugData = RasterizerDebugData();
            debugData.activeEdgePair[0] = edge0;
            debugData.activeEdgePair[1] = edge1;
            for (int x = xLeft; x <= xRight; x++) {
                int2 pixel = int2(x, y);
                // mpColorTexture->fetch<float4>(pixel) = float4(pseudoColor(pId->id), 1);

                rasterizePoint(pixel, activePrim.vpCrd, *pId, fragmentShader, &debugData);
            }
            edge0.x += edge0.dx;
            edge1.x += edge1.dx;
            int dyLeft = --edge0.dy;
            int dyRight = --edge1.dy;

            activePrim.dy--;
            if (activePrim.dy <= 0) {
                activeEdgePairs.erase(aepIt);
            } else {
                activePrimsNew.push_back(activePrim);
                auto cetIt = cet[y].find(pId);
                if ((dyLeft <= 0 || dyRight <= 0) && cetIt != cet[y].end() && !cetIt->second.empty()) {
                    // Find a new edge to replace it
                    EdgeItem newEdge = cetIt->second[cetIt->second.size() == 3 ? 1 : 0];
                    std::swap(dyLeft <= 0 ? edge0 : edge1, newEdge);
                }
            }
        }
        activePrimsNew.shrink_to_fit();
        std::swap(activePrimsNew, activePrims);
    }
}

}  // namespace Rastery