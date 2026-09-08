#include "App.h"

#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>

#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl2.h>

namespace {
constexpr float kDisplayScale = 1.0f;
constexpr float kZRatio = 10.0f;

// Dear ImGui doesn't parse '&' mnemonics on its own (unlike Win32/MFC menus),
// so we do it ourselves: an "&x" in a label means "x" is this item's
// Alt-key mnemonic. This strips the '&' for display and reports which
// ImGuiKey it names.
struct MnemonicLabel {
    std::string display;
    int mnemonicByteIndex = -1;
    ImGuiKey mnemonicKey = ImGuiKey_None;
};

MnemonicLabel ParseMnemonic(const char* raw) {
    MnemonicLabel result;
    for (const char* p = raw; *p != '\0';) {
        if (*p == '&' && *(p + 1) != '\0') {
            char c = *(p + 1);
            char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            if (upper >= 'A' && upper <= 'Z') {
                result.mnemonicByteIndex = static_cast<int>(result.display.size());
                result.mnemonicKey = static_cast<ImGuiKey>(ImGuiKey_A + (upper - 'A'));
            }
            result.display.push_back(c);
            p += 2;
        } else {
            result.display.push_back(*p);
            p += 1;
        }
    }
    return result;
}

// A top-level, horizontal menu-bar entry: Alt+<mnemonic> opens it, and while
// Alt is held its mnemonic letter is underlined. Must be closed with
// ImGui::EndMenu() exactly like a plain BeginMenu(), if it returns true.
bool BeginMnemonicMenuBar(const char* rawLabel) {
    MnemonicLabel parsed = ParseMnemonic(rawLabel);
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyAlt && parsed.mnemonicKey != ImGuiKey_None && ImGui::IsKeyPressed(parsed.mnemonicKey, false))
        ImGui::OpenPopup(parsed.display.c_str());

    if (parsed.mnemonicByteIndex >= 0 && io.KeyAlt) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        const char* text = parsed.display.c_str();
        float preWidth = ImGui::CalcTextSize(text, text + parsed.mnemonicByteIndex).x;
        float charWidth = ImGui::CalcTextSize(text + parsed.mnemonicByteIndex, text + parsed.mnemonicByteIndex + 1).x;
        float x = cursor.x + ImGui::GetStyle().ItemSpacing.x * 0.5f + preWidth;
        float y = cursor.y + ImGui::GetTextLineHeight() + 1.0f;
        drawList->AddLine(ImVec2(x, y), ImVec2(x + charWidth, y), ImGui::GetColorU32(ImGuiCol_Text));
    }

    return ImGui::BeginMenu(parsed.display.c_str());
}

// A nested (vertical, dropdown) submenu, e.g. Lights > Upper center. Same
// Alt+<mnemonic>-opens behavior, but no underline: Dear ImGui doesn't expose
// enough of its internal column layout to place it correctly here.
bool BeginMnemonicMenu(const char* rawLabel) {
    MnemonicLabel parsed = ParseMnemonic(rawLabel);
    ImGuiIO& io = ImGui::GetIO();

    if (io.KeyAlt && parsed.mnemonicKey != ImGuiKey_None && ImGui::IsKeyPressed(parsed.mnemonicKey, false))
        ImGui::OpenPopup(parsed.display.c_str());

    return ImGui::BeginMenu(parsed.display.c_str());
}

// Shared by the two leaf-item helpers below: true if this item's mnemonic
// key was just pressed while Alt is held. Only meaningful while textually
// inside the enclosing menu's open BeginMenu()/BeginPopup() block, which is
// the only place these are ever called from.
bool MnemonicKeyPressed(const MnemonicLabel& parsed) {
    return parsed.mnemonicKey != ImGuiKey_None && ImGui::GetIO().KeyAlt &&
        ImGui::IsKeyPressed(parsed.mnemonicKey, false);
}

// A plain action item (no checkmark), e.g. "Quit" or a color choice in a
// radio-style group. Returns true if clicked or its Alt+mnemonic fired;
// `selected` only controls whether it's drawn with a checkmark.
bool MnemonicMenuItem(const char* rawLabel, const char* shortcut = nullptr, bool selected = false) {
    MnemonicLabel parsed = ParseMnemonic(rawLabel);
    bool activated = ImGui::MenuItem(parsed.display.c_str(), shortcut, selected);
    return activated || MnemonicKeyPressed(parsed);
}

