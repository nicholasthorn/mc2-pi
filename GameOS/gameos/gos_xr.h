#pragma once

// OpenXR presentation (VR plan, Phase 2). Built with MC2_OPENXR (the Meta
// Quest build); elsewhere these are no-ops and the window is swapped as
// before. Slice 1: the finished frame is shown on a quad layer, a screen
// fixed in the room in front of the player.

namespace gos_xr {

#ifdef MC2_OPENXR
// After the GL context is made current. False when XR is off (MC2_XR=0) or
// fails to start; the game then runs flat as before.
bool init();
// At the end of a frame: shows the default framebuffer (width x height) in
// the headset and returns true, or returns false when the caller should
// swap the window instead.
bool present(int width, int height);
// The runtime asked the app to quit (the session is exiting).
bool exitRequested();
void shutdown();
#else
inline bool init() { return false; }
inline bool present(int, int) { return false; }
inline bool exitRequested() { return false; }
inline void shutdown() {}
#endif

} // namespace gos_xr
