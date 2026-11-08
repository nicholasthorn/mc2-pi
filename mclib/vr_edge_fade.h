// VR: the game draws a square of terrain round the point the camera looks at.
// The headset's wide view sees its edge, where the game's haze has barely begun
// (the haze was tuned for 40 degrees), so in VR the ground fades out before it
// ends. The fade is a circle round that point, not the square: it reads as a
// horizon, and does not move in steps as the square does. shaders/include/
// scene.hglsl has the same sums for meshes. GL-free so it can be unit tested.
#ifndef VR_EDGE_FADE_H
#define VR_EDGE_FADE_H

#include <cmath>

namespace vr_edge_fade {

struct Range {
    float start;        // no fade closer than this
    float invWidth;     // 1 / the distance from no fade to full; 0: no fade at all
};

// How many terrain vertices the fade takes to reach full.
const float kWidthVertices = 4.0f;

// The square runs from N/2 vertices before the vertex nearest the point to
// N/2 - 1 after it, and the point is within half a vertex of that one: no
// vertex on the square's edge is nearer than N/2 - 1.5, so all are fully faded.
inline Range rangeFor(long verticesPerSide, float unitsPerVertex)
{
    const float full = (verticesPerSide * 0.5f - 1.5f) * unitsPerVertex;
    if (full <= 0.0f)
        return Range{0.0f, 0.0f};
    const float wanted = kWidthVertices * unitsPerVertex;
    const float width = (wanted < full * 0.5f) ? wanted : full * 0.5f;
    return Range{full - width, 1.0f / width};
}

// 0 clear, 1 fully faded, smooth at both ends.
inline float amount(const Range& range, float dx, float dy)
{
    float t = (std::sqrt(dx * dx + dy * dy) - range.start) * range.invWidth;
    t = (t < 0.0f) ? 0.0f : ((t > 1.0f) ? 1.0f : t);
    return t * t * (3.0f - 2.0f * t);
}

// The round window. The square's corners lie wholly past the fade, so in VR
// they are not drawn at all. Quad (x, y) runs from vertex x to x + 1 (and y),
// counted from the square's top left; the vertex nearest the point is N / 2,
// and the point is within half a vertex of it on each axis, so a quad whose
// nearest point is further than the full fade plus that half diagonal from
// the vertex is fully faded everywhere.
inline bool quadInCircle(long verticesPerSide, long x, long y)
{
    const float full = verticesPerSide * 0.5f - 1.5f;
    if (full <= 0.0f)
        return true;
    const float centre = float(verticesPerSide >> 1);
    const float reach = full + 0.7072f;
    float gapX = float(x) - centre;
    if (centre - float(x + 1) > gapX) gapX = centre - float(x + 1);
    float gapY = float(y) - centre;
    if (centre - float(y + 1) > gapY) gapY = centre - float(y + 1);
    gapX = (gapX > 0.0f) ? gapX : 0.0f;
    gapY = (gapY > 0.0f) ? gapY : 0.0f;
    return gapX * gapX + gapY * gapY <= reach * reach;
}

// Whether ground there can be seen at all: dx, dy from the point, in world units.
inline bool pointInCircle(long verticesPerSide, float unitsPerVertex, float dx, float dy)
{
    const float full = (verticesPerSide * 0.5f - 1.5f) * unitsPerVertex;
    if (full <= 0.0f)
        return true;
    return dx * dx + dy * dy < full * full;
}

} // namespace vr_edge_fade

#endif
