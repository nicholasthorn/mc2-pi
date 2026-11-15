// tests/unit/test_mc2xhud.cpp
//
// wolfman-MC2X's exchange and spy map buttons (code/mc2xhud.h).

#include "doctest.h"

#include "mc2xhud.h"

TEST_SUITE("MC2XHud")
{

TEST_CASE("C-Bills to resource points: 10000 at a time, when there are 10000")
{
	CHECK(mc2xhud::canUse(mc2xhud::kToResourcePoints, 10000, 0, false));
	CHECK_FALSE(mc2xhud::canUse(mc2xhud::kToResourcePoints, 9999, 500000, false));
	CHECK(mc2xhud::exchange(mc2xhud::kToResourcePoints, 25000, 7) == 10000);
	CHECK(mc2xhud::exchange(mc2xhud::kToResourcePoints, 9999, 7) == 0);
}

TEST_CASE("resource points to C-Bills: all of them, when there are any")
{
	CHECK(mc2xhud::canUse(mc2xhud::kToCBills, 0, 1, false));
	CHECK_FALSE(mc2xhud::canUse(mc2xhud::kToCBills, 5000000, 0, false));
	CHECK(mc2xhud::exchange(mc2xhud::kToCBills, 0, 85000) == -85000);
	CHECK(mc2xhud::exchange(mc2xhud::kToCBills, 0, 0) == 0);
}

TEST_CASE("spy map: a million C-Bills, not while one is running")
{
	CHECK(mc2xhud::canUse(mc2xhud::kSpyMap, 1000000, 0, false));
	CHECK_FALSE(mc2xhud::canUse(mc2xhud::kSpyMap, 999999, 2000000, false));
	CHECK_FALSE(mc2xhud::canUse(mc2xhud::kSpyMap, 3000000, 0, true));
	CHECK(mc2xhud::exchange(mc2xhud::kSpyMap, 3000000, 0) == 0);
}

TEST_CASE("other buttons do nothing")
{
	CHECK_FALSE(mc2xhud::canUse(21, 9000000, 9000000, false));	// MC2X's logo button
	CHECK(mc2xhud::exchange(24, 9000000, 9000000) == 0);
}

}
