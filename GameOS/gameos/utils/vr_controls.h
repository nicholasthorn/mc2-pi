#ifndef VR_CONTROLS_H
#define VR_CONTROLS_H

// The Quest controllers of the VR plan's Phase 3
// (planning/future-features/VR-TABLETOP.md). GL-free so it can be unit
// tested. Everything is in OpenXR's axes: x right, y up, z back, metres.

#include <cstdint>

namespace vr_controls {

// A position and an orientation quaternion (x, y, z, w).
struct Pose {
    float position[3];
    float orientation[4];
};

// Where a pointing ray meets a quad, as the quad's image coordinates: u from
// its left edge, v from its top, both 0 to 1. The ray starts at ray.position
// and points along the pose's -z (as OpenXR's aim pose does); the quad is
// width x height, centred on quad.position, facing along the pose's +z. False
// when the ray misses, points away, or comes from behind the quad. distance,
// when given, is how far along the ray the hit is.
bool rayHitsQuad(const Pose& ray, const Pose& quad, float width, float height, float& u, float& v,
                 float* distance = nullptr);

// The point on a quad at image coordinates u, v (as rayHitsQuad gives them).
void quadPoint(const Pose& quad, float width, float height, float u, float v, float out[3]);

// The point distance metres along a ray (its pose's -z).
void rayPoint(const Pose& ray, float distance, float out[3]);

// The pointer's beam as a quad: centred between from and to, its height
// (y) along the beam, length long, turned about the beam to face eye.
Pose beamPose(const float from[3], const float to[3], const float eye[3], float& length);

// A quad 'distance' metres from origin, 'down' radians below the horizontal,
// in the direction 'yaw' (radians about +y, so positive is to the left; 0
// is -z), facing back at origin.
Pose hudPose(const float origin[3], float yaw, float distance, float down);

// Holding the pointer still through a click. Pulling a trigger moves the
// hand, and the game takes a press that moves more than 1.7% of the screen
// for a drag (a selection box instead of a move order). From when a trigger
// or grip starts to move (pull past 0.15) until it is let go (below 0.1),
// the pointer stays where the pull started, unless it moves more than
// dragDistance from there (in u, v units): that is a drag, and the pointer
// follows the hand until it is let go.
struct ClickHold {
    bool holding = false;
    bool dragging = false;
    float u = 0.0f, v = 0.0f;
};
void holdForClick(ClickHold& hold, float pull, float& u, float& v, float dragDistance = 0.05f);

// The heading of an orientation: the angle about +y of its -z axis, seen
// from above. Looking up or down and tilting the head do not change it.
float yawOf(const float orientation[4]);

// The camera moves the thumbsticks ask for. The left stick pans, the right
// stick turns (x) and zooms (y, up is in). A direction starts at a
// deflection of 0.5 and stops below 0.3, given the previous result, so it
// does not flicker at the threshold. Turning stops the panning and the
// zoom: the game turns on shift + arrow, so an arrow held for panning would
// turn as well, and its zoom keys do nothing while shift is down.
enum : uint32_t {
    PAN_UP = 1u << 0,
    PAN_DOWN = 1u << 1,
    PAN_LEFT = 1u << 2,
    PAN_RIGHT = 1u << 3,
    ROTATE_LEFT = 1u << 4,
    ROTATE_RIGHT = 1u << 5,
    ZOOM_IN = 1u << 6,
    ZOOM_OUT = 1u << 7,
};
uint32_t stickActions(const float leftStick[2], const float rightStick[2], uint32_t previous);

}

#endif // VR_CONTROLS_H
