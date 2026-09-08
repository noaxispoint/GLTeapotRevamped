#include "Scene.h"

#include <algorithm>
#include <cmath>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace {

constexpr float kDisplayScale = 1.0f;
constexpr float kDepthOfView = 30.0f;

constexpr float kDimWhite[3] = {0.25f, 0.25f, 0.25f};
constexpr float kWhite[3] = {1.0f, 1.0f, 1.0f};
constexpr float kYellow[3] = {1.0f, 1.0f, 0.0f};
constexpr float kRed[3] = {1.0f, 0.0f, 0.0f};
constexpr float kBlue[3] = {0.0f, 0.0f, 1.0f};
constexpr float kGreen[3] = {0.0f, 1.0f, 0.0f};
constexpr float kBlack[3] = {0.0f, 0.0f, 0.0f};
constexpr float kFoggy[3] = {0.4f, 0.4f, 0.4f};

const float* ColorRgb(LightColor c) {
    switch (c) {
        case LightColor::White: return kWhite;
        case LightColor::Yellow: return kYellow;
        case LightColor::Red: return kRed;
        case LightColor::Blue: return kBlue;
        case LightColor::Green: return kGreen;
        default: return kBlack;
    }
}

} // namespace

Scene::Scene() {
    objects_.emplace_back();
}

void Scene::AddTeapot() {
    objects_.emplace_back();
}

void Scene::SetLight(int index, LightColor color) {
    lightColor_[index] = color;
    ApplyLight(index);
}

void Scene::ApplyLight(int index) {
    GLenum light = GL_LIGHT0 + index;
    LightColor c = lightColor_[index];
    if (c == LightColor::Off) {
        glDisable(light);
        return;
    }
    const float* rgb = ColorRgb(c);
    float diffuse[4] = {rgb[0], rgb[1], rgb[2], 1.0f};
    float ambient[4] = {kDimWhite[0], kDimWhite[1], kDimWhite[2], 1.0f};
    glEnable(light);
    glLightfv(light, GL_AMBIENT, ambient);
    glLightfv(light, GL_DIFFUSE, diffuse);
    glLightfv(light, GL_SPECULAR, diffuse);
}

void Scene::InitGL() {
    float position0[] = {0.0f, 3.0f, 3.0f, 0.0f};
    float position1[] = {-3.0f, -3.0f, 3.0f, 0.0f};
    float position2[] = {3.0f, 0.0f, 0.0f, 0.0f};

    glEnable(GL_DITHER);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);

    glShadeModel(GL_SMOOTH);

    glLightfv(GL_LIGHT0, GL_POSITION, position0);
    glLightfv(GL_LIGHT1, GL_POSITION, position1);
    glLightfv(GL_LIGHT2, GL_POSITION, position2);
    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_FALSE);

    for (int i = 0; i < 3; i++)
        ApplyLight(i);

    glEnable(GL_LIGHTING);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);

    glMaterialf(GL_FRONT, GL_SHININESS, 0.6f * 128.0f);

    glColor3f(1.0f, 1.0f, 1.0f);
}

void Scene::SetupProjection(int width, int height) {
    if (width <= 0) width = 1;
    if (height <= 0) height = 1;

    glViewport(0, 0, width, height);

    float yxRatio = static_cast<float>(height) / static_cast<float>(width);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float scale = kDisplayScale;
    if (settings.perspective) {
        constexpr float kNear = 0.15f;
        constexpr float kFar = 120.0f;
        constexpr float kFovYDeg = 60.0f;
        float aspect = 1.0f / yxRatio; // width / height
        float top = kNear * std::tan((kFovYDeg * 0.5f) * static_cast<float>(M_PI) / 180.0f);
        float right = top * aspect;
        glFrustum(-right, right, -top, top, kNear, kFar);
    } else if (yxRatio < 1.0f) {
        glOrtho(-scale / yxRatio, scale / yxRatio, -scale, scale, -1.0, kDepthOfView * 4);
    } else {
        glOrtho(-scale, scale, -scale * yxRatio, scale * yxRatio, -1.0, kDepthOfView * 4);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Scene::EnforceState() {
    glShadeModel(settings.gouraud ? GL_SMOOTH : GL_FLAT);
    if (settings.zbuffer) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (settings.culling) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
    if (settings.lighting) glEnable(GL_LIGHTING); else glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, settings.filled ? GL_FILL : GL_LINE);

    const float* bg = kBlack;
    if (settings.fog) {
        glFogf(GL_FOG_START, 10.0f);
        glFogf(GL_FOG_DENSITY, 0.2f);
        glFogf(GL_FOG_END, kDepthOfView);
        glFogfv(GL_FOG_COLOR, kFoggy);
        glEnable(GL_FOG);
        bg = kFoggy;
    } else {
        glDisable(GL_FOG);
    }
    glClearColor(bg[0], bg[1], bg[2], 1.0f);
}

void Scene::DrawFrame(int width, int height) {
    (void)width;
    (void)height;

    glClear(GL_COLOR_BUFFER_BIT | (settings.zbuffer ? GL_DEPTH_BUFFER_BIT : 0));

    EnforceState();

    for (const Object& obj : objects_) {
        if (obj.solidity == Solidity::Solid)
            obj.Draw(false, nullptr);
    }
    for (const Object& obj : objects_) {
        if (obj.solidity != Solidity::Solid)
            obj.Draw(false, nullptr);
    }

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

int Scene::PickObjectAt(double x, double y, int width, int height) {
    if (width <= 0 || height <= 0)
        return -1;

    glShadeModel(GL_FLAT);
    glDisable(GL_LIGHTING);
    glDisable(GL_FOG);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | (settings.zbuffer ? GL_DEPTH_BUFFER_BIT : 0));

    for (size_t i = 0; i < objects_.size(); i++) {
        float idColor[3] = {(255.0f - static_cast<float>(i << 3)) / 255.0f, 0.0f, 0.0f};
        objects_[i].Draw(true, idColor);
    }

    glReadBuffer(GL_BACK);
    unsigned char pixel[4] = {0, 0, 0, 0};
    int px = static_cast<int>(x);
    int py = height - 1 - static_cast<int>(y);
    px = std::clamp(px, 0, width - 1);
    py = std::clamp(py, 0, height - 1);
    glReadPixels(px, py, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

    int objNum = (255 - pixel[0]) >> 3;

    EnforceState();

    if (objNum < 0 || objNum >= static_cast<int>(objects_.size()))
        return -1;
    return objNum;
}
