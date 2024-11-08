#include "Window.h"

#include <glad.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "GlfwInclude.h"
#include "Utils/Logger.h"
#include "imgui_impl_glfw.h"


namespace Rastery {
class ApiCallbacks {
   public:
    static void handleFrameBufferResize(GLFWwindow* pGlfwWindow, int width,
                                        int height) {
        if (width == 0 || height == 0) {
            return;
        }

        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            pWindow->resize(width,
                            height);  // Window callback is handled in here
        }
    }

    static void handleKeyCallback(GLFWwindow* pGlfwWindow, int key, int _,
                                  int action, int mods) {
        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            pWindow->mpCallback->handleKeyEvent(key, action, mods);
        }
    }
};
Window::Window(const WindowDesc& desc, ICallback* pCallback)
    : mDesc(desc), mpCallback(pCallback) {
    // Init GLFW
    if (!glfwInit()) {
        logFatal("Failed to init GLFW");
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if RASTERY_MACOSX
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
#endif

    mpGlfwWindow = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(),
                                    nullptr, nullptr);
    glfwMakeContextCurrent(mpGlfwWindow);

    // Glad load gl from glfw
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        logFatal("Failed to init glad");
    }

    setCallbacks();

    // Gui init TODO: make it individual
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    ImGui_ImplGlfw_InitForOpenGL(mpGlfwWindow, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Window::resize(int width, int height) {
    glViewport(0, 0, width, height);
    mDesc.width = width;
    mDesc.height = height;
}
void Window::beginLoop() {
    while (!shouldClose()) {
        mpCallback->handleRenderFrame();

        glfwSwapBuffers(mpGlfwWindow);
        glfwPollEvents();
    }
}

Window::~Window() {
    glfwDestroyWindow(mpGlfwWindow);
    glfwTerminate();
}
void Window::shouldClose(bool close) {
    glfwSetWindowShouldClose(mpGlfwWindow, close ? GLFW_TRUE : GLFW_FALSE);
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(mpGlfwWindow) != GLFW_FALSE;
}

void Window::setCallbacks() {
    glfwSetWindowUserPointer(mpGlfwWindow, this);
    glfwSetFramebufferSizeCallback(mpGlfwWindow,
                                   &ApiCallbacks::handleFrameBufferResize);
    glfwSetKeyCallback(mpGlfwWindow, &ApiCallbacks::handleKeyCallback);
}
}  // namespace Rastery