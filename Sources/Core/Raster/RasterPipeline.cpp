#include "RasterPipeline.h"

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>

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
    result.position = lhs.position + rhs.position;
    result.normal = lhs.normal + rhs.normal;
    result.texCoord = lhs.texCoord + rhs.texCoord;
    return result;
}

VertexOut operator-(const VertexOut& lhs, const VertexOut& rhs) {
    VertexOut result;
    result.position = lhs.position - rhs.position;
    result.normal = lhs.normal - rhs.normal;
    result.texCoord = lhs.texCoord - rhs.texCoord;
    return result;
}

VertexOut operator*(const VertexOut& vertex, float scalar) {
    VertexOut result;
    result.position = vertex.position * scalar;
    result.normal = vertex.normal * scalar;
    result.texCoord = vertex.texCoord * scalar;
    return result;
}

VertexOut operator*(float scalar, const VertexOut& vertex) {
    return vertex * scalar;  // 利用前面定义的乘法
}

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

static bool isFrontFace(const TrianglePrimitive& primitive) {
    // RHS
    float3 forward = float3(0, 0, -1);
    float3 v0v1 = primitive.v1.getPosition() - primitive.v0.getPosition();
    float3 v0v2 = primitive.v2.getPosition() - primitive.v0.getPosition();

    float3 primForward = cross(v0v2, v0v1);
    // If the cross product points the opposite direction to forward, then front faced
    return dot(primForward, forward) < 0;
}

// TODO static std::vector<VertexOut> clipVerticesWithPlane(const std::vector<VertexOut>& inVertices, float3 normal, float3 p) {
//     std::vector<VertexOut> result;
//     // Iterate the polygon edges over the clip plane
//     for (size_t i = 0; i <= inVertices.size(); i++) {
//         int nextI = i % (inVertices.size());

//     }
// }

/** Clip the primitive and make homogeneous division.
 */
static void clipPrimitive(TrianglePrimitive prim, tbb::concurrent_vector<TrianglePrimitive>& out) {
    // Clip space coordinates
    float w = prim.v0.position.w;
    if (w <= 0) {
        return;
    }
    prim.v0.position /= w;
    prim.v1.position /= w;
    prim.v2.position /= w;

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
        tbb::parallel_for_each(indexData.begin(), indexData.end(), [&](uint32_t index) {
            index = indexData[index];
            VertexOut vOut = vertexShader(vertexData[index]);

            vertexResult[index] = vOut;
        });
    }

    tbb::concurrent_vector<TrianglePrimitive> primitives;
    primitives.reserve(vertexResult.size() / 3);

    CullMode cullMode = mDesc.cullMode;
    auto cullFunc = [cullMode](bool frontFace) {
        if (cullMode == CullMode::None) return true;
        if ((cullMode == CullMode::FrontFace && !frontFace) || (cullMode == CullMode::BackFace && frontFace)) return true;
        return false;
    };

    tbb::parallel_for(0, (int)vertexResult.size() / 3, [&](int index) {
        TrianglePrimitive primitive;
        int vIndex = index * 3;
        primitive.v0 = vertexResult[vIndex + 0];
        primitive.v1 = vertexResult[vIndex + 1];
        primitive.v2 = vertexResult[vIndex + 2];

        if (!cullFunc(isFrontFace(primitive))) {
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
static float2 ndcToScreenSpace(int width, int height, float3 ndcCoord) {
    float2 pixel = float2(ndcCoord.x, -ndcCoord.y);
    pixel = pixel * float2(0.5) + float2(0.5);
    pixel *= float2(width, height);
    return float3(pixel, ndcCoord.z);
}

static bool isInsidePrimitive(float2 v0, float2 v1, float2 v2, float2 p, float3& barycentricCoord) {
    float2 v0v1 = v1 - v0;
    float2 v0v2 = v2 - v0;
    float2 v0p = p - v0;

    float det_v1v2 = v0v1.x * v0v2.y - v0v1.y * v0v2.x;
    float det_pv2 = v0p.x * v0v2.y - v0p.y * v0v2.x;
    float det_v1p = v0v1.x * v0p.y - v0v1.y * v0p.x;

    float a = det_pv2 / det_v1v2;
    float b = det_v1p / det_v1v2;

    barycentricCoord = float3(a, b, 1 - a - b);

    return a > 0.0 && b > 0.0 && (a + b) < 1.0;
}
bool RasterPipeline::zbufferTest(float2 sample, float depth) {
    float fragDepth = mpColorTexture->fetch<float>(uint2(sample));
    bool passed = depth > fragDepth;
    if (passed) {
        // RHS + ZO depth, the larger the closer
        mpColorTexture->fetch<float>(uint2(sample)) = depth;
    }
    return passed;
}

void RasterPipeline::executeRasterization(const tbb::concurrent_vector<TrianglePrimitive>& primitives, FragmentShader fragmentShader) {
    Timer timer;

    int width = mDesc.width;
    int height = mDesc.height;
    // Cull the pixels in screen space
    if (mDesc.rasterMode == RasterMode::Naive || mDesc.rasterMode == RasterMode::ScanLine) {
        for (const auto& primitive : primitives) {
            float2 v[3];
            v[0] = ndcToScreenSpace(width, height, primitive.v0.getPosition());
            v[1] = ndcToScreenSpace(width, height, primitive.v1.getPosition());
            v[2] = ndcToScreenSpace(width, height, primitive.v2.getPosition());

            auto rasterizePoint = [&](int2 pixel) {
                if (any(glm::lessThan(pixel, int2(0))) || any(glm::greaterThanEqual(pixel, int2(width, height)))) return;

                float2 samplePoint = float2(pixel) + float2(0.5);
                float3 baryCoord;
                if (!isInsidePrimitive(v[0], v[1], v[2], samplePoint, baryCoord)) return;

                // Interpolated fragment data
                VertexOut interpolateVertex = primitive.v0 * baryCoord.x + primitive.v1 * baryCoord.y + primitive.v2 * baryCoord.z;

                if (!zbufferTest(samplePoint, interpolateVertex.position.z)) {
                    return;
                }

                mpColorTexture->fetch<float4>(pixel) = fragmentShader();
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
                case RasterMode::ScanLine: {
                    // Sort vertices by y
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

                    // Upper triangle
                    tbb::parallel_for(0, (int)std::ceil(upperHeight), [&](int yOffset) {
                        float y = std::floor(v[0].y) + float(yOffset);
                        float xLeft = std::floor(std::lerp(v[0].x, v[1].x, (y + 1 - v[0].y) / upperHeight));
                        float xRight = std::ceil(std::lerp(v[0].x, vMid.x, (y + 1 - v[0].y) / upperHeight));
                        for (float x = xLeft; x <= xRight; x += 1.f) {
                            rasterizePoint(int2(x, y));
                        }
                    });

                    // Lower triangle
                    tbb::parallel_for(0, (int)std::ceil(lowerHeight), [&](int yOffset) {
                        float y = std::floor(v[1].y) + float(yOffset);
                        float xLeft = std::floor(std::lerp(v[1].x, v[2].x, (y - v[1].y - 1) / lowerHeight));
                        float xRight = std::ceil(std::lerp(vMid.x, v[2].x, (y - vMid.y - 1) / lowerHeight));
                        for (float x = xLeft; x <= xRight; x += 1.f) {
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
    mStats.triangleCount += primitives.size();
    mStats.rasterizeTime += timer.elapsedMilliseconds();
}

}  // namespace Rastery