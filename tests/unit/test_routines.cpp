// tests/unit/test_routines.cpp
//
// RollDice() / RandomNumber() in mclib/routines.cpp. Both assume gos_rand()
// returns 0..32767 (RAND_MAX on MSVC). On Linux rand() goes up to 2^31-1,
// which made RollDice's "(r*100)>>15" overflow and roll nonsense until alariq
// switched it to gos_rand() (1c601fc). These tests pin the mapping across the
// whole 15-bit range so that cannot regress.
//
// gos_rand() normally lives in GameOS/gameos/gameos.cpp, which drags the whole
// platform layer; this file defines a controllable one instead (the only
// definition in mc2_tests).

#include "doctest.h"

#include "gameos.hpp"
#include "mathfunc.h"

namespace
{
int g_nextRand = 0;
}

int __stdcall gos_rand()
{
	return g_nextRand;
}

TEST_SUITE("Routines")
{

TEST_CASE("RollDice at the bottom of the 15-bit range")
{
	g_nextRand = 0;					// maps to 0%
	CHECK(RollDice(1));
	CHECK_FALSE(RollDice(0));
}

TEST_CASE("RollDice at the top of the 15-bit range")
{
	g_nextRand = 32767;				// (32767*100)>>15 == 99
	CHECK(RollDice(100));
	CHECK_FALSE(RollDice(99));
}

TEST_CASE("RollDice(50) splits the range in half")
{
	g_nextRand = 16383;				// just below the midpoint: 49%
	CHECK(RollDice(50));
	g_nextRand = 16384;				// midpoint: 50%
	CHECK_FALSE(RollDice(50));
}

TEST_CASE("RollDice(p) succeeds for exactly p percent of the 15-bit range")
{
	for (long p = 0; p <= 100; p += 10)
	{
		long hits = 0;
		for (int r = 0; r < 32768; ++r)
		{
			g_nextRand = r;
			hits += RollDice(p) ? 1 : 0;
		}
		// each percent is 327 or 328 values of r
		CHECK(hits >= p * 327);
		CHECK(hits <= p * 328 + 1);
	}
}

TEST_CASE("RandomNumber(range) stays in [0, range)")
{
	const int samples[] = { 0, 1, 99, 16384, 32767 };
	for (int r : samples)
	{
		g_nextRand = r;
		const long v = RandomNumber(10);
		CHECK(v >= 0);
		CHECK(v < 10);
	}
}

} // TEST_SUITE("Routines")
