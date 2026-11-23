#ifndef AR_VIEW_H
#define AR_VIEW_H

// A phone's view through ARCore (planning/future-features/AR-TABLETOP.md,
// phones), turned into what the game draws a VR eye with and what the Quest's
// controllers give it: GL-free so it can be unit tested. Poses are in the
// tracking system's axes, which ARCore and OpenXR share: x right, y up, z
// back; a camera looks down its -z.
namespace ar_view {

// The four angles of a GL projection's view (column-major, as ARCore's
// ArCamera_getProjectionMatrix gives it), in radians, in OpenXR's order:
// left, right, up, down; left and down are negative.
void fovFromProjection(const float projection[16], float fov[4]);

// A ray from a camera with this orientation (x, y, z, w) and view (fov, as
// above) through the point u, v of its image (0 to 1, from the top left): the
// ray's orientation, which looks down its -z like the camera's.
void rayThroughPixel(const float orientation[4], const float fov[4], float u, float v,
                     float rayOrientation[4]);

}

#endif // AR_VIEW_H
