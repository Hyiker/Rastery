#pragma once
#include <tbb/concurrent_vector.h>

#include <functional>
#include <memory>

#include "Core/API/Texture.h"
#include "Core/API/Vao.h"
#include "Core/Macros.h"
namespace Rastery {

struct VertexOut {
    float4 position;
    float3 normal;
    float2 texCoord;
    [[nodiscard]] float3 getPosition() const { return position; }
};

using VertexShader = std::function<VertexOut(Vertex)>;
using FragmentShader = std::function<float4()>;

enum class CullMode { BackFace, FrontFace, None };

enum class RasterMode {
    ScanLine,  ///< Use scoped scan line for each triangle
    Hierarchy  ///< Hierarchy z-buffer
};

struct RasterDesc {
    // We actually mixup framebuffer and raster state here
    int width;
    int height;
    CullMode cullMode = CullMode::None;
    RasterMode rasterMode = RasterMode::ScanLine;
};

struct TrianglePrimitive {
    VertexOut v0;
    VertexOut v1;
    VertexOut v2;
};

class RASTERY_API RasterPipeline {
   public:
    using SharedPtr = std::shared_ptr<RasterPipeline>;
    RasterPipeline(const RasterDesc& desc, const CpuTexture::SharedPtr& pDepthTexture, const CpuTexture::SharedPtr& pColorTexture);

    /** Execute rasterization pipeline.
     *
     * @param vao CPU vertex array object data
     * TODO: move shaders elsewhere
     */
    void execute(const CpuVao& vao, VertexShader vertexShader, FragmentShader fragmentShader);

   private:
    /** Vertex shader for projection misc.
     */
    [[nodiscard]] tbb::concurrent_vector<TrianglePrimitive> executeVertexShader(const CpuVao& vao, VertexShader vertexShader) const;

    

    void executeRasterization(const tbb::concurrent_vector<TrianglePrimitive>& primitives, FragmentShader fragmentShader);

    RasterDesc mDesc;
    CpuTexture::SharedPtr mpDepthTexture;
    CpuTexture::SharedPtr mpColorTexture;
};
}  // namespace Rastery