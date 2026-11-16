#pragma once

// The tabletop on a phone through ARCore (planning/future-features/
// AR-TABLETOP.md, phones). Built with MC2_ARCORE (the Android phone build);
// elsewhere these are no-ops.
//
// MC2_AR_TABLE=1 turns it on: the phone's camera image fills the screen, and
// the battle stands on a table in it, the Quest's tabletop seen through one
// eye, the phone. The table goes on the first level surface found under the
// middle of the screen (until then, ahead of and below the phone), facing
// the way the phone faced. MC2_TABLE_SCALE sets its scale, as on the Quest.
// The interface is drawn over the camera image as in a flat game.
//
// Each frame (beginFrame) ARCore's newest camera image and the phone's pose
// come in; the camera draws the world once, for the eye eyeViews gives,
// into the window (bindEye), after the camera image.

struct gosVREyeView;

namespace gos_ar {

#ifdef MC2_ARCORE
// After the GL context is made current: asks for the camera and starts
// ARCore. False when the tabletop is off or ARCore cannot start; the game
// then runs flat as before.
bool init();
// Before drawing a frame: the camera image and the phone's pose.
void beginFrame();
// The tabletop is running (gos_IsVR, gos_GetVRTabletop).
bool active();
// The eye to draw this frame: 1 while ARCore tracks the phone, else 0.
int eyeViews(gosVREyeView views[2]);
// The eye's image: the window.
void eyeImageSize(int eye, int& width, int& height);
// Eye 0: the window, cleared, with the camera image drawn. Others: false.
bool bindEye(int eye);
// The table's scale (gos_GetVRWorldScale).
float tableScale();

// Touching the battle: a finger on the screen is a ray from the phone through
// that point, as the Quest's controllers give one, which the game picks the
// world with (gos_GetVRPointerRay) and answers with where that is on the game
// camera's screen (gos_SetVRWorldPointer). The mouse goes there, so a tap
// selects and orders where the finger was; a finger on the interface (the
// game says so: no point) keeps the mouse under it.
//
// pointerFor, for each move of the touch screen's pointer (drawable pixels):
// where the mouse should go, or false when the game has not yet answered for
// this point (a press then waits a frame, as it must land where the finger
// was).
bool pointerFor(float x, float y, float& mouseX, float& mouseY);
// The finger, 0 to 1 across the game's screen (gos_GetVRTouchPoint).
bool touchPoint(float& x, float& y);
bool pointerRay(float position[3], float orientation[4]);
void setWorldPointer(bool valid, float x, float y);
// The app goes into the background and comes back: ARCore lets go of the
// camera and takes it again.
void pause();
void resume();
void shutdown();
#else
inline bool init() { return false; }
inline void beginFrame() {}
inline bool active() { return false; }
inline int eyeViews(gosVREyeView*) { return 0; }
inline void eyeImageSize(int, int& width, int& height) { width = height = 0; }
inline bool bindEye(int) { return false; }
inline float tableScale() { return 1.0f; }
inline bool pointerFor(float x, float y, float& mouseX, float& mouseY) { mouseX = x; mouseY = y; return true; }
inline bool touchPoint(float&, float&) { return false; }
inline bool pointerRay(float*, float*) { return false; }
inline void setWorldPointer(bool, float, float) {}
inline void pause() {}
inline void resume() {}
inline void shutdown() {}
#endif

} // namespace gos_ar
