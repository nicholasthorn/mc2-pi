#include "vr_controls.h"

#include <algorithm>
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

bool rayHitsQuad(const Pose& ray, const Pose& quad, float width, float height, float& u, float& v,
                 float* distance)
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
    if(distance)
        *distance = t;      // dir is a unit vector
    return u >= 0.0f && u <= 1.0f && v >= 0.0f && v <= 1.0f;
}

void quadPoint(const Pose& quad, float width, float height, float u, float v, float out[3])
{
    const float local[3] = {(u - 0.5f) * width, (0.5f - v) * height, 0.0f};
    rotate(quad.orientation, local, out);
    for(int i = 0; i < 3; ++i)
        out[i] += quad.position[i];
}

void rayPoint(const Pose& ray, float distance, float out[3])
{
    const float forward[3] = {0.0f, 0.0f, -1.0f};
    float dir[3];
    rotate(ray.orientation, forward, dir);
    for(int i = 0; i < 3; ++i)
        out[i] = ray.position[i] + distance * dir[i];
}

Pose beamPose(const float from[3], const float to[3], const float eye[3], float& length)
{
    Pose pose;
    float y[3] = {to[0] - from[0], to[1] - from[1], to[2] - from[2]};
    length = std::sqrt(y[0] * y[0] + y[1] * y[1] + y[2] * y[2]);
    for(int i = 0; i < 3; ++i)
        pose.position[i] = 0.5f * (from[i] + to[i]);
    if(length < 1e-6f) {
        pose.orientation[0] = pose.orientation[1] = pose.orientation[2] = 0.0f;
        pose.orientation[3] = 1.0f;
        return pose;
    }
    for(float& c : y)
        c /= length;
    // the face: towards the eye, with the part along the beam taken out
    float z[3] = {eye[0] - pose.position[0], eye[1] - pose.position[1], eye[2] - pose.position[2]};
    float along = z[0] * y[0] + z[1] * y[1] + z[2] * y[2];
    for(int i = 0; i < 3; ++i)
        z[i] -= along * y[i];
    float n = std::sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    if(n < 1e-6f) {
        // the eye is on the beam's line: any face square to it
        const float up[3] = {std::fabs(y[1]) < 0.9f ? 0.0f : 1.0f, std::fabs(y[1]) < 0.9f ? 1.0f : 0.0f, 0.0f};
        along = up[0] * y[0] + up[1] * y[1];
        for(int i = 0; i < 3; ++i)
            z[i] = up[i] - along * y[i];
        n = std::sqrt(z[0] * z[0] + z[1] * z[1] + z[2] * z[2]);
    }
    for(float& c : z)
        c /= n;
    const float x[3] = {y[1] * z[2] - y[2] * z[1], y[2] * z[0] - y[0] * z[2], y[0] * z[1] - y[1] * z[0]};

    // the rotation whose columns are x, y, z, as a quaternion
    const float trace = x[0] + y[1] + z[2];
    float* q = pose.orientation;
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
    return pose;
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

void holdForClick(ClickHold& hold, float pull, float& u, float& v, float dragDistance)
{
    if(pull < 0.1f) {
        hold.holding = hold.dragging = false;
        return;
    }
    if(!hold.holding && !hold.dragging && pull > 0.15f) {
        hold.holding = true;
        hold.u = u;
        hold.v = v;
    }
    if(!hold.holding)
        return;
    if(std::fabs(u - hold.u) > dragDistance || std::fabs(v - hold.v) > dragDistance) {
        hold.holding = false;
        hold.dragging = true;
        return;
    }
    u = hold.u;
    v = hold.v;
}

bool rayHitsGround(const float origin[3], const float dir[3], float (*height)(void* context, float x, float y),
                   void* context, float step, float maxDistance, float& distance)
{
    // above the ground at t, by how much (negative: below)
    auto above = [&](float t) {
        return origin[2] + t * dir[2] - height(context, origin[0] + t * dir[0], origin[1] + t * dir[1]);
    };
    float tNear = 0.0f;
    if(above(tNear) <= 0.0f)
        return false;       // starts under the ground
    for(float tFar = step; tNear < maxDistance; tFar += step) {
        tFar = std::min(tFar, maxDistance);
        if(above(tFar) <= 0.0f) {
            // the crossing is between tNear and tFar
            for(int i = 0; i < 16; ++i) {
                const float mid = 0.5f * (tNear + tFar);
                if(above(mid) > 0.0f)
                    tNear = mid;
                else
                    tFar = mid;
            }
            distance = tFar;
            return true;
        }
        tNear = tFar;
    }
    return false;
}

bool rayHitsSphere(const float origin[3], const float dir[3], const float centre[3], float radius,
                   float& distance)
{
    const float oc[3] = {centre[0] - origin[0], centre[1] - origin[1], centre[2] - origin[2]};
    const float along = oc[0] * dir[0] + oc[1] * dir[1] + oc[2] * dir[2];
    const float offAxis2 = oc[0] * oc[0] + oc[1] * oc[1] + oc[2] * oc[2] - along * along;
    const float r2 = radius * radius;
    if(offAxis2 > r2)
        return false;
    const float t = along - std::sqrt(r2 - offAxis2);
    if(t < 0.0f)
        return false;       // behind, or the origin is inside it
    distance = t;
    return true;
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
    if(turn != 0)
        return turn > 0 ? ROTATE_RIGHT : ROTATE_LEFT;
    if(zoom > 0)
        actions |= ZOOM_IN;
    else if(zoom < 0)
        actions |= ZOOM_OUT;

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
