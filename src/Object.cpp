#include "Object.h"

#include "TeapotGeometry.h"

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

namespace {

// Tessellation grid density for each teapot. The original app's static mesh
// looked roughly like this; feel free to raise it for a smoother teapot.
constexpr int kTeapotGrid = 8;

struct Material {
    float ambient[3];
    float diffuse[3];
    float specular[3];
};

// Same values as the original GLTeapot's `materials[]` table, indexed by
// ObjectColor's underlying int (White=1 .. Green=5).
const Material kMaterials[6] = {
    // 0: unused
    {{0.1745f, 0.03175f, 0.03175f}, {0.61424f, 0.10136f, 0.10136f}, {0.727811f, 0.626959f, 0.626959f}},
    // 1: White
    {{0.1745f, 0.1745f, 0.1745f}, {0.61424f, 0.61424f, 0.61424f}, {0.727811f, 0.727811f, 0.727811f}},
    // 2: Yellow
    {{0.1745f, 0.1745f, 0.03175f}, {0.61424f, 0.61424f, 0.10136f}, {0.727811f, 0.727811f, 0.626959f}},
    // 3: Blue
    {{0.03175f, 0.03175f, 0.1745f}, {0.10136f, 0.10136f, 0.61424f}, {0.626959f, 0.626959f, 0.727811f}},
    // 4: Red
    {{0.1745f, 0.03175f, 0.03175f}, {0.61424f, 0.10136f, 0.10136f}, {0.727811f, 0.626959f, 0.626959f}},
    // 5: Green
    {{0.03175f, 0.1745f, 0.03175f}, {0.10136f, 0.61424f, 0.10136f}, {0.626959f, 0.727811f, 0.626959f}},
};

} // namespace

Object::Object() = default;

void Object::SetSpin(float spinAroundX, float spinAroundY) {
    fSpinX = spinAroundX;
    fSpinY = spinAroundY;
}

void Object::RotateWorldSpace(float rx, float ry) {
    rotation = Quaternion(Vec3{0.0f, 1.0f, 0.0f}, 0.01f * rx) * rotation;
    rotation = Quaternion(Vec3{1.0f, 0.0f, 0.0f}, 0.01f * ry) * rotation;
    rotation.normalize();
}

bool Object::Update() {
    if (fSpinX == 0.0f && fSpinY == 0.0f)
        return false;
    // Matches the original's SpinIt(): RotateWorldSpace(spinY, spinX).
    RotateWorldSpace(fSpinY, fSpinX);
    return true;
}

void Object::Draw(bool forId, const float idColor[3]) const {
    glPushMatrix();
    glTranslatef(x, y, z);

    float mat[16];
    rotation.toOpenGLMatrix(mat);
    glMultMatrixf(mat);

    if (forId) {
        glColor3fv(idColor);
        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    } else {
        const Material& m = kMaterials[static_cast<int>(color)];

        float alpha = 1.0f;
        if (solidity == Solidity::Translucent) alpha = 0.95f;
        else if (solidity == Solidity::Transparent) alpha = 0.6f;

        float ambient[4] = {m.ambient[0], m.ambient[1], m.ambient[2], alpha};
        float diffuse[4] = {m.diffuse[0], m.diffuse[1], m.diffuse[2], alpha};
        float specular[4] = {m.specular[0], m.specular[1], m.specular[2], alpha};

        if (solidity != Solidity::Solid) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glEnable(GL_BLEND);
            glDepthMask(GL_FALSE);
            glDisable(GL_CULL_FACE);
        } else {
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
        }

        glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
    }

    TeapotGeometry::Draw(kTeapotGrid);

    glPopMatrix();
}
