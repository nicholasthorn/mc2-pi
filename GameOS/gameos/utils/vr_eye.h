#ifndef VR_EYE_H
#define VR_EYE_H

// The head-tracked eyes of the VR plan's Phase 2
// (planning/future-features/VR-TABLETOP.md). GL-free so it can be unit
// tested.
//
// Engine camera axes, as Camera::projectZ maps them to the screen: x screen
// left, y screen down, z the FORWARD_AXIS row (the rows of
// Camera::cameraOrigin, whatever the names LEFT_AXIS and UP_AXIS say). The
// camera looks along minus z, whatever its name: use viewDirection(), never
// z, for where an eye or a pointer points (a pointer taking z went up when
// the hand pointed down; found in the headset, VR plan Phase 3). Eye bases
// are unaffected: they are whole matrices. OpenXR view axes: x right, y up,
// z back. The two differ by a point reflection.
namespace vr_eye {

// A camera's axes and position in world space.
struct Basis {
    float x[3];
    float y[3];
    float z[3];
    float position[3];
};

// An OpenXR position and orientation quaternion (x, y, z, w) in the
// engine camera's axes.
void fromXr(const float xrPosition[3], const float xrOrientation[4], float position[3], float orientation[4]);

// The eye for a head pose (engine camera axes, metres, relative to where the
// head started): the game camera's yaw, then the head. With keepGamePitch
// the game's pitch comes last, so looking ahead shows the game's view, but
// the world then turns about a tilted axis as the head turns; without it the
// horizon stays level with the real one and the player looks down at the
// battle. The head's movement is level with the ground, unitsPerMetre world
// units per metre. worldUp is the world's vertical axis.
Basis eyeBasis(const Basis& game, const float worldUp[3], const float headOrientation[4],
               const float headPosition[3], float unitsPerMetre, bool keepGamePitch);

// Where a basis made by eyeBasis looks, in world space: minus its z (see
// above). For a controller's aim pose put through eyeBasis, the ray.
void viewDirection(const Basis& eye, float direction[3]);

// An eye's frustum edges at distance near, as Camera::setOrthogonal names
// them, from OpenXR's field of view (angleLeft, angleRight, angleUp,
// angleDown in radians; left and down negative). projectZ puts camera
// x = leftClip at the screen's right edge and y = topClip at its bottom.
void clipEdges(const float fov[4], float near, float& leftClip, float& rightClip,
               float& topClip, float& bottomClip);

}

#endif // VR_EYE_H
