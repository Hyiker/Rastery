#include "App.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>
#include <string>

#include "Core/API/Shader.h"
#include "Core/API/Texture.h"
#include "Core/Window.h"
#include "glad.h"
namespace {
const std::string kVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;
void main() {
    gl_Position = vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

const std::string kFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;
void main() {
    FragColor = texture(texture1, TexCoord);
}
)";
}  // namespace

namespace Rastery {

App::App() {
    WindowDesc desc{.width = 800, .height = 600, .title = "RasteryApp"};
    mpWindow = std::make_shared<Window>(desc, this);

    mpPresentShader =
        createShaderProgram({{ShaderStage::Vertex, kVertexShaderSource},
                             {ShaderStage::Fragment, kFragmentShaderSource}});
    {
        TextureDesc textureDesc{.type = GL_TEXTURE_2D,
                                .format = GL_RGBA8,
                                .width = (int)desc.width,
                                .height = (int)desc.height,
                                .depth = 0,
                                .layers = 0,
                                .wrapDesc = TextureWrapDesc(),
                                .filterDesc = TextureFilterDesc()};
        mpPresentTexture = std::make_shared<Texture>(textureDesc);
    }
    mpWindow->beginLoop();
}

void App::handleRenderFrame() {
    beginFrame();

    renderUI();
}

void App::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    ImGui::ShowDemoWindow();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::beginFrame() {
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

App::~App() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

}  // namespace Rastery