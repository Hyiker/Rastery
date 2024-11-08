#pragma once
#include "Core/API/Shader.h"
#include "Core/API/Texture.h"
#include "Macros.h"
#include "Window.h"
namespace Rastery {
class RASTERY_API App : public ICallback {
   public:
    App();

    void renderUI();
    void handleRenderFrame() override;
    void handleKeyEvent(int key, int action, int mods) override {}

    ~App();

   private:
    void beginFrame();

    Texture::SharedPtr mpPresentTexture;
    ShaderProgram::SharedPtr mpPresentShader;
    Window::SharedPtr mpWindow;
};
}  // namespace Rastery