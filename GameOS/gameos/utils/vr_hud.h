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

}  // namespace vr_hud

#endif
