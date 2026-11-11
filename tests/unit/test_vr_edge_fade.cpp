#include "doctest.h"

#include "vr_edge_fade.h"

#include <cmath>
#include <initializer_list>

using namespace vr_edge_fade;

TEST_SUITE("VrEdgeFade") {

const float kUnits = 128.0f;

TEST_CASE("clear near the point, full by the nearest edge vertex") {
    for (long side : {30L, 45L, 60L}) {
        const Range range = rangeFor(side, kUnits);
        CHECK(amount(range, 0.0f, 0.0f) == 0.0f);
        CHECK(amount(range, range.start, 0.0f) == 0.0f);

        // the point half a vertex towards the near side of the square
        const float nearestEdge = (side * 0.5f - 1.5f) * kUnits;
        CHECK(amount(range, nearestEdge, 0.0f) == 1.0f);
        CHECK(amount(range, 0.0f, -nearestEdge) == 1.0f);
        // a corner, and beyond it, stay full
        CHECK(amount(range, nearestEdge, nearestEdge) == 1.0f);
        CHECK(amount(range, 3.0f * nearestEdge, 0.0f) == 1.0f);
    }
}

TEST_CASE("rises steadily over its width, the same every way round") {
    const Range range = rangeFor(30, kUnits);
    const float width = 1.0f / range.invWidth;
    CHECK(width == doctest::Approx(4.0f * kUnits));
    CHECK(amount(range, range.start + width * 0.5f, 0.0f) == doctest::Approx(0.5f));

    float last = 0.0f;
    for (int i = 1; i <= 8; ++i) {
        const float value = amount(range, range.start + width * i / 8.0f, 0.0f);
        CHECK(value > last);
        last = value;
    }

    const float d = range.start + width * 0.25f;
    CHECK(amount(range, -d, 0.0f) == doctest::Approx(amount(range, d, 0.0f)));
    CHECK(amount(range, d * 0.6f, d * 0.8f) == doctest::Approx(amount(range, d, 0.0f)));
}

TEST_CASE("a small square keeps half of itself clear") {
    const Range range = rangeFor(10, kUnits);
    const float full = 3.5f * kUnits;
    CHECK(range.start == doctest::Approx(full * 0.5f));
    CHECK(amount(range, full, 0.0f) == 1.0f);
}

TEST_CASE("no square, no fade") {
    for (long side : {0L, 1L, 3L}) {
        const Range range = rangeFor(side, kUnits);
        CHECK(range.invWidth == 0.0f);
        CHECK(amount(range, 100000.0f, 0.0f) == 0.0f);
    }
}

// The round window: a quad runs from vertex x to x + 1 (and y), counted from the
// window's top left; the vertex nearest the point is N / 2, and the point is
// within half a vertex of it on each axis.
static float nearestDistance(long x, long y, float px, float py) {
    const float dx = (px < x) ? x - px : ((px > x + 1) ? px - (x + 1) : 0.0f);
    const float dy = (py < y) ? y - py : ((py > y + 1) ? py - (y + 1) : 0.0f);
    return std::sqrt(dx * dx + dy * dy);
}

TEST_CASE("round window: the middle stays, the corners go") {
    for (long side : {30L, 45L, 60L, 76L, 90L}) {
        const long c = side >> 1;
        const long last = side - 2;     // the last quad in a row
        CHECK(quadInCircle(side, c, c));
        CHECK(quadInCircle(side, c - 1, c - 1));
        CHECK(quadInCircle(side, 0, c));        // the middle of each side
        CHECK(quadInCircle(side, last, c));
        CHECK(quadInCircle(side, c, 0));
        CHECK(quadInCircle(side, c, last));
        CHECK_FALSE(quadInCircle(side, 0, 0));
        CHECK_FALSE(quadInCircle(side, last, 0));
        CHECK_FALSE(quadInCircle(side, 0, last));
        CHECK_FALSE(quadInCircle(side, last, last));
    }
}

TEST_CASE("round window: no quad dropped that the fade leaves visible") {
    for (long side : {10L, 30L, 31L, 45L, 60L, 76L, 90L}) {
        const float full = side * 0.5f - 1.5f;      // in vertices, as rangeFor
        const float c = float(side >> 1);
        for (float ox = -0.5f; ox <= 0.5f; ox += 0.125f)
            for (float oy = -0.5f; oy <= 0.5f; oy += 0.125f)
                for (long y = 0; y < side - 1; ++y)
                    for (long x = 0; x < side - 1; ++x)
                        if (nearestDistance(x, y, c + ox, c + oy) < full)
                            REQUIRE(quadInCircle(side, x, y));
    }
}

TEST_CASE("round window: drops a real share of the square") {
    for (long side : {30L, 60L, 90L}) {
        long kept = 0, all = 0;
        for (long y = 0; y < side - 1; ++y)
            for (long x = 0; x < side - 1; ++x) {
                ++all;
                kept += quadInCircle(side, x, y) ? 1 : 0;
            }
        const float dropped = 1.0f - float(kept) / float(all);
        CHECK(dropped > 0.10f);
        CHECK(dropped < 0.215f);    // a circle in the square drops 21.5%
    }
}

TEST_CASE("round window: ground picking stops where the fade is full") {
    const long side = 60;
    const float full = (side * 0.5f - 1.5f) * kUnits;
    CHECK(pointInCircle(side, kUnits, 0.0f, 0.0f));
    CHECK(pointInCircle(side, kUnits, full * 0.99f, 0.0f));
    CHECK_FALSE(pointInCircle(side, kUnits, full * 0.75f, full * 0.75f));
    CHECK_FALSE(pointInCircle(side, kUnits, 0.0f, -full * 1.01f));
    // too small a window for a fade: all of it counts
    CHECK(pointInCircle(3, kUnits, 1000.0f, 1000.0f));
    CHECK(quadInCircle(3, 0, 0));
}

TEST_CASE("round window: every corner of a kept quad is kept, the far corners are not") {
    for (long side : {10L, 30L, 45L, 60L, 90L}) {
        for (long y = 0; y < side - 1; ++y)
            for (long x = 0; x < side - 1; ++x)
                if (quadInCircle(side, x, y)) {
                    REQUIRE(vertexInCircle(side, x, y));
                    REQUIRE(vertexInCircle(side, x + 1, y));
                    REQUIRE(vertexInCircle(side, x, y + 1));
                    REQUIRE(vertexInCircle(side, x + 1, y + 1));
                }
        CHECK(vertexInCircle(side, side >> 1, side >> 1));
        CHECK_FALSE(vertexInCircle(side, 0, 0));
        CHECK_FALSE(vertexInCircle(side, side - 1, side - 1));
    }
}

TEST_CASE("the fade is 0 inside its start without a square root, 1 past its end") {
    const Range range = rangeFor(60, kUnits);
    CHECK(amount(range, range.start * 0.5f, range.start * 0.5f) == 0.0f);
    CHECK(amount(range, 0.0f, range.start) == 0.0f);
    CHECK(amount(range, 0.0f, range.start + 2.0f / range.invWidth) == 1.0f);
}

// The near edge: what the game used to drop, and what VR now leaves to the GPU.
TEST_CASE("near edge: a triangle in front is drawn, as the flat game draws it") {
    const float rhw[3] = {0.01f, 0.02f, 0.015f};
    const float z[3] = {0.5f, 0.9f, 0.1f};
    CHECK(nearTriangleDrawn(rhw, z));
}

TEST_CASE("near edge: one that crosses the eye's plane goes to the GPU to clip") {
    // the flat test dropped these: a corner behind the eye, or before the near plane
    const float rhw1[3] = {0.01f, -0.02f, 0.015f};
    const float z1[3] = {0.5f, 1.7f, 0.4f};
    CHECK(nearTriangleDrawn(rhw1, z1));
    const float rhw2[3] = {-0.01f, -0.02f, 0.015f};
    const float z2[3] = {-3.0f, 2.0f, 0.2f};
    CHECK(nearTriangleDrawn(rhw2, z2));
    const float rhw3[3] = {0.5f, 0.4f, 0.3f};
    const float z3[3] = {-0.1f, 0.2f, 0.3f};
    CHECK(nearTriangleDrawn(rhw3, z3));
}

TEST_CASE("near edge: wholly behind the eye, or wholly past the far plane, is dropped") {
    const float behind[3] = {-0.01f, -0.02f, -0.015f};
    const float anyZ[3] = {0.5f, 0.5f, 0.5f};
    CHECK_FALSE(nearTriangleDrawn(behind, anyZ));
    const float front[3] = {0.001f, 0.002f, 0.001f};
    const float pastFar[3] = {1.0f, 1.2f, 1.01f};
    CHECK_FALSE(nearTriangleDrawn(front, pastFar));
    // past the far plane but for one corner: the GPU clips it
    const float partFar[3] = {1.0f, 0.99f, 1.01f};
    CHECK(nearTriangleDrawn(front, partFar));
}

}
