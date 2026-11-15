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

// groupGoalAreaUsable: a goal cell is given out only where a unit can get to.
TEST_CASE("goal area: a cell in no area is never a goal")
{
	GroupGoalAreas asked;
	long calls = 0;
	auto reach = [&calls](long) { calls++; return true; };
	CHECK_FALSE(groupGoalAreaUsable(-1, 5, asked, reach));
	CHECK_FALSE(groupGoalAreaUsable(-1, -1, asked, reach));
	CHECK(calls == 0);
}

TEST_CASE("goal area: the clicked area needs no route")
{
	GroupGoalAreas asked;
	long calls = 0;
	auto reach = [&calls](long) { calls++; return false; };
	CHECK(groupGoalAreaUsable(5, 5, asked, reach));
	// a click on a cell in no area has nothing to measure from: any area will do
	CHECK(groupGoalAreaUsable(7, -1, asked, reach));
	CHECK(calls == 0);
}

TEST_CASE("goal area: another area must be reachable, asked once")
{
	GroupGoalAreas asked;
	long calls = 0;
	auto reach = [&calls](long area) { calls++; return area != 9; };	// 9: behind a closed gate
	CHECK(groupGoalAreaUsable(6, 5, asked, reach));
	CHECK_FALSE(groupGoalAreaUsable(9, 5, asked, reach));
	CHECK(groupGoalAreaUsable(6, 5, asked, reach));
	CHECK_FALSE(groupGoalAreaUsable(9, 5, asked, reach));
	CHECK(calls == 2);
}

TEST_CASE("goal area: past the cache's size an area is asked again, with the same answer")
{
	GroupGoalAreas asked;
	long calls = 0;
	auto reach = [&calls](long area) { calls++; return (area % 2) == 0; };
	for (long a = 100; a < 100 + GroupGoalAreas::SIZE; a++)
		groupGoalAreaUsable(a, 1, asked, reach);
	CHECK(asked.count == GroupGoalAreas::SIZE);
	CHECK(groupGoalAreaUsable(1000, 1, asked, reach));
	CHECK(groupGoalAreaUsable(1000, 1, asked, reach));
	CHECK_FALSE(groupGoalAreaUsable(1001, 1, asked, reach));
	CHECK(asked.count == GroupGoalAreas::SIZE);
	CHECK(calls == GroupGoalAreas::SIZE + 3);
}

}
