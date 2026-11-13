// tests/unit/test_missaim.cpp
//
// Where a missed shot at a copter lands: it passes beside the copter at the
// aimed height and carries on along the line of fire to the ground.

#include "doctest.h"

#include "missaim.h"

#include <cmath>

namespace
{

float flatGround(float, float, void* context)
{
	return context ? *(const float*)context : 0.0f;
}

// ground rising 1 unit per unit of x past x = 100 (a hill behind the target)
float hillGround(float x, float, void*)
{
	return x > 100.0f ? x - 100.0f : 0.0f;
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

} // TEST_SUITE("MissAim")
