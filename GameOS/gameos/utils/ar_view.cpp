#include "ar_view.h"

#include <cmath>

namespace ar_view {

void fovFromProjection(const float m[16], float fov[4])
{
    // m[0] = 2n / (r - l), m[8] = (r + l) / (r - l): r / n = (m[8] + 1) / m[0]
    // and l / n = (m[8] - 1) / m[0]; the same with m[5], m[9] for top, bottom
    fov[0] = std::atan((m[8] - 1.0f) / m[0]);
    fov[1] = std::atan((m[8] + 1.0f) / m[0]);
    fov[2] = std::atan((m[9] + 1.0f) / m[5]);
    fov[3] = std::atan((m[9] - 1.0f) / m[5]);
}

void rayThroughPixel(const float q[4], const float fov[4], float u, float v, float out[4])
{
    // the direction in the camera's own axes: across from the left edge's
    // tangent to the right's, down from the top's to the bottom's
    const float left = std::tan(fov[0]), right = std::tan(fov[1]);
    const float up = std::tan(fov[2]), down = std::tan(fov[3]);
    float d[3] = {left + u * (right - left), up + v * (down - up), -1.0f};
    const float length = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    for(float& c : d)
        c /= length;

    // the shortest turn from -z to d: axis -z x d, w 1 + (-z . d); never
    // opposite, as d always points forward
    float r[4] = {d[1], -d[0], 0.0f, 1.0f - d[2]};
    const float n = std::sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2] + r[3] * r[3]);
    for(float& c : r)
        c /= n;

    // the camera's orientation, then that turn: q * r
    out[0] = q[3] * r[0] + q[0] * r[3] + q[1] * r[2] - q[2] * r[1];
    out[1] = q[3] * r[1] - q[0] * r[2] + q[1] * r[3] + q[2] * r[0];
    out[2] = q[3] * r[2] + q[0] * r[1] - q[1] * r[0] + q[2] * r[3];
    out[3] = q[3] * r[3] - q[0] * r[0] - q[1] * r[1] - q[2] * r[2];
}

}
