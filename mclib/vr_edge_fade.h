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

} // namespace vr_edge_fade

#endif
