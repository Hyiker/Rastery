#include "RasterPipeline.h"

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>

#include <array>
#include <memory>
#include <sstream>
#include <vector>

#include "Core/API/Texture.h"
#include "Core/API/Vao.h"
#include "Core/Error.h"
#include "Utils/Gui.h"
#include "Utils/Logger.h"
#include "Utils/Timer.h"
#include "glm/gtc/quaternion.hpp"

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

            auto rasterizePoint = [&](int2 pixel) {
                if (any(glm::lessThan(pixel, int2(0))) || any(glm::greaterThanEqual(pixel, int2(width, height)))) return;

                float2 samplePoint = float2(pixel) + float2(0.5);
                float3 baryCoord = computeBarycentricCoordinate(vpCrd[0], vpCrd[1], vpCrd[2], samplePoint);
                if (!isInsidePrimitive(baryCoord)) return;

                // Interpolated fragment data in clip space
                // FIXME interpolate in linear space
                VertexOut interpolateVertex = primitive.v0 * baryCoord.x + primitive.v1 * baryCoord.y + primitive.v2 * baryCoord.z;
                interpolateVertex.rasterPosition = float4(clipToNDC(interpolateVertex.rasterPosition), interpolateVertex.rasterPosition.w);

                if (interpolateVertex.rasterPosition.z <= 0 || interpolateVertex.rasterPosition.z > 1 ||
                    !zbufferTest(samplePoint, interpolateVertex.rasterPosition.z)) {
                    return;
                }

                FragIn fragIn = interpolateVertex;
                mpColorTexture->fetch<float4>(pixel) = fragmentShader(fragIn);
            };

            switch (mDesc.rasterMode) {
                case RasterMode::Naive: {
                    tbb::parallel_for(tbb::blocked_range2d<int>(0, height, 0, width), [&](tbb::blocked_range2d<int> r) {
                        for (int y = r.rows().begin(), y_end = r.rows().end(); y < y_end; y++) {
                            for (int x = r.cols().begin(), x_end = r.cols().end(); x < x_end; x++) {
                                rasterizePoint(int2(x, y));
                            }
                        }
                    });

                } break;
                case RasterMode::BoundedNaive: {
                    std::array<float2, 3> v = vpCrd;
                    // bubble sort vertices by y
                    if (v[0].y > v[1].y) {
                        std::swap(v[0], v[1]);
                    }
                    if (v[1].y > v[2].y) {
                        std::swap(v[1], v[2]);
                    }
                    if (v[0].y > v[1].y) {
                        std::swap(v[0], v[1]);
                    }

                    float triangleHeight = v[2].y - v[0].y;
                    float upperHeight = v[1].y - v[0].y;
                    float lowerHeight = v[2].y - v[1].y;
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
                            rasterizePoint(int2(x, y));
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
                            rasterizePoint(int2(x, y));
                        }
                    });

                } break;
                default: {
                    RASTERY_UNREACHABLE();
                };
            }
        }
    } else {
    }
    timer.end();
    mStats.drawCallCount++;
    mStats.triangleCount += (uint32_t)primitives.size();
    mStats.rasterizeTime += timer.elapsedMilliseconds();
}

}  // namespace Rastery