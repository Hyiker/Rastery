#include "RasterPipeline.h"

#include <tbb/blocked_range2d.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>

#include <memory>
#include <vector>

#include "Core/API/Texture.h"
#include "Core/API/Vao.h"
#include "Utils/Logger.h"
namespace Rastery {

RasterPipeline::RasterPipeline(const RasterDesc& desc, const CpuTexture::SharedPtr& pDepthTexture,
                               const CpuTexture::SharedPtr& pColorTexture)
    : mDesc(desc), mpDepthTexture(pDepthTexture), mpColorTexture(pColorTexture) {}

void RasterPipeline::execute(const CpuVao& vao, VertexShader vertexShader, FragmentShader fragmentShader) {
    auto primitives = executeVertexShader(vao, vertexShader);

    executeRasterization(primitives, fragmentShader);
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
static float3 ndcToScreenSpace(int width, int height, float3 ndcCoord) {
    float2 pixel = float2(ndcCoord.x, -ndcCoord.y);
    pixel = pixel * float2(0.5) + float2(0.5);
    pixel *= float2(width, height);
    return float3(pixel, ndcCoord.z);
}

static bool isInsidePrimitive(float2 v0, float2 v1, float2 v2, float2 p) {
    float2 v0v1 = v1 - v0;
    float2 v0v2 = v2 - v0;
    float2 v0p = p - v0;

    float det_v1v2 = v0v1.x * v0v2.y - v0v1.y * v0v2.x;
    float det_pv2 = v0p.x * v0v2.y - v0p.y * v0v2.x;
    float det_v1p = v0v1.x * v0p.y - v0v1.y * v0p.x;

    float a = det_pv2 / det_v1v2;
    float b = det_v1p / det_v1v2;

    return a > 0.0 && b > 0.0 && (a + b) < 1.0;
}

void RasterPipeline::executeRasterization(const tbb::concurrent_vector<TrianglePrimitive>& primitives, FragmentShader fragmentShader) {
    int width = mDesc.width;
    int height = mDesc.height;
    // TODO: Z-Buffer algorithm here
    // Cull the pixels in screen space
    switch (mDesc.rasterMode) {
        case RasterMode::ScanLine: {
            for (const auto& primitive : primitives) {
                // Convert from NDC to screen space
                float3 v[3];
                v[0] = ndcToScreenSpace(width, height, primitive.v0.getPosition());
                v[1] = ndcToScreenSpace(width, height, primitive.v1.getPosition());
                v[2] = ndcToScreenSpace(width, height, primitive.v2.getPosition());

                // Sort vertices by y
                if (v[0].y > v[1].y) {
                    std::swap(s[0], s[1]);
                }
                
                if (v[1].y > v[2].y) {
                    std::swap(s[1], s[2]);
                }

                if (v[0].y > v[1].y) {
                    std::swap(s[0], s[1]);
                }
                
                

                tbb::parallel_for(tbb::blocked_range2d<int>(0, height, 0, width), [&](tbb::blocked_range2d<int> r) {
                    for (int y = r.rows().begin(), y_end = r.rows().end(); y < y_end; y++) {
                        for (int x = r.cols().begin(), x_end = r.cols().end(); x < x_end; x++) {
                            float2 pixelCenter = float2(x + 0.5, y + 0.5);
                            if (isInsidePrimitive(v0, v1, v2, pixelCenter)) {
                                mpColorTexture->fetch<float4>(x, y) = fragmentShader();
                            }
                        }
                    }
                });
            }
        } break;
        case RasterMode::Hierarchy:
            break;
    }
}

}  // namespace Rastery