// tests/unit/test_missaim.cpp
//
// Where a missed shot at a copter lands: it passes beside the copter at the
// aimed height and carries on along the line of fire to the ground.

#include "doctest.h"

#include "missaim.h"

#include <cmath>

namespace
{

bool flatGround(float, float, void* context, float* height)
{
	*height = context ? *(const float*)context : 0.0f;
	return true;
}

// ground rising 1 unit per unit of x past x = 100 (a hill behind the target)
bool hillGround(float x, float, void*, float* height)
{
	*height = x > 100.0f ? x - 100.0f : 0.0f;
	return true;
}

// flat ground on a map ending at x = 250
bool edgeGround(float x, float, void*, float* height)
{
	*height = 0.0f;
	return x <= 250.0f;
}

MissVec vec(float x, float y, float z)
{
	MissVec v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

} // namespace

TEST_SUITE("MissAim") {

TEST_CASE("missFlyPast: a shot fired downward lands behind the target")
{
	// shooter 200 up at x = 0, aimed beside a copter 100 up at x = 100: the
	// line drops 1 per unit of x and meets flat ground at x = 200
	MissVec p = missFlyPast(vec(0, 0, 200), vec(100, 0, 100), flatGround, NULL, 1000.0f, 8.0f);
	CHECK(p.x == doctest::Approx(200.0f).epsilon(0.01));
	CHECK(p.y == doctest::Approx(0.0f));
	CHECK(p.z == doctest::Approx(0.0f));
}

TEST_CASE("missFlyPast: a hill behind the target stops the shot sooner")
{
	// same line; the ground rises from x = 100, so they meet at x = 150
	MissVec p = missFlyPast(vec(0, 0, 200), vec(100, 0, 100), hillGround, NULL, 1000.0f, 8.0f);
	CHECK(p.x == doctest::Approx(150.0f).epsilon(0.01));
	CHECK(p.z == doctest::Approx(50.0f).epsilon(0.01));
}

TEST_CASE("missFlyPast: a shot fired upward ends maxPast beyond the target, in the air")
{
	// shooter on the ground, copter above: the line never comes down
	MissVec p = missFlyPast(vec(0, 0, 20), vec(300, 400, 120), flatGround, NULL, 500.0f, 8.0f);
	const float len = std::sqrt(300.0f * 300.0f + 400.0f * 400.0f + 100.0f * 100.0f);
	CHECK(p.x == doctest::Approx(300.0f + 300.0f / len * 500.0f).epsilon(0.001));
	CHECK(p.y == doctest::Approx(400.0f + 400.0f / len * 500.0f).epsilon(0.001));
	CHECK(p.z == doctest::Approx(120.0f + 100.0f / len * 500.0f).epsilon(0.001));
}

TEST_CASE("missFlyPast: a point beside the target that is already underground lands there")
{
	float ground = 150.0f;
	MissVec p = missFlyPast(vec(0, 0, 200), vec(100, 0, 100), flatGround, &ground, 1000.0f, 8.0f);
	CHECK(p.x == doctest::Approx(100.0f));
	CHECK(p.z == doctest::Approx(150.0f));
}

TEST_CASE("missFlyPast: no direction or no step leaves the shot beside the target, on the ground")
{
	MissVec same = missFlyPast(vec(100, 0, 100), vec(100, 0, 100), flatGround, NULL, 1000.0f, 8.0f);
	CHECK(same.x == doctest::Approx(100.0f));
	CHECK(same.z == doctest::Approx(0.0f));
	MissVec noStep = missFlyPast(vec(0, 0, 200), vec(100, 0, 100), flatGround, NULL, 1000.0f, 0.0f);
	CHECK(noStep.x == doctest::Approx(100.0f));
	CHECK(noStep.z == doctest::Approx(0.0f));
}

TEST_CASE("missFlyPast: a shot that leaves the map ends in the air at the edge")
{
	// fired upward toward x = 250, the map edge: it stops there, still up
	MissVec p = missFlyPast(vec(0, 0, 20), vec(200, 0, 120), edgeGround, NULL, 200.0f, 8.0f);
	CHECK(p.x == doctest::Approx(250.0f).epsilon(0.01));
	CHECK(p.x <= 250.0f);
	CHECK(p.z == doctest::Approx(145.0f).epsilon(0.01));
	// a point beside the target off the map comes back unchanged
	MissVec off = missFlyPast(vec(0, 0, 20), vec(300, 0, 120), edgeGround, NULL, 200.0f, 8.0f);
	CHECK(off.x == doctest::Approx(300.0f));
	CHECK(off.z == doctest::Approx(120.0f));
}

TEST_CASE("missSurface: water counts only where the cell has it and it is higher")
{
	CHECK(missSurface(10.0f, true, 30.0f) == doctest::Approx(30.0f));		// lake: the surface
	CHECK(missSurface(50.0f, true, 30.0f) == doctest::Approx(50.0f));		// bank above the level
	CHECK(missSurface(10.0f, false, 30.0f) == doctest::Approx(10.0f));		// dry valley below it
}

TEST_CASE("missEndsInAir: more than 10 units above the surface")
{
	CHECK(missEndsInAir(120.0f, 0.0f));
	CHECK_FALSE(missEndsInAir(10.0f, 0.0f));
	CHECK_FALSE(missEndsInAir(5.0f, 30.0f));
}

TEST_CASE("missSides: left and right of the line of fire first, then beyond and in front")
{
	MissVec out[4];
	// fired along +x at a target at (100, 0, 200)
	missSides(vec(0, 0, 0), vec(100, 0, 200), 25.0f, true, out);
	CHECK(out[0].x == doctest::Approx(100.0f));		// right of +x is -y
	CHECK(out[0].y == doctest::Approx(-25.0f));
	CHECK(out[1].y == doctest::Approx(25.0f));
	CHECK(out[2].x == doctest::Approx(125.0f));		// beyond
	CHECK(out[3].x == doctest::Approx(75.0f));		// in front
	for (int i = 0; i < 4; i++)
		CHECK(out[i].z == doctest::Approx(200.0f));

	missSides(vec(0, 0, 0), vec(100, 0, 200), 25.0f, false, out);
	CHECK(out[0].y == doctest::Approx(25.0f));		// left first
	CHECK(out[1].y == doctest::Approx(-25.0f));

	// fired along +y: right is +x, whichever way the map axes run
	missSides(vec(0, 0, 0), vec(0, 100, 0), 10.0f, true, out);
	CHECK(out[0].x == doctest::Approx(10.0f));
	CHECK(out[0].y == doctest::Approx(100.0f));
}

TEST_CASE("missSides: straight down, the 2001 order")
{
	MissVec out[4];
	missSides(vec(50, 50, 300), vec(50, 50, 0), 5.0f, true, out);
	CHECK(out[0].x == doctest::Approx(55.0f));
	CHECK(out[1].x == doctest::Approx(45.0f));
	CHECK(out[2].y == doctest::Approx(55.0f));
	CHECK(out[3].y == doctest::Approx(45.0f));
}

} // TEST_SUITE("MissAim")
