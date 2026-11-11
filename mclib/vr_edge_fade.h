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
// The fade is full this many vertices from the point; 0 or less: no fade.
inline float fullVertices(long verticesPerSide)
{
    return verticesPerSide * 0.5f - 1.5f;
}

inline Range rangeFor(long verticesPerSide, float unitsPerVertex)
{
    const float full = fullVertices(verticesPerSide) * unitsPerVertex;
    if (full <= 0.0f)
        return Range{0.0f, 0.0f};
    const float wanted = kWidthVertices * unitsPerVertex;
    const float width = (wanted < full * 0.5f) ? wanted : full * 0.5f;
    return Range{full - width, 1.0f / width};
}

// 0 clear, 1 fully faded: smoothstep over the range, as in scene.hglsl.
inline float amount(const Range& range, float dx, float dy)
{
    if (range.invWidth == 0.0f)
        return 0.0f;
    const float d2 = dx * dx + dy * dy;
    if (d2 <= range.start * range.start)
        return 0.0f;
    float t = (std::sqrt(d2) - range.start) * range.invWidth;
    t = (t > 1.0f) ? 1.0f : t;
    return t * t * (3.0f - 2.0f * t);
}

// The round window. The square's corners lie wholly past the fade, so in VR
// they are not drawn at all. Quad (x, y) runs from vertex x to x + 1 (and y),
// counted from the square's top left; the vertex nearest the point is N / 2,
// and the point is within half a vertex of it on each axis (a half diagonal in
// all), so a quad further than the full fade plus that from the vertex is
// fully faded everywhere.
const float kHalfDiagonal = 0.7072f;

inline bool quadInCircle(long verticesPerSide, long x, long y)
{
    const float full = fullVertices(verticesPerSide);
    if (full <= 0.0f)
        return true;
    const long centre = verticesPerSide >> 1;
    // whole vertices from the centre vertex to the quad's nearest side
    const float gapX = float((x >= centre) ? x - centre : centre - 1 - x);
    const float gapY = float((y >= centre) ? y - centre : centre - 1 - y);
    const float reach = full + kHalfDiagonal;
    return gapX * gapX + gapY * gapY <= reach * reach;
}

// Vertex (x, y) is a corner of a quad the round window keeps.
inline bool vertexInCircle(long verticesPerSide, long x, long y)
{
    return quadInCircle(verticesPerSide, x, y) || quadInCircle(verticesPerSide, x - 1, y) ||
        quadInCircle(verticesPerSide, x, y - 1) || quadInCircle(verticesPerSide, x - 1, y - 1);
}

// Whether ground there can be seen at all: dx, dy from the point, in world units.
inline bool pointInCircle(long verticesPerSide, float unitsPerVertex, float dx, float dy)
{
    const float full = fullVertices(verticesPerSide) * unitsPerVertex;
    if (full <= 0.0f)
        return true;
    return dx * dx + dy * dy < full * full;
}

// The near edge. The game drops a ground triangle with any corner outside the
// depth range (0 to 1): the flat camera never sees ground that near. An eye in
// VR looking down does, and dropping the triangles that cross its near plane
// left a gap at the bottom of the view. In VR the corners keep a signed rhw
// (Camera::projectZSigned: negative behind the eye) and the GPU clips the
// triangle, so only one wholly behind the eye or wholly past the far plane is
// dropped. rhw, z: the three corners' as projected.
inline bool nearTriangleDrawn(const float rhw[3], const float z[3])
{
    bool behind = true, beyond = true;
    for (int i = 0; i < 3; ++i) {
        behind = behind && (rhw[i] < 0.0f);
        beyond = beyond && (rhw[i] > 0.0f) && (z[i] >= 1.0f);
    }
    return !behind && !beyond;
}

} // namespace vr_edge_fade

#endif
