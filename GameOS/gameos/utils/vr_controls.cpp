#include "vr_controls.h"

#include <cmath>

namespace vr_controls {

namespace {

// v rotated by the unit quaternion q
void rotate(const float q[4], const float v[3], float out[3])
{
    // t = 2 * cross(q.xyz, v); out = v + w * t + cross(q.xyz, t)
    const float t[3] = {2.0f * (q[1] * v[2] - q[2] * v[1]),
                        2.0f * (q[2] * v[0] - q[0] * v[2]),
                        2.0f * (q[0] * v[1] - q[1] * v[0])};
    out[0] = v[0] + q[3] * t[0] + (q[1] * t[2] - q[2] * t[1]);
    out[1] = v[1] + q[3] * t[1] + (q[2] * t[0] - q[0] * t[2]);
    out[2] = v[2] + q[3] * t[2] + (q[0] * t[1] - q[1] * t[0]);
}

void rotateInverse(const float q[4], const float v[3], float out[3])
{
    const float conjugate[4] = {-q[0], -q[1], -q[2], q[3]};
    rotate(conjugate, v, out);
}

// one stick axis: +1, -1 or 0, with hysteresis on whether it was on before
int axisDirection(float value, bool wasPositive, bool wasNegative)
{
    const float kStart = 0.5f, kStop = 0.3f;
    if(value >= (wasPositive ? kStop : kStart))
        return 1;
    if(value <= -(wasNegative ? kStop : kStart))
        return -1;
    return 0;
}

}

bool rayHitsQuad(const Pose& ray, const Pose& quad, float width, float height, float& u, float& v)
{
    const float forward[3] = {0.0f, 0.0f, -1.0f};
    float dir[3];
    rotate(ray.orientation, forward, dir);
    const float offset[3] = {ray.position[0] - quad.position[0],
                             ray.position[1] - quad.position[1],
                             ray.position[2] - quad.position[2]};
    // in the quad's frame: its surface is z = 0, its front faces +z
    float o[3], d[3];
    rotateInverse(quad.orientation, offset, o);
    rotateInverse(quad.orientation, dir, d);
    if(o[2] <= 0.0f || d[2] >= 0.0f)
        return false;
    const float t = -o[2] / d[2];
    const float x = o[0] + t * d[0];
    const float y = o[1] + t * d[1];
    u = 0.5f + x / width;
    v = 0.5f - y / height;
    return u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f;
}

Pose hudPose(const float origin[3], float yaw, float distance, float down)
{
    const float sy = std::sin(0.5f * yaw), cy = std::cos(0.5f * yaw);
    const float sp = std::sin(-0.5f * down), cp = std::cos(-0.5f * down);
    Pose pose;
    // yaw about +y after pitch about +x
    pose.orientation[0] = cy * sp;
    pose.orientation[1] = sy * cp;
    pose.orientation[2] = -sy * sp;
    pose.orientation[3] = cy * cp;
    const float ahead = distance * std::cos(down);
    pose.position[0] = origin[0] - ahead * std::sin(yaw);
    pose.position[1] = origin[1] - distance * std::sin(down);
    pose.position[2] = origin[2] - ahead * std::cos(yaw);
    return pose;
}

float yawOf(const float orientation[4])
{
    const float forward[3] = {0.0f, 0.0f, -1.0f};
    float f[3];
    rotate(orientation, forward, f);
    // forward = (-sin(yaw), ., -cos(yaw)) for a pure yaw
    return std::atan2(-f[0], -f[2]);
}

uint32_t stickActions(const float leftStick[2], const float rightStick[2], uint32_t previous)
{
    uint32_t actions = 0;
    const int turn = axisDirection(rightStick[0], (previous & ROTATE_RIGHT) != 0, (previous & ROTATE_LEFT) != 0);
    const int zoom = axisDirection(rightStick[1], (previous & ZOOM_IN) != 0, (previous & ZOOM_OUT) != 0);
    if(turn > 0)
        actions |= ROTATE_RIGHT;
    else if(turn < 0)
        actions |= ROTATE_LEFT;
    if(zoom > 0)
        actions |= ZOOM_IN;
    else if(zoom < 0)
        actions |= ZOOM_OUT;
    if(turn != 0)
        return actions;

    const int side = axisDirection(leftStick[0], (previous & PAN_RIGHT) != 0, (previous & PAN_LEFT) != 0);
    const int ahead = axisDirection(leftStick[1], (previous & PAN_UP) != 0, (previous & PAN_DOWN) != 0);
    if(side > 0)
        actions |= PAN_RIGHT;
    else if(side < 0)
        actions |= PAN_LEFT;
    if(ahead > 0)
        actions |= PAN_UP;
    else if(ahead < 0)
        actions |= PAN_DOWN;
    return actions;
}

}
