// tests/unit/test_group_goals.cpp
//
// code/groupgoals.h: a group move order whose search finds fewer goal cells
// than movers must still give every mover a goal (the order's own), never
// what the caller's list held before.

#include "doctest.h"

#include "groupgoals.h"

namespace {
struct Point {
	float x, y;
};
}

TEST_SUITE("GroupGoals")
{

TEST_CASE("found all: the list is left as the search wrote it")
{
	Point goals[3] = {{1, 1}, {2, 2}, {3, 3}};
	fillMissingGroupGoals(goals, 3, 3, Point{9, 9});
	CHECK(goals[0].x == 1);
	CHECK(goals[1].x == 2);
	CHECK(goals[2].x == 3);
}

TEST_CASE("found too few: the rest get the order's goal")
{
	// a 12-mech order on a crowded spot of a 300x300 map
	Point goals[12];
	for (int i = 0; i < 12; i++)
		goals[i] = Point{-12345, -12345};	// stale, as the caller's stack array
	goals[0] = Point{1, 1};
	goals[1] = Point{2, 2};
	fillMissingGroupGoals(goals, 2, 12, Point{9, 8});
	CHECK(goals[0].x == 1);
	CHECK(goals[1].x == 2);
	for (int i = 2; i < 12; i++) {
		CHECK(goals[i].x == 9);
		CHECK(goals[i].y == 8);
	}
}

TEST_CASE("found none: everyone gets the order's goal")
{
	Point goals[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
	fillMissingGroupGoals(goals, 0, 4, Point{5, 6});
	for (int i = 0; i < 4; i++)
		CHECK(goals[i].x == 5);
	fillMissingGroupGoals(goals, -1, 4, Point{7, 6});
	for (int i = 0; i < 4; i++)
		CHECK(goals[i].x == 7);
}

TEST_CASE("no movers: nothing is written")
{
	Point goals[1] = {{3, 3}};
	fillMissingGroupGoals(goals, 0, 0, Point{9, 9});
	CHECK(goals[0].x == 3);
}

}
