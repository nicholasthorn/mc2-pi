// tests/unit/test_move_save_bounds.cpp
//
// code/movesavebounds.h: a saved game's path counters must not index past the
// fixed arrays they are restored into, and a good save must not be refused.

#include "doctest.h"

#include "movesavebounds.h"

TEST_SUITE("MoveSaveBounds")
{

TEST_CASE("steps: what a running game can save is accepted")
{
	CHECK(savedStepsValid(0, 0, 50));		// no path
	CHECK(savedStepsValid(10, 3, 50));		// on the way
	CHECK(savedStepsValid(10, 10, 50));		// finished
	CHECK(savedStepsValid(50, 49, 50));		// a full array, last step
	CHECK(savedStepsValid(50, 50, 50));		// a full array, finished
	CHECK(savedStepsValid(0, 7, 50));		// count reset, cur left over
}

TEST_CASE("steps: what would index past the array is refused")
{
	CHECK_FALSE(savedStepsValid(51, 0, 50));
	CHECK_FALSE(savedStepsValid(-1, 0, 50));
	CHECK_FALSE(savedStepsValid(10, -1, 50));
	CHECK_FALSE(savedStepsValid(10, 50, 50));	// globalPath[50]
	CHECK_FALSE(savedStepsValid(10, 51, 50));
	CHECK_FALSE(savedStepsValid(50, 51, 50));
	CHECK_FALSE(savedStepsValid(127, 127, 50));	// the most a char can claim
	CHECK_FALSE(savedStepsValid(-128, -128, 50));
}

TEST_CASE("count")
{
	CHECK(savedCountValid(0, 50));
	CHECK(savedCountValid(50, 50));
	CHECK_FALSE(savedCountValid(51, 50));
	CHECK_FALSE(savedCountValid(-1, 50));
	CHECK_FALSE(savedCountValid(32767, 50));
}

TEST_CASE("index: -1 means none")
{
	CHECK(savedIndexValid(-1, 50));
	CHECK(savedIndexValid(0, 50));
	CHECK(savedIndexValid(49, 50));
	CHECK_FALSE(savedIndexValid(50, 50));
	CHECK_FALSE(savedIndexValid(-2, 50));
}

} // TEST_SUITE("MoveSaveBounds")
