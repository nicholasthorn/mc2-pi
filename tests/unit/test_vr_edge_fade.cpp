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

// windowCentre: the window slides towards a camera far behind the point it
// looks at. FogPos 75 gives 52 vertices; the default camera is 1200 units out
// at 35 degrees, about 983 units back on the ground.
TEST_CASE("window centre: the point the camera looks at, at the default zoom") {
    Point c = windowCentre(52, kUnits, Point{5000.0f, 7000.0f}, Point{5000.0f, 7000.0f - 983.0f});
    CHECK(c.x == 5000.0f);
    CHECK(c.y == 7000.0f);
    c = windowCentre(52, kUnits, Point{5000.0f, 7000.0f}, Point{5000.0f, 7000.0f});    // straight down
    CHECK(c.x == 5000.0f);
    CHECK(c.y == 7000.0f);
}

TEST_CASE("window centre: a far camera stands in the clear ground, on the line to the point") {
    for (long side : {30L, 45L, 52L, 60L}) {
        const Range range = rangeFor(side, kUnits);
        for (float back : {1500.0f, 2500.0f, 6000.0f, 20000.0f}) {
            // the camera back along a diagonal, as on a fly-in
            const float vx = 1000.0f - back * 0.6f, vy = 2000.0f + back * 0.8f;
            const Point c = windowCentre(side, kUnits, Point{1000.0f, 2000.0f}, Point{vx, vy});
            const float cx = c.x, cy = c.y;
            const float fromViewer = std::sqrt((cx - vx) * (cx - vx) + (cy - vy) * (cy - vy));
            CHECK(fromViewer <= range.start * 0.5f + 0.5f);
            // on the line from the camera to the point, not past the point
            CHECK(std::fabs((cx - vx) * 0.8f + (cy - vy) * 0.6f) < 0.5f);
            CHECK((1000.0f - cx) * -0.6f + (2000.0f - cy) * 0.8f <= 0.5f);
            // all ground within half the clear radius of the camera is clear and kept
            for (int a = 0; a < 16; ++a) {
                const float angle = a * 0.3927f;
                const float gx = vx + std::cos(angle) * range.start * 0.5f - cx;
                const float gy = vy + std::sin(angle) * range.start * 0.5f - cy;
                CHECK(amount(range, gx, gy) <= 0.01f);
                CHECK(pointInCircle(side, kUnits, gx, gy));
            }
        }
    }
}

TEST_CASE("window centre: moves only as far as it must") {
    const Range range = rangeFor(52, kUnits);
    const float reach = range.start * 0.5f;
    const Point c = windowCentre(52, kUnits, Point{0.0f, 0.0f}, Point{reach + 300.0f, 0.0f});
    CHECK(c.x == doctest::Approx(300.0f));
    CHECK(c.y == 0.0f);
}

TEST_CASE("window centre: no fade, no move") {
    const Point c = windowCentre(3, kUnits, Point{10.0f, 20.0f}, Point{90000.0f, 20.0f});
    CHECK(c.x == 10.0f);
    CHECK(c.y == 20.0f);
}

TEST_CASE("the fade is 0 inside its start without a square root, 1 past its end") {
    const Range range = rangeFor(60, kUnits);
    CHECK(amount(range, range.start * 0.5f, range.start * 0.5f) == 0.0f);
    CHECK(amount(range, 0.0f, range.start) == 0.0f);
    CHECK(amount(range, 0.0f, range.start + 2.0f / range.invWidth) == 1.0f);
}

}
