// tests/unit/test_render_heuristics.cpp
//
// mclib/render_heuristics.h: LOD switch hysteresis (appearances), the
// point-to-box distance behind terrain light culling (Camera) and the turn
// rate the texture cache uses to turn times into frame counts (txmmgr).

#include "doctest.h"

#include "render_heuristics.h"

TEST_SUITE("RenderHeuristics")
{

TEST_CASE("LODSwitchDistance: harder to switch to a level, harder to leave it")
{
	// at LOD 0, level 1 needs 5% past its distance
	CHECK(LODSwitchDistance(1000.0f, 1, 0) == doctest::Approx(1050.0f));
	// at LOD 1 (or coarser), level 1 is kept until 5% inside its distance
	CHECK(LODSwitchDistance(1000.0f, 1, 1) == doctest::Approx(950.0f));
	CHECK(LODSwitchDistance(1000.0f, 1, 2) == doctest::Approx(950.0f));
	CHECK(LODSwitchDistance(2000.0f, 2, 1) == doctest::Approx(2100.0f));
}

TEST_CASE("LODSwitchDistance: no flip-flop inside the band")
{
	// an object at 1000 (exactly the threshold) keeps whichever level it has
	const float d = 1000.0f;
	CHECK_FALSE(d > LODSwitchDistance(1000.0f, 1, 0));	// at LOD 0: stays 0
	CHECK(d > LODSwitchDistance(1000.0f, 1, 1));			// at LOD 1: stays 1
}

TEST_CASE("PointBoxDistanceSquared")
{
	const float lo[3] = { 0.0f, 0.0f, 0.0f };
	const float hi[3] = { 10.0f, 20.0f, 30.0f };

	const float inside[3] = { 5.0f, 5.0f, 5.0f };
	CHECK(PointBoxDistanceSquared(inside, lo, hi) == 0.0f);

	const float onFace[3] = { 10.0f, 5.0f, 5.0f };
	CHECK(PointBoxDistanceSquared(onFace, lo, hi) == 0.0f);

	// outside on one axis
	const float pastX[3] = { 13.0f, 5.0f, 5.0f };
	CHECK(PointBoxDistanceSquared(pastX, lo, hi) == doctest::Approx(9.0f));
	const float belowZ[3] = { 5.0f, 5.0f, -4.0f };
	CHECK(PointBoxDistanceSquared(belowZ, lo, hi) == doctest::Approx(16.0f));

	// outside a corner: all three axes add up
	const float corner[3] = { -3.0f, 24.0f, 30.0f + 12.0f };
	CHECK(PointBoxDistanceSquared(corner, lo, hi) == doctest::Approx(9.0f + 16.0f + 144.0f));
}

TEST_CASE("TurnRate: assumes 30 turns/s until measured")
{
	TurnRate rate;
	CHECK(rate.turnsFor(2000, 60) == 60);
	CHECK(rate.turnsFor(5000, 150) == 150);
}

TEST_CASE("TurnRate: converges to the measured frame rate")
{
	TurnRate rate;
	uint32_t now = 100000;
	long turn = 0;
	rate.update(now, turn);
	// 180 turns per second, one frame every ~5.5 ms, for 3 seconds
	for (int frame = 1; frame <= 540; frame++)
	{
		turn++;
		now = 100000 + uint32_t(frame * 1000 / 180);
		rate.update(now, turn);
	}
	CHECK(rate.turnsPerSecond == doctest::Approx(180.0f).epsilon(0.03));
	CHECK(rate.turnsFor(2000, 60) == doctest::Approx(360).epsilon(0.03));
	// never fewer than the minimum
	CHECK(rate.turnsFor(100, 60) == 60);
}

TEST_CASE("TurnRate: a pause or a new mission does not count as a frame rate")
{
	TurnRate rate;
	rate.update(1000, 0);
	rate.update(1100, 10);		// 100 turns/s
	const float before = rate.turnsPerSecond;

	// paused for 10 s: no frames, then a few turns
	rate.update(11100, 12);
	CHECK(rate.turnsPerSecond == before);

	// new mission: turn restarts at 0
	rate.update(11200, 0);
	CHECK(rate.turnsPerSecond == before);
}

TEST_CASE("TurnRate: millisecond timer wrap-around")
{
	TurnRate rate;
	rate.update(0xFFFFFFF0u, 100);
	rate.update(0x00000050u, 116);	// 96 ms later across the wrap: 16 turns
	CHECK(rate.turnsPerSecond > 30.0f);
	CHECK(rate.turnsPerSecond < 200.0f);
}

}
