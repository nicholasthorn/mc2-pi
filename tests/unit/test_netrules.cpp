// tests/unit/test_netrules.cpp
//
// netrules: when a multiplayer game ends and who wins, by mode.

#include "doctest.h"

#include "netrules.h"

#include <initializer_list>

using namespace netrules;

namespace {

// Two teams with units, playing.
GameState twoTeams(int mode)
{
	GameState state;
	state.mode = mode;
	for (int t = 0; t < 2; t++) {
		state.teams[t].playing = true;
		state.teams[t].liveUnits = 3;
	}
	return state;
}

const int ALL_MODES[] = { MODE_ELIMINATION, MODE_KING_OF_THE_HILL, MODE_CAPTURE_BASE, MODE_TERRITORIES,
	MODE_LAST_MAN_STANDING, MODE_LAST_MAN_ON_THE_HILL, MODE_OTHER };

}

TEST_SUITE("netrules") {

TEST_CASE("a game goes on while two teams have units and there is time")
{
	for (int mode : ALL_MODES) {
		const Result result = decide(twoTeams(mode));
		CHECK_FALSE(result.over);
	}
}

TEST_CASE("in every mode, the last team with units wins")
{
	for (int mode : ALL_MODES) {
		GameState state = twoTeams(mode);
		state.teams[0].liveUnits = 0;
		const Result result = decide(state);
		CHECK(result.over);
		CHECK(result.winner == 1);
	}
}

TEST_CASE("nobody left is a draw")
{
	GameState state = twoTeams(MODE_ELIMINATION);
	state.teams[0].liveUnits = 0;
	state.teams[1].liveUnits = 0;
	const Result result = decide(state);
	CHECK(result.over);
	CHECK(result.winner == NO_TEAM);
}

TEST_CASE("a team outside the game does not count")
{
	GameState state = twoTeams(MODE_ELIMINATION);
	state.teams[5].liveUnits = 3;	// not playing
	state.teams[0].liveUnits = 0;
	CHECK(decide(state).winner == 1);
}

TEST_CASE("at the time limit the highest score wins, and level is a draw")
{
	for (int mode : { MODE_ELIMINATION, MODE_KING_OF_THE_HILL, MODE_TERRITORIES, MODE_CAPTURE_BASE, MODE_OTHER }) {
		GameState state = twoTeams(mode);
		state.timeUp = true;
		state.teams[1].score = 40;
		state.teams[0].score = 10;
		Result result = decide(state);
		CHECK(result.over);
		CHECK(result.winner == 1);

		state.teams[0].score = 40;
		result = decide(state);
		CHECK(result.over);
		CHECK(result.winner == NO_TEAM);
	}
}

TEST_CASE("capture base: losing your base loses the game, whatever units are left")
{
	GameState state = twoTeams(MODE_CAPTURE_BASE);
	state.teams[0].hasBase = true;
	state.teams[1].hasBase = true;
	CHECK_FALSE(decide(state).over);

	state.teams[1].lostBase = true;
	const Result result = decide(state);
	CHECK(result.over);
	CHECK(result.winner == 0);
}

TEST_CASE("capture base: with three teams it goes on until one base is left")
{
	GameState state = twoTeams(MODE_CAPTURE_BASE);
	state.teams[2].playing = true;
	state.teams[2].liveUnits = 3;
	for (int t = 0; t < 3; t++)
		state.teams[t].hasBase = true;

	state.teams[0].lostBase = true;
	CHECK_FALSE(decide(state).over);
	state.teams[2].lostBase = true;
	CHECK(decide(state).winner == 1);
}

TEST_CASE("capture base: a map with no bases cannot end by capture")
{
	GameState state = twoTeams(MODE_CAPTURE_BASE);
	CHECK_FALSE(decide(state).over);
}

TEST_CASE("a lost base only matters in capture base")
{
	GameState state = twoTeams(MODE_ELIMINATION);
	state.teams[0].hasBase = true;
	state.teams[0].lostBase = true;
	CHECK_FALSE(decide(state).over);
}

TEST_CASE("last man standing: at the time limit the most units win, not the score")
{
	GameState state = twoTeams(MODE_LAST_MAN_STANDING);
	state.timeUp = true;
	state.teams[0].score = 1000;
	state.teams[1].liveUnits = 5;
	CHECK(decide(state).winner == 1);

	state.teams[1].liveUnits = 3;
	const Result result = decide(state);
	CHECK(result.over);
	CHECK(result.winner == NO_TEAM);
}

TEST_CASE("last man on the hill: whoever holds it at the time limit, whatever the score")
{
	GameState state = twoTeams(MODE_LAST_MAN_ON_THE_HILL);
	state.teams[0].score = 500;
	state.hillTeam = 1;
	CHECK_FALSE(decide(state).over);

	state.timeUp = true;
	CHECK(decide(state).winner == 1);

	state.hillTeam = NO_TEAM;
	const Result result = decide(state);
	CHECK(result.over);
	CHECK(result.winner == NO_TEAM);
}

TEST_CASE("points: a second per building held, in the modes that count it")
{
	TeamState team;
	team.held = 2;
	CHECK(pointsPerSecond(MODE_KING_OF_THE_HILL, team) == 2);
	CHECK(pointsPerSecond(MODE_LAST_MAN_ON_THE_HILL, team) == 2);
	CHECK(pointsPerSecond(MODE_TERRITORIES, team) == 2);
	CHECK(pointsPerSecond(MODE_ELIMINATION, team) == 0);
	CHECK(pointsPerSecond(MODE_CAPTURE_BASE, team) == 0);
	CHECK(pointsPerSecond(MODE_LAST_MAN_STANDING, team) == 0);
}

}