// A checkbox-style item bound to a bool, e.g. a Settings toggle: activating
// it (by click or Alt+mnemonic) flips *value, matching plain
// ImGui::MenuItem(label, shortcut, bool*)'s own auto-toggle behavior.
bool MnemonicCheckboxItem(const char* rawLabel, const char* shortcut, bool* value) {
    MnemonicLabel parsed = ParseMnemonic(rawLabel);
    bool activated = ImGui::MenuItem(parsed.display.c_str(), shortcut, *value);
    if (MnemonicKeyPressed(parsed))
        activated = true;
    if (activated)
        *value = !*value;
    return activated;
}
}

App::App() = default;

App::~App() {
    Shutdown();
}

int App::Run() {
    InitWindow();
    InitImGui();

    scene_.InitGL();
    glfwGetFramebufferSize(window_, &fbWidth_, &fbHeight_);
    scene_.SetupProjection(fbWidth_, fbHeight_);

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        glfwGetWindowSize(window_, &windowWidth_, &windowHeight_);

        for (Object& obj : scene_.Objects())
            obj.Update();

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        DrawMenuBar();
        DrawObjectContextMenu();

        double now = glfwGetTime();
        float fpsAvg = 0.0f;
        if (lastFrameTime_ > 0.0) {
            float instFps = static_cast<float>(1.0 / (now - lastFrameTime_));
            int entry;
            if (fpsHistCount_ < kFpsHistSize) {
                entry = (fpsOldest_ + fpsHistCount_) % kFpsHistSize;
                fpsHistCount_++;
            } else {
                entry = fpsOldest_;
                fpsOldest_ = (fpsOldest_ + 1) % kFpsHistSize;
            }
            fpsHistory_[entry] = instFps;
        }
        lastFrameTime_ = now;
        if (fpsHistCount_ > 5) {
            for (int i = 0; i < fpsHistCount_; i++)
                fpsAvg += fpsHistory_[(fpsOldest_ + i) % kFpsHistSize];
            fpsAvg /= static_cast<float>(fpsHistCount_);
            if (scene_.settings.showFps)
                DrawFpsOverlay(fpsAvg);
        }

        scene_.SetupProjection(fbWidth_, fbHeight_);
        scene_.DrawFrame(fbWidth_, fbHeight_);

        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
    }

    return 0;
}

void App::InitWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_SAMPLES, 0);

    window_ = glfwCreateWindow(windowWidth_, windowHeight_, "GLTeapot", nullptr, nullptr);
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(scene_.settings.limitFps ? 1 : 0);

    handCursor_ = glfwCreateStandardCursor(GLFW_HAND_CURSOR);

    glfwSetWindowUserPointer(window_, this);
    glfwSetMouseButtonCallback(window_, &App::MouseButtonCallback);
    glfwSetCursorPosCallback(window_, &App::CursorPosCallback);
    glfwSetFramebufferSizeCallback(window_, &App::FramebufferSizeCallback);
    glfwSetKeyCallback(window_, &App::KeyCallback);
}

void App::InitImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    // Enables Alt-key mnemonic navigation of the menu bar (the '&' in menu
    // labels below marks the underlined/mnemonic letter).
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL2_Init();
}

void App::Shutdown() {
    if (!window_)
        return;
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (handCursor_) glfwDestroyCursor(handCursor_);
    glfwDestroyWindow(window_);
    glfwTerminate();
    window_ = nullptr;
}

