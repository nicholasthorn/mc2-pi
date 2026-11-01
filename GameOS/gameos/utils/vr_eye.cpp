#include "utils/vr_eye.h"

#include <cmath>

namespace vr_eye {

namespace {

float dot(const float a[3], const float b[3])
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void cross(const float a[3], const float b[3], float out[3])
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

bool normalize(float v[3])
{
    const float length = std::sqrt(dot(v, v));
    if(length < 1e-6f)
        return false;
    v[0] /= length; v[1] /= length; v[2] /= length;
    return true;
}

// v minus its part along the unit vector n
void flatten(const float v[3], const float n[3], float out[3])
{
    const float d = dot(v, n);
    out[0] = v[0] - d * n[0]; out[1] = v[1] - d * n[1]; out[2] = v[2] - d * n[2];
}

// the rotation matrix of a unit quaternion, m[row][column], for column vectors
void quatToMatrix(const float q[4], float m[3][3])
{
    const float x = q[0], y = q[1], z = q[2], w = q[3];
    m[0][0] = 1.0f - 2.0f * (y * y + z * z); m[0][1] = 2.0f * (x * y - z * w);        m[0][2] = 2.0f * (x * z + y * w);
    m[1][0] = 2.0f * (x * y + z * w);        m[1][1] = 1.0f - 2.0f * (x * x + z * z); m[1][2] = 2.0f * (y * z - x * w);
    m[2][0] = 2.0f * (x * z - y * w);        m[2][1] = 2.0f * (y * z + x * w);        m[2][2] = 1.0f - 2.0f * (x * x + y * y);
}

}

void fromXr(const float xrPosition[3], const float xrOrientation[4], float position[3], float orientation[4])
{
    // a point reflection: every axis flips. A rotation's axis is a
    // pseudovector, so it flips twice (by the reflection and by the change
    // of handedness): the quaternion stays as it is.
    position[0] = -xrPosition[0];
    position[1] = -xrPosition[1];
    position[2] = -xrPosition[2];
    for(int i = 0; i < 4; ++i)
        orientation[i] = xrOrientation[i];
}

Basis eyeBasis(const Basis& game, const float worldUp[3], const float headOrientation[4],
               const float headPosition[3], float unitsPerMetre, bool keepGamePitch)
{
    float up[3] = {worldUp[0], worldUp[1], worldUp[2]};
    normalize(up);

    // the ground frame G: the game camera without its pitch, with the same
    // axes (x screen left, y screen down, z forward when level)
    float gx[3];
    flatten(game.x, up, gx);
    normalize(gx);
    // y: the vertical, on the side the camera's y (or, looking straight
    // down, its z) leans to
    float lean = dot(game.y, up);
    if(std::fabs(lean) < 1e-4f)
        lean = dot(game.z, up);
    const float side = lean < 0.0f ? -1.0f : 1.0f;
    float gy[3] = {side * up[0], side * up[1], side * up[2]};
    // z: completes the frame with the game camera's handedness
    float gz[3];
    cross(gx, gy, gz);
    float gameZ[3];
    cross(game.x, game.y, gameZ);
    if(dot(gameZ, game.z) < 0.0f) {
        gz[0] = -gz[0]; gz[1] = -gz[1]; gz[2] = -gz[2];
    }
    const float* g[3] = {gx, gy, gz};

    // the game camera's axes in G (its pitch), the head on top of them
    float pitch[3][3];   // pitch[axis][component in G]
    const float* gameAxes[3] = {game.x, game.y, game.z};
    for(int a = 0; a < 3; ++a)
        for(int c = 0; c < 3; ++c)
            pitch[a][c] = keepGamePitch ? dot(gameAxes[a], g[c]) : (a == c ? 1.0f : 0.0f);
    float head[3][3];
    quatToMatrix(headOrientation, head);

    Basis eye;
    float* eyeAxes[3] = {eye.x, eye.y, eye.z};
    for(int a = 0; a < 3; ++a) {
        float inG[3];
        for(int r = 0; r < 3; ++r)
            inG[r] = head[r][0] * pitch[a][0] + head[r][1] * pitch[a][1] + head[r][2] * pitch[a][2];
        for(int c = 0; c < 3; ++c)
            eyeAxes[a][c] = g[0][c] * inG[0] + g[1][c] * inG[1] + g[2][c] * inG[2];
    }
    for(int c = 0; c < 3; ++c)
        eye.position[c] = game.position[c] + unitsPerMetre
                * (g[0][c] * headPosition[0] + g[1][c] * headPosition[1] + g[2][c] * headPosition[2]);
    return eye;
}

void clipEdges(const float fov[4], float near, float& leftClip, float& rightClip,
               float& topClip, float& bottomClip)
{
    leftClip = -near * std::tan(fov[1]);     // the right edge, on x (screen left)
    rightClip = -near * std::tan(fov[0]);
    topClip = -near * std::tan(fov[3]);      // the bottom edge, on y (screen down)
    bottomClip = -near * std::tan(fov[2]);
}

}
