#include "App.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <memory>
#include <string>

#include "API/Shader.h"
#include "Camera.h"
#include "Core/API/Shader.h"
#include "Core/API/Texture.h"
#include "Core/API/Vao.h"
#include "Core/Math.h"
#include "Core/Window.h"
#include "Error.h"
#include "GlfwInclude.h"
#include "Raster/RasterPipeline.h"
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
    FragColor = texture(texture1, vec2(TexCoord.x, 1.0 - TexCoord.y));
}
)";
}  // namespace

namespace Rastery {

App::App() {
    Logger::init();

    WindowDesc desc{.width = 800, .height = 600, .title = "RasteryApp", .enableVSync = false};
    mpWindow = std::make_shared<Window>(desc, this);
    RASTERY_CHECK_GL_ERROR();

    mpPresentShader = createShaderProgram({{ShaderStage::Vertex, kVertexShaderSource}, {ShaderStage::Fragment, kFragmentShaderSource}});
    RASTERY_CHECK_GL_ERROR();

    {
        float vertices[] = {
            // 位置          // 纹理坐标
            1.0f,  1.0f,  0.0f, 1.0f, 1.0f,  // 右上角
            1.0f,  -1.0f, 0.0f, 1.0f, 0.0f,  // 右下角
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,  // 左下角
            -1.0f, 1.0f,  0.0f, 0.0f, 1.0f   // 左上角
        };
        unsigned int indices[] = {0, 1, 3, 1, 2, 3};
        unsigned int VBO, EBO;
        glGenVertexArrays(1, &mVao);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(mVao);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        RASTERY_CHECK_GL_ERROR();
    }

    // Create scene data
    mpCamera = std::make_shared<Camera>();

    handleFrameBufferResize(desc.width, desc.height);

    mpWindow->beginLoop();
}

void App::handleFrameBufferResize(int width, int height) {
    // Resize present texture
    {
        TextureDesc textureDesc{.type = GL_TEXTURE_2D,
                                .format = TextureFormat::Rgba8,
                                .width = (int)width,
                                .height = (int)height,
                                .depth = 0,
                                .layers = 0,
                                .wrapDesc = TextureWrapDesc(),
                                .filterDesc = TextureFilterDesc()};

        mpPresentTexture = std::make_shared<Texture>(textureDesc);
    }

    if (mpCamera) {
        mpCamera->setAspectRatio(float(width) / height);
    }

    // Recreate rasterization pipeline
    {
        TextureDesc depthDesc{.type = GL_TEXTURE_2D,
                              .format = TextureFormat::Rgba32F,
                              .width = (int)width,
                              .height = (int)height,
                              .depth = 0,
                              .layers = 0,
                              .wrapDesc = TextureWrapDesc(),
                              .filterDesc = TextureFilterDesc()};

        auto pDepthTexture = std::make_shared<CpuTexture>(depthDesc);

        TextureDesc colorDesc{.type = GL_TEXTURE_2D,
                              .format = TextureFormat::Rgba32F,
                              .width = (int)width,
                              .height = (int)height,
                              .depth = 0,
                              .layers = 0,
                              .wrapDesc = TextureWrapDesc(),
                              .filterDesc = TextureFilterDesc()};

        auto pColorTexture = std::make_shared<CpuTexture>(colorDesc);

        mRasterizer.mpColorTexture = pColorTexture;
        mRasterizer.mpDepthTexture = pDepthTexture;

        RasterDesc rasterDesc;
        rasterDesc.width = width;
        rasterDesc.height = height;
        mRasterizer.mpPipeline = std::make_shared<RasterPipeline>(rasterDesc, pDepthTexture, pColorTexture);
    }
}

void App::handleRenderFrame() {
    beginFrame();

    executeRasterizer();

    blitFrameBuffer();

    renderUI();

    mFrameCount++;
}

void App::executeRasterizer() const {
    CpuVao vao;
    vao.indexData = {};
    Vertex v0, v1, v2;
    v0.position = float3(0, 0.3, 0);
    v1.position = float3(-0.5, -0.3, 0);
    v2.position = float3(0.5, -0.7, 0);
    vao.vertexData = {v0, v1, v2};

    CameraData data = mpCamera->getData();

    auto modelMatrix = float4x4(1.f);
    auto normalMatrix = transpose(inverse(data.viewMat * modelMatrix));

    auto vertexShader = [&](Vertex v) {
        VertexOut out;
        out.position = data.projViewMat * modelMatrix * float4(v.position, 1.f);
        out.normal = float3(normalMatrix * float4(v.normal, 0.f));
        out.texCoord = v.texCoord;
        return out;
    };

    auto fragmentShader = [&]() { return float4(1.f, 0.f, 0.f, 0.f); };

    mRasterizer.mpPipeline->execute(vao, vertexShader, fragmentShader);
}

void App::blitFrameBuffer() const {
    // Upload texture data
    {
        const auto& desc = mRasterizer.mpColorTexture->getDesc();
        TextureSubresourceDesc subDesc{};

        subDesc.width = desc.width;
        subDesc.height = desc.height;
        subDesc.format = desc.format;
        subDesc.pData = mRasterizer.mpColorTexture->getPtr();
        mpPresentTexture->uploadData(subDesc);
    }

    glBindVertexArray(mVao);
    mpPresentShader->use();
    ShaderVars var = mpPresentShader->getRootVars();
    var["texture1"] = mpPresentTexture;
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
    RASTERY_CHECK_GL_ERROR();
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

    ImGui::Begin("Crafting table");
    ImGui::Text("%d fps, %.2f ms", int(1.f / io.DeltaTime), io.DeltaTime * 1000.f);

    if (ImGui::CollapsingHeader("Rasterizer", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Pipeline
        mRasterizer.mpPipeline->renderUI();
    }

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void App::beginFrame() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    mRasterizer.mpColorTexture->clear(float4(0, 0, 0, 1));
    mRasterizer.mpDepthTexture->clear(float4(0, 0, 0, 1));

    if (mpCamera) {
        mpCamera->computeCameraData();
    }
}

App::~App() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    Logger::shutdown();
}

}  // namespace Rastery