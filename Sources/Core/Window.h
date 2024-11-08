#pragma once
#include <cstdint>
#include <memory>
#include <string>

#include "Core/Macros.h"
struct GLFWwindow;
namespace Rastery {
struct WindowDesc {
    uint32_t width = 800;
    uint32_t height = 600;
    std::string title = "Rastery";
};

class Window;

class RASTERY_API ICallback {
   public:
    virtual void handleRenderFrame() = 0;
    virtual void handleKeyEvent(int key, int action, int mods) = 0;
};

class RASTERY_API Window {
   public:
    using SharedPtr = std::shared_ptr<Window>;
    Window(const WindowDesc& desc, ICallback* pCallback);

    [[nodiscard]] const auto& getDesc() const { return mDesc; }

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