#ifndef VR_MATH_H
#define VR_MATH_H

// Quaternion helpers shared by vr_eye and vr_controls (the VR plan,
// planning/future-features/VR-TABLETOP.md). Quaternions are (x, y, z, w),
// unit length, rotating column vectors.

#include <cmath>

namespace vr_math {

// v rotated by q
inline void rotate(const float q[4], const float v[3], float out[3])
{
    // t = 2 * cross(q.xyz, v); out = v + w * t + cross(q.xyz, t)
    const float t[3] = {2.0f * (q[1] * v[2] - q[2] * v[1]),
                        2.0f * (q[2] * v[0] - q[0] * v[2]),
                        2.0f * (q[0] * v[1] - q[1] * v[0])};
    out[0] = v[0] + q[3] * t[0] + (q[1] * t[2] - q[2] * t[1]);
    out[1] = v[1] + q[3] * t[1] + (q[2] * t[0] - q[0] * t[2]);
    out[2] = v[2] + q[3] * t[2] + (q[0] * t[1] - q[1] * t[0]);
}

// v rotated by the inverse of q
inline void rotateInverse(const float q[4], const float v[3], float out[3])
{
    const float conjugate[4] = {-q[0], -q[1], -q[2], q[3]};
    rotate(conjugate, v, out);
}

// the rotation matrix of q, m[row][column]
inline void toMatrix(const float q[4], float m[3][3])
{
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    m[0][0] = 1.0f - 2.0f * (y * y + z * z); m[0][1] = 2.0f * (x * y - z * w);        m[0][2] = 2.0f * (x * z + y * w);
    m[1][0] = 2.0f * (x * y + z * w);        m[1][1] = 1.0f - 2.0f * (x * x + z * z); m[1][2] = 2.0f * (y * z - x * w);
    m[2][0] = 2.0f * (x * z - y * w);        m[2][1] = 2.0f * (y * z + x * w);        m[2][2] = 1.0f - 2.0f * (x * x + y * y);
}

// the rotation whose columns are the orthonormal axes x, y, z
inline void fromAxes(const float x[3], const float y[3], const float z[3], float q[4])
{
    const float trace = x[0] + y[1] + z[2];
    if(trace > 0.0f) {
        const float s = 2.0f * std::sqrt(1.0f + trace);
        q[3] = 0.25f * s;
        q[0] = (y[2] - z[1]) / s;
        q[1] = (z[0] - x[2]) / s;
        q[2] = (x[1] - y[0]) / s;
    } else if(x[0] > y[1] && x[0] > z[2]) {
        const float s = 2.0f * std::sqrt(1.0f + x[0] - y[1] - z[2]);
        q[3] = (y[2] - z[1]) / s;
        q[0] = 0.25f * s;
        q[1] = (y[0] + x[1]) / s;
        q[2] = (z[0] + x[2]) / s;
    } else if(y[1] > z[2]) {
        const float s = 2.0f * std::sqrt(1.0f + y[1] - x[0] - z[2]);
        q[3] = (z[0] - x[2]) / s;
        q[0] = (y[0] + x[1]) / s;
        q[1] = 0.25f * s;
        q[2] = (z[1] + y[2]) / s;
    } else {
        const float s = 2.0f * std::sqrt(1.0f + z[2] - x[0] - y[1]);
        q[3] = (x[1] - y[0]) / s;
        q[0] = (z[0] + x[2]) / s;
        q[1] = (z[1] + y[2]) / s;
        q[2] = 0.25f * s;
    }
}

}

#endif // VR_MATH_H
