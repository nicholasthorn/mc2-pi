#ifndef STEREO_H
#define STEREO_H

// Desktop side-by-side stereo, the debug view of the VR plan's Phase 1
// (planning/future-features/VR-TABLETOP.md): which part of the window each
// eye draws into and how far its camera moves. GL-free so it can be unit
// tested.
namespace stereo {

// GL viewport for an eye: -1 is the whole window (mono), 0 the left half,
// 1 the right half. out = x, y, width, height.
void eyeViewport(int eye, int width, int height, int out[4]);

// World-plane (x, y) offset of an eye's camera from the game camera: half the
// separation to the left (eye 0) or right (eye 1) along the camera's right
// axis. The axis is given in the engine's camera space, where x is minus
// world x and z is world y; only its horizontal part is used. Mono (-1) and
// a vertical or zero axis give no offset.
void eyeShift(int eye, float rightX, float rightZ, float separation, float* dx, float* dy);

}

#endif // STEREO_H
