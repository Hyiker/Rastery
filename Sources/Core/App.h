#pragma once
#include "Camera.h"
#include "CameraController.h"
#include "Core/API/Shader.h"
#include "Core/API/Texture.h"
#include "Core/API/Vao.h"
#include "Macros.h"
#include "Raster/RasterPipeline.h"
#include "Window.h"
namespace Rastery {

enum class VisualizeMode { Normal, PseudoPrimitiveColor, Depth };

RASTERY_ENUM_INFO(VisualizeMode, {
                                     {VisualizeMode::Normal, "Normal"},
                                     {VisualizeMode::PseudoPrimitiveColor, "PseudoPrimitiveColor"},
                                     {VisualizeMode::Depth, "Depth"},
                                 })

RASTERY_ENUM_REGISTER(VisualizeMode)

class RASTERY_API App : public ICallback {
   public:
    App();

    void import(const std::filesystem::path& p);

    void renderUI();
    void handleFrameBufferResize(int width, int height) override;
    void handleRenderFrame() override;
    void handleFileDrop(const std::vector<std::string>& paths) override;
    void handleKeyEvent(int key, int action, int mods) override;
    void handleMouseEvent(const MouseEvent& event) override;

    ~App();

    void run() const;

   private:
    void beginFrame();

    void executeRasterizer();

    void blitFrameBuffer() const;

    // Scene data
    Camera::SharedPtr mpCamera;
    OrbiterCameraController::SharedPtr mpCameraControl;

    // Present states
    uint32_t mVao;
    Texture::SharedPtr mpPresentTexture;
    ShaderProgram::SharedPtr mpPresentShader;

    // Model
    CpuVao::SharedPtr mpModelVao;

    // Rasterization pipeline
    struct {
        CpuTexture::SharedPtr mpDepthTexture;
        CpuTexture::SharedPtr mpColorTexture;
        RasterPipeline::SharedPtr mpPipeline;
    } mRasterizer;
    BVH::SharedPtr mpBVH;
    Window::SharedPtr mpWindow;

    // Params
    VisualizeMode mVisualizeMode = VisualizeMode::PseudoPrimitiveColor;
    int2 mSelectedPixel = int2(-1, -1);

    // Statistics
    RasterizerDebugData mRasterizerDebugData;
    uint32_t mFrameCount = 0u;
};
}  // namespace Rastery