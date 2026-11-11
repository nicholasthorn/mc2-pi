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

// Overlays are in window coordinates, stretched over the eye's image, so a
// font pixel's size in the eye depends on the game's resolution. How much to
// scale text by, in x and y, for a font pixel to be kOverlayTextEyePixels eye
// pixels across at any resolution (judged in the Quest 2). 1 when a size is
// not known.
const float kOverlayTextEyePixels = 0.825f;
// Bars, icons and brackets, as a share of the text's scale in x: narrower, at
// 0.393 eye pixels to a game pixel (also judged in the Quest 2).
const float kOverlayMarkerShareX = 0.393f / kOverlayTextEyePixels;
void overlayTextScale(int windowWidth, int windowHeight, int eyeWidth, int eyeHeight, float& scaleX, float& scaleY);

// The interface's layout size in a headset (Environment.screenWidth/Height): the
// game screen the HUD is laid out on, which sets how large its text is on the
// panel. Not the pixels it is drawn into (the window's frame, drawableWidth/
// Height, the headset's full 3664x1920 since gosRenderer::handleEvents stopped
// resizing it), which set how sharp it is, nor the panel's size in metres
// (MC2_XR_HUD), which sets how large everything on it looks. See "The
// interface's three sizes" in planning/future-features/VR-TABLETOP.md.
// 1280x960 was judged right in the Quest 2 and keeps 800x600's shape, so the
// game camera does not change (that judgement was made while the frame was
// still an 800x600 corner, so "800x600 is coarse" was partly the frame).
// It replaces 800x600 (also what a file with no size reads as) and anything
// wider than the largest layout (1920), which has none: the panel's contents
// would fill a corner of it, and the options screen can save the headset's own
// 3664 there. Other sizes stand. True when it changed the size.
const int kInterfaceWidth = 1280;
const int kInterfaceHeight = 960;
bool fitInterfaceResolution(int& width, int& height);

// The interface panel's width in a headset, for a screen laid out layoutWidth
// pixels wide. interfaceWidth (MC2_XR_HUD, A + stick) is the width of the
// mission HUD's kInterfaceWidth layout; every other screen gets the same size
// per layout pixel. The menus' 800x600 layouts (stock data) are bitmaps made
// for that size: on a panel as wide as the HUD's each of their pixels was 1.6
// times as large as a HUD pixel, coarse and with larger text. Now a menu is on
// a smaller panel, as sharp as the HUD; Omnitech's 1280x1024 front end is as
// wide as the HUD. 0 or less (no size yet): interfaceWidth.
float panelWidth(float interfaceWidth, int layoutWidth);

// A fixed head rotation given as text (MC2_XR_POSE=yaw,pitch in degrees: yaw
// about +y, left, then pitch about +x, up, in OpenXR's axes) as a quaternion
// (x, y, z, w). False when the text is not two numbers.
bool parseHeadPose(const char* text, float orientation[4]);

}

#endif // VR_EYE_H
