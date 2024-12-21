#pragma once
#include <tbb/concurrent_vector.h>

#include <functional>
#include <memory>

#include "Core/API/BVH.h"
#include "Core/API/Texture.h"
#include "Core/API/Vao.h"
#include "Core/Enum.h"
#include "Core/Macros.h"

namespace Rastery {

/** Vertex/fragment attributes.
 */
struct VertexOut {
    float4 rasterPosition;  ///< xyz = NDC coordinate, w keeps the original depth(before homogeneous division)

    float3 position;  ///< User defined position
    float3 normal;
    float2 texCoord;

    [[nodiscard]] float3 getPosition() const { return position; }

    friend VertexOut operator+(const VertexOut& lhs, const VertexOut& rhs);
    friend VertexOut operator-(const VertexOut& lhs, const VertexOut& rhs);
    friend VertexOut operator*(const VertexOut& vertex, float scalar);
    friend VertexOut operator*(float scalar, const VertexOut& vertex);
};

struct TrianglePrimitive {
    uint32_t id;  ///< primitive index, actually primitive index in Vao
    VertexOut v0;
    VertexOut v1;
    VertexOut v2;
};

struct EdgeItem {
    const TrianglePrimitive* pPrimitive;
    float x0;  ///< Edge upper vertex start x
    float x;   ///< Edge upper vertex start x
    float dx;  ///< dx = 1 / k
    int dy;    ///< scanline count across the edge
};

struct RasterizerDebugData {
    struct DebugEdgeItem {
        TrianglePrimitive primitive;
        float x0;  ///< Edge upper vertex start x
        float dx;  ///< dx = 1 / k
        int dy;    ///< scanline count across the edge
        DebugEdgeItem() = default;
        DebugEdgeItem(const EdgeItem& item) : primitive(*item.pPrimitive), x0(item.x0), dx(item.dx), dy(item.dy) {}
    } activeEdgePair[2];
};

/** General graphics context data.
 */
struct GraphicsContextData {
    // Rasterization context
    uint32_t primitiveId;
    float2 sampleCrd;

    // Debug
    RasterizerDebugData debugData;

    GraphicsContextData(uint32_t primId, float2 sampleCrd) : primitiveId(primId), sampleCrd(sampleCrd) {}
};

using FragIn = VertexOut;

using VertexShader = std::function<VertexOut(Vertex)>;
using FragmentShader = std::function<float4(const FragIn&, const GraphicsContextData&)>;

enum class CullMode { BackFace, FrontFace, None };

RASTERY_ENUM_INFO(CullMode, {
                                {CullMode::BackFace, "BackFace"},
                                {CullMode::FrontFace, "FrontFace"},
                                {CullMode::None, "None"},
                            })

RASTERY_ENUM_REGISTER(CullMode)

enum class RasterMode {
    Naive,            ///< Very slow
    BoundedNaive,     ///< Faster naive per primitive drawing
    ScanLineZBuffer,  ///< Scan line z-buffer with AET
};

RASTERY_ENUM_INFO(RasterMode, {
                                  {RasterMode::Naive, "Naive"},
                                  {RasterMode::BoundedNaive, "BoundedNaive"},
                                  {RasterMode::ScanLineZBuffer, "ScanLineZBuffer"},
                              })

RASTERY_ENUM_REGISTER(RasterMode)

struct RasterDesc {
    // We actually mixup framebuffer and raster state here
    int width;
    int height;
    CullMode cullMode = CullMode::BackFace;
    RasterMode rasterMode = RasterMode::BoundedNaive;

    bool useHierarchicalZBuffer = true;     ///< Enable HiZ for primitive culling
    bool useAccelerationStructure = false;  ///< Enable spatial acceleration structure
};

class RASTERY_API RasterPipeline {
   public:
    struct Stats {
        uint32_t triangleCount = 0;  ///< Triangle primitive count.(after culling)
        uint32_t drawCallCount = 0;  ///< Time of draw call count.
        float rasterizeTime = 0;     ///< Rasterization time in ms.

        uint32_t hiZCullCount = 0;
    };

    using SharedPtr = std::shared_ptr<RasterPipeline>;
    RasterPipeline(const RasterDesc& desc, const CpuTexture::SharedPtr& pDepthTexture, const CpuTexture::SharedPtr& pColorTexture);

    void setRasterMode(RasterMode mode) { mDesc.rasterMode = mode; }

    RasterMode getRasterMode() const { return mDesc.rasterMode; }

    void beginFrame();

    /** Execute rasterization pipeline.
     *
     * @param vao CPU vertex array object data
     * TODO: move shaders elsewhere
     */
    void draw(const CpuVao& vao, BVH& bvh, VertexShader vertexShader, FragmentShader fragmentShader);

    void renderUI();

    bool useHiZ() const;

    bool useAccelerationStructure() const;

   private:
    Stats mStats;

    void renderStats() const;

    bool earlyHiZBufferTest(const AABB& vpBounds) const;
    
    bool earlyHiZBufferTest(const AABB& vpBounds, int layer) const;

    /** Down-top update hierarchical z-buffer pyramid in the range.
     */
    void cascadeUpdateHiZBuffer(std::pair<uint2, uint2> range);

    bool zBufferTest(float2 sample, float depth);

    /** Vertex shader for projection misc.
     */
    [[nodiscard]] tbb::concurrent_vector<TrianglePrimitive> executeVertexShader(const CpuVao& vao, VertexShader vertexShader) const;

    void rasterizePrimitive(const TrianglePrimitive& primitive, FragmentShader fragmentShader);

    void rasterizePoint(int2 pixel, std::span<const float3, 3> viewportCrds, const TrianglePrimitive& primitive,
                        FragmentShader fragmentShader, RasterizerDebugData* pDebugData = nullptr);

    void prepareRasterization(const tbb::concurrent_vector<TrianglePrimitive>& primitives, BVH& bvh);

    void executeRasterization(const tbb::concurrent_vector<TrianglePrimitive>& primitives, BVH& bvh, FragmentShader fragmentShader);

    void scanlineZBuffer(const tbb::concurrent_vector<TrianglePrimitive>& primitives, FragmentShader fragmentShader);

    RasterDesc mDesc;

    std::vector<CpuTexture::SharedPtr> mHiZDepthTextures;
    CpuTexture::SharedPtr mpDepthTexture;
    CpuTexture::SharedPtr mpColorTexture;
};
}  // namespace Rastery