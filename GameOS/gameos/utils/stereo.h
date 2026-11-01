#ifndef STEREO_H
#define STEREO_H

// Desktop side-by-side stereo, the debug view of the VR plan's Phase 1
// (planning/future-features/VR-TABLETOP.md). GL-free so it can be unit
// tested.
namespace stereo {

// GL viewport for an eye: -1 is the whole window (mono), 0 the left half,
// 1 the right half. out = x, y, width, height.
void eyeViewport(int eye, int width, int height, int out[4]);

// How far an eye's camera moves sideways from the game camera, positive to
// screen right: half the separation left (eye 0) or right (eye 1), none for
// mono (-1).
float eyeOffset(int eye, float separation);

}

#endif // STEREO_H
