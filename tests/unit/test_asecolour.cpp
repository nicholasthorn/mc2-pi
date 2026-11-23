#include "doctest.h"

#include "asecolour.h"

#include <cmath>
#include <limits>

TEST_SUITE("AseColour") {

TEST_CASE("in-range colours scale to a byte, truncating") {
    CHECK(aseColourToChannel(0.0f) == 0u);
    CHECK(aseColourToChannel(0.5f) == 127u);
    CHECK(aseColourToChannel(1.0f) == 255u);
}

// Values from stock bridge5dam.ase, whose vertex colours hold numbers far out
// of range. An x86 build wraps them through a 64-bit signed conversion; every
// .tgl shipped so far was built that way, so every machine must match it.
TEST_CASE("out-of-range colours give what an x86 build gives") {
    CHECK(aseColourToChannel(-22.9491f) == 0xFFFFE924u);  // -5852 as 32 bits
    CHECK(aseColourToChannel(45.5699f) == 11620u);        // spills past the byte
    CHECK(aseColourToChannel(-0.2382f) == 0xFFFFFFC4u);   // -60
}

TEST_CASE("NaN and huge values give 0, as x86's invalid conversion truncates to") {
    CHECK(aseColourToChannel(std::numeric_limits<float>::quiet_NaN()) == 0u);
    CHECK(aseColourToChannel(1e30f) == 0u);
    CHECK(aseColourToChannel(-1e30f) == 0u);
    CHECK(aseColourToChannel(std::numeric_limits<float>::infinity()) == 0u);
}

}
