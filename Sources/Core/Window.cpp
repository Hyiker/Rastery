#include "Window.h"

#include <glad.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Error.h"
#include "GLFW/glfw3.h"
#include "GlfwInclude.h"
#include "Utils/Logger.h"

namespace Rastery {

static MouseButton mapButton(int button) {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return MouseButton::Right;
        default:
            return MouseButton::None;
    }
}

static MouseEvent::Type mapMouseEvent(int event) {
    switch (event) {
        case GLFW_PRESS:
            return MouseEvent::Type::ButtonDown;
        case GLFW_RELEASE:
            return MouseEvent::Type::ButtonUp;
    }
    RASTERY_UNREACHABLE();
}

class ApiCallbacks {
   public:
    static void handleFrameBufferResize(GLFWwindow* pGlfwWindow, int width, int height) {
        if (width == 0 || height == 0) {
            return;
        }

        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            pWindow->resize(width,
                            height);  // Window callback is handled in here
            pWindow->mpCallback->handleFrameBufferResize(width, height);
        }
    }

    static void handleKeyEvent(GLFWwindow* pGlfwWindow, int key, int _, int action, int mods) {
        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            pWindow->mpCallback->handleKeyEvent(key, action, mods);
        }
    }

    static void handleFileDrop(GLFWwindow* pGlfwWindow, int pathCount, const char* paths[]) {
        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            std::vector<std::string> pathVector(pathCount);
            for (int i = 0; i < pathCount; i++) {
                pathVector[i] = paths[i];
            }
            pWindow->mpCallback->handleFileDrop(pathVector);
        }
    }

    static void handleCursorPos(GLFWwindow* pGlfwWindow, double xpos, double ypos) {
        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            float2 screenPos = float2(xpos, ypos);
            MouseEvent event;
            event.type = MouseEvent::Type::Move;
            event.pos = pWindow->getNormalizedPos(screenPos);
            event.screenPos = screenPos;
            pWindow->mpCallback->handleMouseEvent(event);
        }
    }

    static void handleMouseButton(GLFWwindow* pGlfwWindow, int button, int action, int mods) {
        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            double xpos, ypos;
            glfwGetCursorPos(pGlfwWindow, &xpos, &ypos);
            float2 size = float2(pWindow->getDesc().width, pWindow->getDesc().height);
            float2 screenPos = float2(xpos, ypos);

            MouseEvent event;
            event.type = mapMouseEvent(action);
            event.button = mapButton(button);
            event.pos = pWindow->getNormalizedPos(screenPos);
            event.screenPos = screenPos;
            pWindow->mpCallback->handleMouseEvent(event);
        }
    }
    static void handleScroll(GLFWwindow* pGlfwWindow, double xoffset, double yoffset) {
        auto* pWindow = (Window*)glfwGetWindowUserPointer(pGlfwWindow);
        if (pWindow != nullptr) {
            double xpos, ypos;
            glfwGetCursorPos(pGlfwWindow, &xpos, &ypos);
            float2 size = float2(pWindow->getDesc().width, pWindow->getDesc().height);
            float2 screenPos = float2(xpos, ypos);

            MouseEvent event;
            event.type = MouseEvent::Type::Wheel;
            event.wheelDelta = float2(xoffset, yoffset);
            event.pos = pWindow->getNormalizedPos(screenPos);
            event.screenPos = screenPos;
            pWindow->mpCallback->handleMouseEvent(event);
        }
    }
};
Window::Window(const WindowDesc& desc, ICallback* pCallback) : mDesc(desc), mpCallback(pCallback) {
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

    mpGlfwWindow = glfwCreateWindow(desc.width, desc.height, desc.title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(mpGlfwWindow);
    if (!desc.enableVSync) {
        glfwSwapInterval(0);
    }

    // Glad load gl from glfw
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        logFatal("Failed to init glad");
    }

    setCallbacks();

    resize(desc.width, desc.height);

    // Gui init TODO: make it individual
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(mpGlfwWindow, true);
    ImGui_ImplGlfw_SetCallbacksChainForAllWindows(false);
    ImGui_ImplOpenGL3_Init("#version 410");
}

float2 Window::getNormalizedPos(const float2& screenPos) const { return screenPos / float2(mDesc.width, mDesc.height); }

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
void Window::shouldClose(bool close) { glfwSetWindowShouldClose(mpGlfwWindow, close ? GLFW_TRUE : GLFW_FALSE); }

bool Window::shouldClose() const { return glfwWindowShouldClose(mpGlfwWindow) != GLFW_FALSE; }

void Window::setCallbacks() {
    glfwSetWindowUserPointer(mpGlfwWindow, this);
    glfwSetFramebufferSizeCallback(mpGlfwWindow, &ApiCallbacks::handleFrameBufferResize);
    glfwSetKeyCallback(mpGlfwWindow, &ApiCallbacks::handleKeyEvent);
    glfwSetDropCallback(mpGlfwWindow, &ApiCallbacks::handleFileDrop);
    glfwSetCursorPosCallback(mpGlfwWindow, &ApiCallbacks::handleCursorPos);
    glfwSetMouseButtonCallback(mpGlfwWindow, &ApiCallbacks::handleMouseButton);
    glfwSetScrollCallback(mpGlfwWindow, &ApiCallbacks::handleScroll);
}
}  // namespace Rastery