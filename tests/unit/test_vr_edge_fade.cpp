#include "doctest.h"

#include "vr_edge_fade.h"

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

}
