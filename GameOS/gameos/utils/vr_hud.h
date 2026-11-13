#ifndef VR_HUD_H
#define VR_HUD_H

// When the VR interface can keep showing its last image instead of redrawing.
// GL-free so it can be unit tested; gos_xr.cpp keeps the state.
namespace vr_hud {

struct Frame {
    bool missionAskedLastFrame;     // a mission's interface draws, not a menu's
    bool eyesInUse;                 // the world is in the eyes, not in that image
    bool everyFrame;                // MC2_XR_HUD_EVERY_FRAME=1
    bool hasImage;                  // an image has reached the headset
    bool changed;                   // a game step since the interface last drew
    bool gameStepped;               // this frame has a game step
    int framesSinceDrawn;
};

// Rest while nothing has changed. A game step's own frame rests too, so that the
// step has the time, and the interface draws on the frame after it; but not
// through a run of steps, or it would fall behind.
inline bool rests(const Frame& f)
{
    if(!f.missionAskedLastFrame || !f.eyesInUse || f.everyFrame || !f.hasImage)
        return false;
    return !f.changed || (f.gameStepped && f.framesSinceDrawn < 2);
}

// Mip levels for the interface panel's image: down to 1 pixel on its longer
// side. The panel shows the headset's whole 3664x1920 frame on a quad about
// 1300 eye pixels across (800 for a stock menu's), and the compositor samples
// one level only: without smaller levels it skips texels, and fine text and
// edges sparkle as the head moves.
inline int mipLevels(int width, int height)
{
    int longest = width > height ? width : height;
    int levels = 1;
    while(longest > 1) {
        longest /= 2;
        ++levels;
    }
    return levels;
}

}  // namespace vr_hud

#endif
