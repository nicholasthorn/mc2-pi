// tests/unit/test_ablscan.cpp
//
// ABL integer literals are scanned as a float, then made an int. The stock
// mission scripts (mc2_01, mc2_08, mc2_17, mc2_18, multiplayer) call
// fadeToColor(4278190080, ...), past the int range: the 2001 build's _ftol
// gave 0xFF000000 (opaque black), which these pin.

#include "doctest.h"

#include "ablscan.h"

#include <cstdint>

TEST_SUITE("AblScan")
{

TEST_CASE("an integer literal past the int range keeps its low 32 bits, as the 2001 build did")
{
	CHECK((uint32_t)ablIntegerLiteral(4278190080.0f) == 0xFF000000u);
	// 2147483647 is 2147483648 as a float, which wraps to INT_MIN
	CHECK((uint32_t)ablIntegerLiteral(2147483647.0f) == 0x80000000u);
	CHECK((uint32_t)ablIntegerLiteral(4294967296.0f) == 0u);
}

TEST_CASE("ordinary literals are unchanged")
{
	CHECK(ablIntegerLiteral(0.0f) == 0);
	CHECK(ablIntegerLiteral(12.0f) == 12);
	CHECK(ablIntegerLiteral(350.0f) == 350);
	CHECK(ablIntegerLiteral(-1.0f) == -1);
	CHECK(ablIntegerLiteral(16777216.0f) == 16777216);
}

TEST_CASE("past the 64-bit range, and not a number, give 0 (the old conversion's low bits)")
{
	CHECK(ablIntegerLiteral(1e20f) == 0);
	CHECK(ablIntegerLiteral(-1e20f) == 0);
	CHECK(ablIntegerLiteral(1e38f) == 0);
}

} // TEST_SUITE("AblScan")
