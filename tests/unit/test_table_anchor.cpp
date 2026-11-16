// tests/unit/test_table_anchor.cpp
//
// table_anchor: the tabletop mode's table (AR-TABLETOP.md), shared by the
// Quest and the phone. A pose tracked by either (OpenXR's axes: x right, y up,
// z back, metres) is turned into the table's own frame, where the battle's
// centre is the origin. A wrong sign here puts the battle behind the player or
// spins it the wrong way as they walk round the table.

#include "doctest.h"

#include "utils/table_anchor.h"

#include <cmath>

namespace {

const float kPi = 3.14159265f;

bool near3(const float a[3], float x, float y, float z, float eps = 1e-4f)
{
	return std::fabs(a[0] - x) < eps && std::fabs(a[1] - y) < eps && std::fabs(a[2] - z) < eps;
}

// a quaternion's rotation of v
void rotate(const float q[4], const float v[3], float out[3])
{
	const float t[3] = {2.0f * (q[1] * v[2] - q[2] * v[1]),
	                    2.0f * (q[2] * v[0] - q[0] * v[2]),
	                    2.0f * (q[0] * v[1] - q[1] * v[0])};
	out[0] = v[0] + q[3] * t[0] + (q[1] * t[2] - q[2] * t[1]);
	out[1] = v[1] + q[3] * t[1] + (q[2] * t[0] - q[0] * t[2]);
	out[2] = v[2] + q[3] * t[2] + (q[0] * t[1] - q[1] * t[0]);
}

void yawQuat(float yaw, float q[4])
{
	q[0] = 0.0f; q[1] = std::sin(yaw * 0.5f); q[2] = 0.0f; q[3] = std::cos(yaw * 0.5f);
}

}

TEST_SUITE("TableAnchor") {

TEST_CASE("the table is placed ahead of the head and below it, facing the same way")
{
	// a head at (1, 1.6, 2) facing straight ahead (-z)
	const float head[3] = {1.0f, 1.6f, 2.0f};
	const table_anchor::Table t = table_anchor::inFront(head, 0.0f, 0.6f, 0.5f);
	CHECK(near3(t.position, 1.0f, 1.1f, 1.4f));
	CHECK(t.yaw == doctest::Approx(0.0f));

	// turned 90 degrees left (yaw about +y): ahead is -x
	const table_anchor::Table left = table_anchor::inFront(head, kPi * 0.5f, 0.6f, 0.5f);
	CHECK(near3(left.position, 0.4f, 1.1f, 2.0f));
	CHECK(left.yaw == doctest::Approx(kPi * 0.5f));
}

TEST_CASE("a pose in the table's frame: moved to its centre, turned with it")
{
	table_anchor::Table t;
	t.position[0] = 1.0f; t.position[1] = 1.0f; t.position[2] = -1.0f;
	t.yaw = 0.0f;

	// a head 0.5 m above the table and 0.4 m back from it, looking down a little
	float look[4];
	look[0] = std::sin(-0.3f); look[1] = 0.0f; look[2] = 0.0f; look[3] = std::cos(-0.3f);
	const float head[3] = {1.0f, 1.5f, -0.6f};
	float position[3], orientation[4];
	table_anchor::toTable(t, head, look, position, orientation);
	CHECK(near3(position, 0.0f, 0.5f, 0.4f));
	for(int i = 0; i < 4; ++i)
		CHECK(orientation[i] == doctest::Approx(look[i]));

	// the table turned 90 degrees left (its forward is now -x, its right -z):
	// the same head is now on its left
	t.yaw = kPi * 0.5f;
	table_anchor::toTable(t, head, look, position, orientation);
	CHECK(near3(position, -0.4f, 0.5f, 0.0f));
	// and its view turned the other way: what was the table's forward (-z)
	// is, seen from the head, 90 degrees to its right
	const float forward[3] = {0.0f, 0.0f, -1.0f};
	float flatLook[4];
	yawQuat(0.0f, flatLook);
	table_anchor::toTable(t, head, flatLook, position, orientation);
	float seen[3];
	rotate(orientation, forward, seen);
	CHECK(near3(seen, 1.0f, 0.0f, 0.0f));
}

TEST_CASE("walking round the table keeps looking at its centre")
{
	// the table is turned; a head walks to each of its sides and looks at the
	// centre: in the table's frame it is always at the table's distance
	table_anchor::Table t;
	t.position[0] = -2.0f; t.position[1] = 0.8f; t.position[2] = 3.0f;
	t.yaw = 0.7f;
	for(int side = 0; side < 4; ++side) {
		const float a = side * kPi * 0.5f;
		const float head[3] = {t.position[0] + 0.5f * std::sin(a), 1.3f, t.position[2] + 0.5f * std::cos(a)};
		float q[4];
		yawQuat(a, q);     // facing -z turned by a: towards the centre
		float position[3], orientation[4];
		table_anchor::toTable(t, head, q, position, orientation);
		CHECK(std::sqrt(position[0] * position[0] + position[2] * position[2]) == doctest::Approx(0.5f));
		CHECK(position[1] == doctest::Approx(0.5f));
		const float forward[3] = {0.0f, 0.0f, -1.0f};
		float seen[3];
		rotate(orientation, forward, seen);
		// the view points from the head's place on the table to its centre
		CHECK(seen[0] == doctest::Approx(-position[0] / 0.5f).epsilon(1e-3));
		CHECK(seen[2] == doctest::Approx(-position[2] / 0.5f).epsilon(1e-3));
	}
}

TEST_CASE("settings: the scale and where the table goes")
{
	table_anchor::Settings s = table_anchor::settingsFrom(nullptr, nullptr);
	CHECK(s.scale == doctest::Approx(table_anchor::kDefaultScale));
	CHECK(s.distance == doctest::Approx(table_anchor::kDefaultDistance));
	CHECK(s.drop == doctest::Approx(table_anchor::kDefaultDrop));

	s = table_anchor::settingsFrom("400", "0.8,0.6");
	CHECK(s.scale == doctest::Approx(400.0f));
	CHECK(s.distance == doctest::Approx(0.8f));
	CHECK(s.drop == doctest::Approx(0.6f));

	// out of range or not numbers: the defaults
	s = table_anchor::settingsFrom("5", "x,y");
	CHECK(s.scale == doctest::Approx(table_anchor::kDefaultScale));
	CHECK(s.distance == doctest::Approx(table_anchor::kDefaultDistance));
	s = table_anchor::settingsFrom("100000", "0.5");
	CHECK(s.scale == doctest::Approx(table_anchor::kDefaultScale));
	CHECK(s.distance == doctest::Approx(0.5f));
	CHECK(s.drop == doctest::Approx(table_anchor::kDefaultDrop));
}

}
