#pragma once

#include "Quaternion.h"

enum class ObjectColor { White = 1, Yellow = 2, Blue = 3, Red = 4, Green = 5 };
enum class Solidity { Solid = 0, Translucent = 1, Transparent = 2 };

// A single spinnable, draggable teapot instance, mirroring the original
// GLTeapot's GLObject/TriangleObject: position, quaternion orientation,
// a constant "auto-spin" velocity that a mouse drag interrupts and can
// relaunch (with momentum) on release, a paint color, and a solidity
// (opacity) mode set from its right-click-free (middle-click) context menu.
class Object {
public:
    Object();

    void Draw(bool forId, const float idColor[3]) const;

    // Applies one frame's worth of auto-spin (if any). Returns true if the
    // object's transform changed and a redraw is warranted.
    bool Update();

    // Sets the constant per-frame spin velocity (degrees-ish, matching the
    // original's 0.01 * value radians-per-frame convention).
    void SetSpin(float spinAroundX, float spinAroundY);

    // Applies an incremental drag rotation, exactly as the original
    // GLObject::RotateWorldSpace(rx, ry) does.
    void RotateWorldSpace(float rx, float ry);

    float x = 0.0f;
    float y = 0.0f;
    float z = -2.0f;
    Quaternion rotation;

    ObjectColor color = ObjectColor::Red;
    Solidity solidity = Solidity::Solid;

private:
    float fSpinX = 2.0f;
    float fSpinY = 2.0f;
};
