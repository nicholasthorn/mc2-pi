// code/netrules.h
//
// When a multiplayer game is over, and who won, by game mode. The game's own rules went
// with the code Microsoft removed, so these are rebuilt from what the stock maps hold
// (planning/future-features/NETWORKING.md, "Game modes"). No game dependency: the game
// fills in a GameState and acts on the answer, and the tests do the same.

#ifndef NETRULES_H
#define NETRULES_H

namespace netrules {

// The order of MISSION_TYPE_* in multplyr.h, which is what a map's MissionType holds.
enum Mode {
	MODE_ELIMINATION = 0,
	MODE_KING_OF_THE_HILL,
	MODE_CAPTURE_BASE,
	MODE_TERRITORIES,
	MODE_LAST_MAN_STANDING,
	MODE_LAST_MAN_ON_THE_HILL,
	MODE_OTHER
};

const int NUM_TEAMS = 8;
const int NO_TEAM = -1;

struct TeamState {
	bool playing = false;	// a team of this game, whether or not it has units left
	long liveUnits = 0;
	long score = 0;
	bool hasBase = false;	// has a base to lose (capture base)
	bool lostBase = false;
	long held = 0;			// special buildings it holds now (hill, territories)
};

struct GameState {
	int mode = MODE_ELIMINATION;
	bool timeUp = false;
	int hillTeam = NO_TEAM;	// who holds the hill, for the hill modes
	TeamState teams[NUM_TEAMS];
};

struct Result {
	bool over = false;
	int winner = NO_TEAM;	// NO_TEAM when over is a draw
};

Result decide(const GameState& state);

// Points a team earns per second it holds what it holds, by mode.
long pointsPerSecond(int mode, const TeamState& team);

}

#endif
