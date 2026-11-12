// tests/unit/test_ablscan.cpp
//
// ABL integer literals: their digits' value, as its low 32 bits, the rule the
// FIT and CSV readers follow too (wholeNumberLow32). The stock mission
// scripts (mc2_01, mc2_08, mc2_17, mc2_18, multiplayer) call
// fadeToColor(4278190080, ...), past the int range: the 2001 build gave
// 0xFF000000 (opaque black), which these pin.

#include "doctest.h"

#include "ablscan.h"

#include <cstdint>

TEST_SUITE("AblScan")
{

TEST_CASE("an integer literal past the int range keeps its low 32 bits, as the 2001 build did")
{
	CHECK((uint32_t)ablIntegerLiteral("4278190080") == 0xFF000000u);
	CHECK((uint32_t)ablIntegerLiteral("4294967296") == 0u);
}

TEST_CASE("literals a float could not hold are exact")
{
	// 0xFF123456: the float route gave 0xFF123400
	CHECK((uint32_t)ablIntegerLiteral("4279383126") == 0xFF123456u);
	CHECK(ablIntegerLiteral("2147483647") == 2147483647);
	CHECK(ablIntegerLiteral("16777217") == 16777217);
}

TEST_CASE("ordinary literals are unchanged")
{
	CHECK(ablIntegerLiteral("0") == 0);
	CHECK(ablIntegerLiteral("12") == 12);
	CHECK(ablIntegerLiteral("350") == 350);
	CHECK(ablIntegerLiteral("16777216") == 16777216);
}

TEST_CASE("past 64 bits, still the low 32 bits of the value")
{
	// 2^64 + 5
	CHECK(ablIntegerLiteral("18446744073709551621") == 5);
}

} // TEST_SUITE("AblScan")
