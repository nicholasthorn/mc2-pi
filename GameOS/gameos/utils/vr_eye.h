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

// The game camera following the head (OpenXR orientation, as fromXr keeps
// it): it turns as the eyes do (eyeBasis) but stays where it is, as what it
// counts as in view does not need the eyes' few centimetres.
Basis followHead(const Basis& game, const float worldUp[3], const float headOrientation[4], bool keepGamePitch);

// Where a basis made by eyeBasis looks, in world space: minus its z (see
// above). For a controller's aim pose put through eyeBasis, the ray.
void viewDirection(const Basis& eye, float direction[3]);

// A camera's axes in the game's world (z up; the engine's world is -x, z, y)
// as Camera::cameraFrame holds them: i screen right, j where it looks, k
// screen up.
void worldFrame(const Basis& camera, float i[3], float j[3], float k[3]);

// Where a point on a camera's screen (x right, y down, -1 to 1) meets the ground
// at groundHeight, in the game's world: i, j, k as worldFrame, tanX and tanY the
// tangents of half its field of view. A point at or above the horizon, or
// further than maxDistance, gives the point maxDistance along where it heads
// (the minimap's wedge, which must not fly off when the head looks up).
void screenToGround(const float i[3], const float j[3], const float k[3], const float origin[3],
                    float tanX, float tanY, float x, float y, float groundHeight, float maxDistance,
                    float point[3]);

// An eye's frustum edges at distance near, as Camera::setOrthogonal names
// them, from OpenXR's field of view (angleLeft, angleRight, angleUp,
// angleDown in radians; left and down negative). projectZ puts camera
// x = leftClip at the screen's right edge and y = topClip at its bottom.
void clipEdges(const float fov[4], float near, float& leftClip, float& rightClip,
               float& topClip, float& bottomClip);

// The range of the game camera's field of view in VR, in degrees. The view
// cone's test breaks down above 127: cos(fov * 0.7071) goes negative and lets
// through what is behind the camera.
const float kMinGameFov = 20.0f;
const float kMaxGameFov = 120.0f;

// The field of view, in degrees, that the game camera needs to cover what the
// two eyes see (fov as in clipEdges). Camera::camera_fov is horizontal and
// symmetric, and its window is usually wider than the eyes are, so a tall view
// decides through heightOverWidth (0 when there is no window yet: the eyes'
// width alone). In the range above, and its minimum for a field of view that
// is no use.
float gameFieldOfView(const float leftFov[4], const float rightFov[4], float heightOverWidth);

// A field of view given as text (MC2_XR_FOV): its degrees when in the range
// above, 0 for "0" (keep the game's own), -1 for anything else.
float parseGameFov(const char* text);

// A fixed head rotation given as text (MC2_XR_POSE=yaw,pitch in degrees: yaw
// about +y, left, then pitch about +x, up, in OpenXR's axes) as a quaternion
// (x, y, z, w). False when the text is not two numbers.
bool parseHeadPose(const char* text, float orientation[4]);

}

#endif // VR_EYE_H
