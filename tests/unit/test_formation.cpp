// tests/unit/test_formation.cpp
//
// wolfman-MC2X's move formations (code/formation.h): slot shapes, placement
// in the world, which unit takes which slot, and the free-cell search.

#include "doctest.h"

#include "formation.h"

#include <cmath>
#include <set>
#include <utility>

using formation::Kind;
using formation::Point;

namespace
{

Point centroid(const Point* p, int n)
{
	Point c = { 0.f, 0.f };
	for (int i = 0; i < n; i++)
	{
		c.x += p[i].x;
		c.y += p[i].y;
	}
	c.x /= (float)n;
	c.y /= (float)n;
	return c;
}

float minGap(const Point* p, int n)
{
	float best = 1e30f;
	for (int i = 0; i < n; i++)
		for (int j = i + 1; j < n; j++)
			best = std::fmin(best, std::hypot(p[i].x - p[j].x, p[i].y - p[j].y));
	return best;
}

}

TEST_SUITE("Formation")
{

TEST_CASE("kindForButton: MC2X's palette buttons 24 to 33")
{
	CHECK(formation::kindForButton(24) == formation::kNone);
	CHECK(formation::kindForButton(25) == formation::kLine);
	CHECK(formation::kindForButton(26) == formation::kDoubleLine);
	CHECK(formation::kindForButton(27) == formation::kColumn);
	CHECK(formation::kindForButton(28) == formation::kDoubleColumn);
	CHECK(formation::kindForButton(29) == formation::kWedge);
	CHECK(formation::kindForButton(30) == formation::kArrow);
	CHECK(formation::kindForButton(31) == formation::kDispersion);
	CHECK(formation::kindForButton(32) == formation::kSmallCircle);
	CHECK(formation::kindForButton(33) == formation::kLargeCircle);
	CHECK(formation::kindForButton(23) == formation::kNotAFormation);
	CHECK(formation::kindForButton(34) == formation::kNotAFormation);
}

TEST_CASE("indicatorButton: 35 to 43 show the line to the large circle, none for kNone")
{
	CHECK(formation::indicatorButton(formation::kNone) == -1);
	CHECK(formation::indicatorButton(formation::kLine) == 35);
	CHECK(formation::indicatorButton(formation::kDispersion) == 41);
	CHECK(formation::indicatorButton(formation::kLargeCircle) == 43);
}

TEST_CASE("slots: every shape is centred, one slot apart at least, no two equal")
{
	for (int k = formation::kLine; k < formation::kCount; k++)
	{
		for (int n = 1; n <= 16; n++)
		{
			Point s[16];
			formation::slots((Kind)k, n, s);
			const Point c = centroid(s, n);
			CAPTURE(k);
			CAPTURE(n);
			CHECK(std::fabs(c.x) < 1e-4f);
			CHECK(std::fabs(c.y) < 1e-4f);
			if (n > 1)
				CHECK(minGap(s, n) >= 0.99f);
		}
	}
}

TEST_CASE("slots: the shapes")
{
	Point s[6];

	// a line is abreast: across the direction of travel
	formation::slots(formation::kLine, 3, s);
	for (int i = 0; i < 3; i++)
		CHECK(s[i].y == doctest::Approx(0.f));
	CHECK(s[0].x == doctest::Approx(-1.f));
	CHECK(s[2].x == doctest::Approx(1.f));

	// a column is single file, along it
	formation::slots(formation::kColumn, 3, s);
	for (int i = 0; i < 3; i++)
		CHECK(s[i].x == doctest::Approx(0.f));

	// two lines: two ranks
	formation::slots(formation::kDoubleLine, 6, s);
	std::set<float> ys;
	for (int i = 0; i < 6; i++)
		ys.insert(s[i].y);
	CHECK(ys.size() == 2);

	// two columns: two files
	formation::slots(formation::kDoubleColumn, 6, s);
	std::set<float> xs;
	for (int i = 0; i < 6; i++)
		xs.insert(s[i].x);
	CHECK(xs.size() == 2);

	// a wedge points forward: its leader is the furthest forward, alone
	formation::slots(formation::kWedge, 5, s);
	int front = 0;
	for (int i = 1; i < 5; i++)
		if (s[i].y > s[front].y)
			front = i;
	for (int i = 0; i < 5; i++)
		if (i != front)
			CHECK(s[i].y < s[front].y - 0.5f);

	// circles: every slot the same distance from the centre, the large one wider
	Point small[6], large[6];
	formation::slots(formation::kSmallCircle, 6, small);
	formation::slots(formation::kLargeCircle, 6, large);
	const float rs = std::hypot(small[0].x, small[0].y);
	for (int i = 0; i < 6; i++)
		CHECK(std::hypot(small[i].x, small[i].y) == doctest::Approx(rs));
	CHECK(std::hypot(large[0].x, large[0].y) > rs * 1.5f);

	// dispersion spreads wider than a plain grid
	formation::slots(formation::kDispersion, 4, s);
	CHECK(minGap(s, 4) >= 1.99f);
}

TEST_CASE("place: rotates the shape to the direction of travel and scales it")
{
	Point s[3];
	// travelling east (+x): a line runs north-south
	formation::place(formation::kLine, 3, Point{ 100.f, 200.f }, Point{ 1.f, 0.f }, 10.f, s);
	for (int i = 0; i < 3; i++)
		CHECK(s[i].x == doctest::Approx(100.f));
	CHECK(minGap(s, 3) == doctest::Approx(10.f));
	const Point c = centroid(s, 3);
	CHECK(c.y == doctest::Approx(200.f));

	// travelling north (+y): a column runs north-south too
	formation::place(formation::kColumn, 3, Point{ 0.f, 0.f }, Point{ 0.f, 1.f }, 10.f, s);
	for (int i = 0; i < 3; i++)
		CHECK(s[i].x == doctest::Approx(0.f));

	// no direction (already there): the shape faces north, not NaN
	formation::place(formation::kLine, 2, Point{ 5.f, 5.f }, Point{ 0.f, 0.f }, 10.f, s);
	CHECK(s[0].y == doctest::Approx(5.f));
	CHECK(s[1].y == doctest::Approx(5.f));
}

TEST_CASE("assign: each unit takes a different slot, the nearest ones first")
{
	const Point movers[3] = { { 0.f, 0.f }, { 10.f, 0.f }, { 20.f, 0.f } };
	const Point goals[3] = { { 21.f, 5.f }, { 1.f, 5.f }, { 11.f, 5.f } };
	int slotOf[3];
	formation::assign(movers, goals, 3, slotOf);
	CHECK(slotOf[0] == 1);
	CHECK(slotOf[1] == 2);
	CHECK(slotOf[2] == 0);

	// a permutation for any input
	const Point same[4] = { { 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f }, { 0.f, 0.f } };
	int s4[4];
	formation::assign(same, same, 4, s4);
	std::set<int> used(s4, s4 + 4);
	CHECK(used.size() == 4);
	CHECK(*used.begin() == 0);
	CHECK(*used.rbegin() == 3);
}

TEST_CASE("nearestFreeCell: the cell itself, else the closest free one in range")
{
	std::set<std::pair<int, int>> blocked = { { 5, 5 }, { 5, 6 }, { 4, 5 }, { 6, 5 } };
	auto isFree = [&](int r, int c) { return !blocked.count({ r, c }); };

	int r = 3, c = 3;
	CHECK(formation::nearestFreeCell(r, c, 2, isFree));
	CHECK(r == 3);
	CHECK(c == 3);

	r = 5;
	c = 5;
	CHECK(formation::nearestFreeCell(r, c, 2, isFree));
	CHECK(std::abs(r - 5) + std::abs(c - 5) == 1);	// the one free neighbour, (5,4)
	CHECK(r == 5);
	CHECK(c == 4);

	auto none = [](int, int) { return false; };
	r = 0;
	c = 0;
	CHECK_FALSE(formation::nearestFreeCell(r, c, 3, none));
}

TEST_CASE("Reach: cells walkable from the start, inside the square, not past a wall")
{
	// a wall down column 3, open at row 9 (outside the radius 4 around (0,0))
	auto passable = [](int r, int c) { return (c != 3) || (r == 9); };
	const formation::Reach reach(0, 0, 4, passable);
	CHECK(reach.contains(0, 0));
	CHECK(reach.contains(-4, 2));
	CHECK(reach.contains(4, -4));
	CHECK_FALSE(reach.contains(0, 3));	// the wall
	CHECK_FALSE(reach.contains(0, 4));	// past it: the way round is out of range
	CHECK_FALSE(reach.contains(5, 0));	// outside the square

	const formation::Reach wider(0, 0, 9, passable);
	CHECK(wider.contains(0, 4));	// round the end of the wall

	auto blocked = [](int, int) { return false; };
	const formation::Reach none(0, 0, 2, blocked);
	CHECK_FALSE(none.contains(0, 0));
}

}