void App::DrawMenuBar() {
    if (!ImGui::BeginMainMenuBar())
        return;

    if (BeginMnemonicMenuBar("&GLTeapot")) {
        if (MnemonicMenuItem("&Add a teapot", "Ctrl+N"))
            scene_.AddTeapot();
        ImGui::Separator();
        if (MnemonicMenuItem("&Quit", "Ctrl+Q"))
            glfwSetWindowShouldClose(window_, GLFW_TRUE);
        ImGui::EndMenu();
    }

    if (BeginMnemonicMenuBar("&Settings")) {
        MnemonicCheckboxItem("&Perspective", nullptr, &scene_.settings.perspective);
        MnemonicCheckboxItem("&FPS display", nullptr, &scene_.settings.showFps);
        MnemonicCheckboxItem("F&illed polygons", nullptr, &scene_.settings.filled);
        MnemonicCheckboxItem("&Lighting", nullptr, &scene_.settings.lighting);
        MnemonicCheckboxItem("&Backface culling", nullptr, &scene_.settings.culling);
        MnemonicCheckboxItem("&Z-buffered", nullptr, &scene_.settings.zbuffer);
        MnemonicCheckboxItem("&Gouraud shading", nullptr, &scene_.settings.gouraud);
        MnemonicCheckboxItem("F&og", nullptr, &scene_.settings.fog);
        ImGui::Separator();
        if (MnemonicCheckboxItem("Limit FPS to &refresh rate", nullptr, &scene_.settings.limitFps))
            glfwSwapInterval(scene_.settings.limitFps ? 1 : 0);
        ImGui::EndMenu();
    }

    if (BeginMnemonicMenuBar("&Lights")) {
        DrawLightSubmenu("&Upper center", 0);
        DrawLightSubmenu("Lo&wer left", 1);
        DrawLightSubmenu("&Right", 2);
        ImGui::EndMenu();
    }

    ImGui::EndMainMenuBar();
}

void App::DrawLightSubmenu(const char* name, int lightIndex) {
    if (!BeginMnemonicMenu(name))
        return;

    LightColor current = scene_.GetLight(lightIndex);
    auto item = [&](const char* label, LightColor c) {
        if (MnemonicMenuItem(label, nullptr, current == c))
            scene_.SetLight(lightIndex, c);
    };
    item("&Off", LightColor::Off);
    ImGui::Separator();
    item("&White", LightColor::White);
    item("&Yellow", LightColor::Yellow);
    item("&Blue", LightColor::Blue);
    item("&Red", LightColor::Red);
    item("&Green", LightColor::Green);

    ImGui::EndMenu();
}

void App::DrawObjectContextMenu() {
    if (contextMenuRequested_) {
        ImGui::SetNextWindowPos(ImVec2(static_cast<float>(contextMenuX_), static_cast<float>(contextMenuY_)));
        ImGui::OpenPopup("ObjectMenu");
        contextMenuRequested_ = false;
    }

    if (!ImGui::BeginPopup("ObjectMenu"))
        return;

    if (contextMenuObject_ >= 0 && contextMenuObject_ < static_cast<int>(scene_.Objects().size())) {
        Object& obj = scene_.Objects()[contextMenuObject_];

        auto colorItem = [&](const char* label, ObjectColor c) {
            if (MnemonicMenuItem(label, nullptr, obj.color == c))
                obj.color = c;
        };
        colorItem("&White", ObjectColor::White);
        colorItem("&Yellow", ObjectColor::Yellow);
        colorItem("&Blue", ObjectColor::Blue);
        colorItem("&Red", ObjectColor::Red);
        colorItem("&Green", ObjectColor::Green);

        ImGui::Separator();

        auto solidItem = [&](const char* label, Solidity s) {
            if (MnemonicMenuItem(label, nullptr, obj.solidity == s))
                obj.solidity = s;
        };
        solidItem("&Solid", Solidity::Solid);
        solidItem("&Translucent", Solidity::Translucent);
        solidItem("Trans&parent", Solidity::Transparent);
    }

    ImGui::EndPopup();
}

void App::DrawFpsOverlay(float fps) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoFocusOnAppearing |
        ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings;

    ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + 8, vp->WorkPos.y + vp->WorkSize.y - 26));
    ImGui::Begin("##fps_overlay", nullptr, flags);
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%.1f FPS", fps);
    ImGui::End();
}

int App::PickAtCursor() {
    double sx = fbWidth_ / static_cast<double>(windowWidth_ > 0 ? windowWidth_ : 1);
    double sy = fbHeight_ / static_cast<double>(windowHeight_ > 0 ? windowHeight_ : 1);
    return scene_.PickObjectAt(cursorX_ * sx, cursorY_ * sy, fbWidth_, fbHeight_);
}

void App::UpdateHoverCursor() {
    bool hoveringNow = PickAtCursor() >= 0;
    if (hoveringNow != hovering_) {
        hovering_ = hoveringNow;
        glfwSetCursor(window_, hovering_ ? handCursor_ : nullptr);
    }
}

