#pragma once

// OpenXR presentation (VR plan, Phase 2). Built with MC2_OPENXR (the Meta
// Quest build); elsewhere these are no-ops and the window is swapped as
// before.
//
// Each frame: beginFrame() before drawing (waits for the headset and finds
// where the eyes will be), the camera draws each eye into its own image
// (bindEye), and present() sends the eyes as a projection layer plus the
// window's frame, which then holds only the interface, as a see-through quad
// below the horizon. With MC2_XR_EYES=0 the whole game frame is shown on a
// big screen ahead instead.
// Debugging: MC2_XR_POSE=yaw,pitch fixes the head rotation, MC2_XR_SHOT=n
// saves the left eye and the window frame as TGAs on the n-th eye frame,
// MC2_XR_MSAA sets the eyes' samples (default 4).
//
// Controllers (VR plan, Phase 3): the ray from the hand that last pulled
// its trigger or grip is the mouse where it meets the quad; the trigger is
// the left button, the grip the right. Left stick: pan (arrow keys); right
// stick: turn (shift + left / right) and zoom (keypad + / -). B holds Ctrl,
// X Shift, Y Alt; the left menu button is Escape. A puts the quad in front
// of the head; holding A, the left stick raises it (up) or lowers it (down)
// and makes it larger (right) or smaller (left), and letting go logs the
// MC2_XR_HUD value to keep. MC2_XR_HUD=distance,
// degrees,width sets where it starts (default 1.5 m, 30 degrees below the
// horizon, 2 m wide; with MC2_XR_EYES=0, 1.5 m ahead and 2 m wide).

struct gosVREyeView;

namespace gos_xr {

#ifdef MC2_OPENXR
// After the GL context is made current. False when XR is off (MC2_XR=0) or
// fails to start; the game then runs flat as before.
bool init();
// Before drawing a frame: waits for the headset's next frame and locates
// the eyes for it.
void beginFrame();
// The eyes to draw this frame (see gos_GetVREyeViews), 0 or 2.
int eyeViews(gosVREyeView views[2]);
// Makes eye 0 or 1's image the render target, cleared, with its viewport;
// -1 goes back to the window. False when not in VR (the caller then sets
// the window's viewport itself).
bool bindEye(int eye);
// At the end of a frame: shows the frame in the headset and returns true,
// or returns false when the caller should swap the window instead.
bool present(int width, int height);
// The runtime asked the app to quit (the session is exiting).
bool exitRequested();
// Seconds between game updates in VR (MC2_XR_LOGIC_HZ, default 30), or 0
// when the game should step every frame (not in VR, or MC2_XR_LOGIC_HZ=0).
double logicStep();
// An OpenXR session exists (gos_IsVR).
bool active();
// Every frame, before the input is sampled: reads the controllers into
// GameOS's virtual mouse and keys.
void pollInput();
// The controllers stand in for the mouse, with their own pointer drawn as
// a beam and a dot: the game draws no cursor.
bool pointerActive();
// World picking (gos_GetVRPointerRay, gos_SetVRWorldPointer): the pointing
// hand's aim pose when the pointer is on the world, and the game's answer.
bool pointerRay(float position[3], float orientation[4]);
void setWorldPointer(bool valid, float x, float y, float metres);
void shutdown();
#else
inline bool init() { return false; }
inline void beginFrame() {}
inline int eyeViews(gosVREyeView*) { return 0; }
inline bool bindEye(int) { return false; }
inline bool present(int, int) { return false; }
inline bool exitRequested() { return false; }
inline double logicStep() { return 0.0; }
inline bool active() { return false; }
inline void pollInput() {}
inline bool pointerActive() { return false; }
inline bool pointerRay(float*, float*) { return false; }
inline void setWorldPointer(bool, float, float, float) {}
inline void shutdown() {}
#endif

} // namespace gos_xr
