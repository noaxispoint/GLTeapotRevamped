// Minimal quaternion math for object orientation.
#pragma once

#include <cmath>

struct Vec3 {
    float x = 0, y = 0, z = 0;
};

struct Quaternion {
    float x = 0, y = 0, z = 0, w = 1;

    Quaternion() = default;
    Quaternion(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

    // Axis-angle constructor (angle in radians, axis need not be normalized).
    Quaternion(Vec3 axis, float angle) {
        float len = std::sqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);
        float s = std::sin(angle * 0.5f) / len;
        x = axis.x * s;
        y = axis.y * s;
        z = axis.z * s;
        w = std::cos(angle * 0.5f);
    }

    Quaternion operator*(const Quaternion& q) const {
        return Quaternion(
            w * q.x + x * q.w + y * q.z - z * q.y,
            w * q.y - x * q.z + y * q.w + z * q.x,
            w * q.z + x * q.y - y * q.x + z * q.w,
            w * q.w - x * q.x - y * q.y - z * q.z);
    }

    void normalize() {
        float len = std::sqrt(x * x + y * y + z * z + w * w);
        if (len > 0.0f) {
            x /= len; y /= len; z /= len; w /= len;
        }
    }

    // Column-major 4x4 matrix suitable for glMultMatrixf.
    void toOpenGLMatrix(float m[16]) const {
        float x2 = x + x, y2 = y + y, z2 = z + z;
        float xx = x * x2, xy = x * y2, xz = x * z2;
        float yy = y * y2, yz = y * z2, zz = z * z2;
        float wx = w * x2, wy = w * y2, wz = w * z2;

        m[0] = 1.0f - (yy + zz); m[4] = xy - wz;          m[8]  = xz + wy;          m[12] = 0.0f;
        m[1] = xy + wz;          m[5] = 1.0f - (xx + zz);  m[9]  = yz - wx;          m[13] = 0.0f;
        m[2] = xz - wy;          m[6] = yz + wx;           m[10] = 1.0f - (xx + yy); m[14] = 0.0f;
        m[3] = 0.0f;             m[7] = 0.0f;              m[11] = 0.0f;             m[15] = 1.0f;
    }
};
