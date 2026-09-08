#pragma once

struct GLFWwindow;
struct GLFWcursor;

#include "Scene.h"

// Owns the GLFW window/GL context, Dear ImGui integration (used only for
// the in-window menu bar, the FPS overlay, and the per-teapot right-click
// style context menu — the 3D teapots themselves are drawn with plain
// fixed-function OpenGL), and all mouse/keyboard interaction.
class App {
public:
    App();
    ~App();

    // Runs the main loop until the window is closed. Returns a process
    // exit code.
    int Run();

private:
    void InitWindow();
    void InitImGui();
    void Shutdown();

    void DrawMenuBar();
    void DrawLightSubmenu(const char* name, int lightIndex);
    void DrawObjectContextMenu();
    void DrawFpsOverlay(float fps);

    void OnMouseButton(int button, int action, int mods);
    void OnCursorPos(double x, double y);
    void OnFramebufferSize(int width, int height);
    void OnKey(int key, int action, int mods);

    void UpdateHoverCursor();
    int PickAtCursor();

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPosCallback(GLFWwindow* window, double x, double y);
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* window_ = nullptr;
    GLFWcursor* handCursor_ = nullptr;

    Scene scene_;

    // Logical window size (points) vs. framebuffer size (pixels); they
    // differ on HiDPI/Retina displays, and OpenGL viewport/picking must use
    // framebuffer pixels while ImGui/GLFW cursor coordinates use points.
    int windowWidth_ = 480;
    int windowHeight_ = 420;
    int fbWidth_ = 480;
    int fbHeight_ = 420;

    enum class DragMode { None, Rotate, Translate };
    DragMode dragMode_ = DragMode::None;
    int draggedObject_ = -1;
    double cursorX_ = 0.0;
    double cursorY_ = 0.0;
    double dragLastX_ = 0.0;
    double dragLastY_ = 0.0;
    float lastDx_ = 0.0f;
    float lastDy_ = 0.0f;
    bool hovering_ = false;

    bool contextMenuRequested_ = false;
    int contextMenuObject_ = -1;
    double contextMenuX_ = 0.0;
    double contextMenuY_ = 0.0;

    double lastFrameTime_ = 0.0;
    static constexpr int kFpsHistSize = 10;
    float fpsHistory_[kFpsHistSize] = {};
    int fpsHistCount_ = 0;
    int fpsOldest_ = 0;
};
