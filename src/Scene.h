#pragma once

#include <vector>

#include "Object.h"

// Off/White/Yellow/Red/Blue/Green — same order & values as the original
// GLTeapot's `enum lights`, so light-menu message values map 1:1.
enum class LightColor { Off = 0, White = 1, Yellow = 2, Red = 3, Blue = 4, Green = 5 };

struct Settings {
    bool perspective = false;
    bool showFps = true;
    bool filled = true;
    bool lighting = true;
    bool culling = true;
    bool zbuffer = true;
    bool gouraud = true;
    bool fog = false;
    bool limitFps = true;
};

// Owns the teapots, the three scene lights, and the render/projection state
// that the original ObjectView managed. Call InitGL() once after a GL
// context exists, then each frame: SetupProjection (on resize), DrawFrame.
class Scene {
public:
    Scene();

    void InitGL();
    void SetupProjection(int width, int height);
    void EnforceState();
    void DrawFrame(int width, int height);

    // Returns the index of the topmost object under the given window-space
    // point (top-left origin, like GLFW cursor coords), or -1 if none.
    // Must be called (and its result used) before the next SwapBuffers,
    // since it draws a hidden ID pass into the back buffer.
    int PickObjectAt(double x, double y, int width, int height);

    void AddTeapot();
    std::vector<Object>& Objects() { return objects_; }
    const std::vector<Object>& Objects() const { return objects_; }

    void SetLight(int index, LightColor color);
    LightColor GetLight(int index) const { return lightColor_[index]; }

    Settings settings;

private:
    void ApplyLight(int index);

    std::vector<Object> objects_;
    LightColor lightColor_[3] = {LightColor::White, LightColor::Blue, LightColor::Off};
};