void App::OnMouseButton(int button, int action, int mods) {
    (void)mods;
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    if (action == GLFW_PRESS) {
        int idx = PickAtCursor();
        if (idx < 0)
            return;

        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            dragMode_ = DragMode::Rotate;
            draggedObject_ = idx;
            dragLastX_ = cursorX_;
            dragLastY_ = cursorY_;
            lastDx_ = lastDy_ = 0.0f;
            scene_.Objects()[idx].SetSpin(0.0f, 0.0f);
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
            dragMode_ = DragMode::Translate;
            draggedObject_ = idx;
            dragLastX_ = cursorX_;
            dragLastY_ = cursorY_;
        } else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
            contextMenuRequested_ = true;
            contextMenuObject_ = idx;
            contextMenuX_ = cursorX_;
            contextMenuY_ = cursorY_;
        }
    } else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && dragMode_ == DragMode::Rotate && draggedObject_ >= 0) {
            if (std::fabs(lastDx_) > 1.0f || std::fabs(lastDy_) > 1.0f)
                scene_.Objects()[draggedObject_].SetSpin(0.5f * lastDy_, 0.5f * lastDx_);
            dragMode_ = DragMode::None;
            draggedObject_ = -1;
        } else if (button == GLFW_MOUSE_BUTTON_RIGHT && dragMode_ == DragMode::Translate) {
            dragMode_ = DragMode::None;
            draggedObject_ = -1;
        }
    }
}

void App::OnCursorPos(double x, double y) {
    cursorX_ = x;
    cursorY_ = y;

    if (ImGui::GetIO().WantCaptureMouse)
        return;

    if (dragMode_ == DragMode::Rotate && draggedObject_ >= 0) {
        float dx = static_cast<float>(x - dragLastX_);
        float dy = static_cast<float>(y - dragLastY_);
        dragLastX_ = x;
        dragLastY_ = y;

        Object& obj = scene_.Objects()[draggedObject_];
        obj.SetSpin(0.0f, 0.0f);
        obj.RotateWorldSpace(dx, dy);
        lastDx_ = dx;
        lastDy_ = dy;
    } else if (dragMode_ == DragMode::Translate && draggedObject_ >= 0) {
        float dx = static_cast<float>(x - dragLastX_);
        float dy = static_cast<float>(y - dragLastY_);
        dragLastX_ = x;
        dragLastY_ = y;

        Object& obj = scene_.Objects()[draggedObject_];
        float xinc = dx * 2.0f * kDisplayScale / static_cast<float>(windowWidth_ > 0 ? windowWidth_ : 1);
        float yinc = -dy * 2.0f * kDisplayScale / static_cast<float>(windowHeight_ > 0 ? windowHeight_ : 1);
        float zinc = 0.0f;

        if (scene_.settings.perspective) {
            zinc = yinc * (obj.z / kDisplayScale);
            xinc *= -(obj.z * 4.0f / kZRatio);
            yinc *= -(obj.z * 4.0f / kZRatio);
        }

        obj.x += xinc;
        bool shiftDown = glfwGetKey(window_, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        if (shiftDown)
            obj.z += zinc;
        else
            obj.y += yinc;
    } else {
        UpdateHoverCursor();
    }
}

void App::OnFramebufferSize(int width, int height) {
    fbWidth_ = width;
    fbHeight_ = height;
    if (fbWidth_ > 0 && fbHeight_ > 0)
        scene_.SetupProjection(fbWidth_, fbHeight_);
}

void App::OnKey(int key, int action, int mods) {
    if (action != GLFW_PRESS)
        return;
    bool cmdOrCtrl = (mods & GLFW_MOD_CONTROL) || (mods & GLFW_MOD_SUPER);
    if (!cmdOrCtrl)
        return;
    if (key == GLFW_KEY_N)
        scene_.AddTeapot();
    else if (key == GLFW_KEY_Q)
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
}

void App::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    static_cast<App*>(glfwGetWindowUserPointer(window))->OnMouseButton(button, action, mods);
}

void App::CursorPosCallback(GLFWwindow* window, double x, double y) {
    static_cast<App*>(glfwGetWindowUserPointer(window))->OnCursorPos(x, y);
}

void App::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    static_cast<App*>(glfwGetWindowUserPointer(window))->OnFramebufferSize(width, height);
}

void App::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    static_cast<App*>(glfwGetWindowUserPointer(window))->OnKey(key, action, mods);
}
