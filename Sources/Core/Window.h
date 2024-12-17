#pragma once
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "Core/Macros.h"
#include "Core/Math.h"

struct GLFWwindow;

namespace Rastery {

enum class MouseButton {
    None,
    Left,
    Right,
};

struct MouseEvent {
    enum class Type {
        ButtonDown,  ///< Mouse button was pressed
        ButtonUp,    ///< Mouse button was released
        Move,        ///< Mouse cursor position changed
        Wheel        ///< Mouse wheel was scrolled
    };
    Type type;         ///< Event Type.
    float2 pos;        ///< Normalized coordinates x,y in range [0, 1]. (0,0) is the top-left corner of the window.
    float2 screenPos;  ///< Screen-space coordinates in range [0, clientSize]. (0,0) is the top-left corner of the window.
    float2 wheelDelta;
    MouseButton button = MouseButton::None;  ///< GLFW button enum.
};

struct WindowDesc {
    uint32_t width = 800;
    uint32_t height = 600;
    std::string title = "Rastery";
    bool enableVSync = false;
};

class Window;

class RASTERY_API ICallback {
   public:
    virtual void handleRenderFrame() = 0;
    virtual void handleKeyEvent(int key, int action, int mods) = 0;
    virtual void handleMouseEvent(const MouseEvent& event) = 0;
    virtual void handleFileDrop(const std::vector<std::string>& paths) = 0;
    virtual void handleFrameBufferResize(int width, int height) = 0;
};

class RASTERY_API Window {
   public:
    using SharedPtr = std::shared_ptr<Window>;
    Window(const WindowDesc& desc, ICallback* pCallback);

    [[nodiscard]] const auto& getDesc() const { return mDesc; }

    float2 getNormalizedPos(const float2& screenPos) const;

    void resize(int width, int height);

    void beginLoop();

    ~Window();

    void shouldClose(bool close);

   private:
    [[nodiscard]] bool shouldClose() const;

    friend class ApiCallbacks;
    void setCallbacks();

    WindowDesc mDesc;
    GLFWwindow* mpGlfwWindow;
    ICallback* mpCallback;
};
}  // namespace Rastery