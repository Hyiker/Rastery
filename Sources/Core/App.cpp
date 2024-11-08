#include "App.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>
#include <string>

#include "API/Shader.h"
#include "Core/API/Shader.h"
#include "Core/API/Texture.h"
#include "Core/Window.h"
#include "Error.h"
#include "GlfwInclude.h"
#include "Utils/Image.h"
#include "Utils/Logger.h"
#include "fmt/format.h"
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
    Logger::init();

    WindowDesc desc{.width = 800, .height = 600, .title = "RasteryApp"};
    mpWindow = std::make_shared<Window>(desc, this);
    RASTERY_CHECK_GL_ERROR();

    mpPresentShader =
        createShaderProgram({{ShaderStage::Vertex, kVertexShaderSource},
                             {ShaderStage::Fragment, kFragmentShaderSource}});
    RASTERY_CHECK_GL_ERROR();

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

    {
        float vertices[] = {
            // 位置          // 纹理坐标
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // 右上角
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,  // 右下角
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 左下角
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f   // 左上角
        };
        unsigned int indices[] = {
            0, 1, 3,  // 第一个三角形
            1, 2, 3   // 第二个三角形
        };
        unsigned int VBO, EBO;
        glGenVertexArrays(1, &mVao);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(mVao);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices,
                     GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices,
                     GL_STATIC_DRAW);

        // 位置属性
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void*)0);
        glEnableVertexAttribArray(0);

        // 纹理坐标属性
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    RASTERY_CHECK_GL_ERROR();
    mpWindow->beginLoop();
}

std::vector<unsigned char> d(800 * 600 * 4, 128);
TextureSubresourceDesc subresourceDesc{
    .width = 800,
    .height = 600,
    .depth = 0,
    .xOffset = 0,
    .yOffset = 0,
    .zOffset = 0,
    .format = GL_RGBA,
    .type = GL_UNSIGNED_BYTE,
    .pData = d.data(),
};
void App::handleRenderFrame() {
    beginFrame();

    glBindVertexArray(mVao);

    mpPresentTexture->uploadData(subresourceDesc);

    mpPresentShader->use();
    ShaderVars var = mpPresentShader->getRootVars();
    var["texture1"] = mpPresentTexture;

    RASTERY_CHECK_GL_ERROR();

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

    renderUI();
}

void App::handleKeyEvent(int key, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE) {
        mpWindow->shouldClose(true);
    }
}

void App::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    auto& io = ImGui::GetIO();
    ImGui::Begin("Status");
    ImGui::Text("%d fps, %.2f ms", int(1.f / io.DeltaTime),
                io.DeltaTime * 1000.f);

    ImGui::End();

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

    Logger::shutdown();
}

}  // namespace Rastery