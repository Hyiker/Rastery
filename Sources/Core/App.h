#pragma once
#include "Camera.h"
#include "Core/API/Shader.h"
#include "Core/API/Texture.h"
#include "Macros.h"
#include "Raster/RasterPipeline.h"
#include "Window.h"
namespace Rastery {
class RASTERY_API App : public ICallback {
   public:
    App();

    void renderUI();
    void handleFrameBufferResize(int width, int height) override;
    void handleRenderFrame() override;
    void handleKeyEvent(int key, int action, int mods) override;

    ~App();

   private:
    void beginFrame();

    void executeRasterizer() const;

    void blitFrameBuffer() const;

    // Scene data
    Camera::SharedPtr mpCamera;

    // Present states
    uint32_t mVao;
    Texture::SharedPtr mpPresentTexture;
    ShaderProgram::SharedPtr mpPresentShader;

    // Rasterization pipeline
    struct {
        CpuTexture::SharedPtr mpDepthTexture;
        CpuTexture::SharedPtr mpColorTexture;
        RasterPipeline::SharedPtr mpPipeline;
    } mRasterizer;

    Window::SharedPtr mpWindow;
};
}  // namespace Rastery