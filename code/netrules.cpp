// code/netrules.cpp: see netrules.h

#include "netrules.h"

namespace netrules {

namespace {

// The team with the most of something, or NO_TEAM if the best is shared.
template <typename Value>
int bestTeam(const GameState& state, Value value)
{
	int best = NO_TEAM;
	bool shared = false;
	for (int t = 0; t < NUM_TEAMS; t++) {
		if (!state.teams[t].playing)
			continue;
		if (best == NO_TEAM || value(state.teams[t]) > value(state.teams[best])) {
			best = t;
			shared = false;
		}
		else if (value(state.teams[t]) == value(state.teams[best]))
			shared = true;
	}
	return shared ? NO_TEAM : best;
}

long byScore(const TeamState& team)
{
	return team.score;
}

long byUnits(const TeamState& team)
{
	return team.liveUnits;
}

}

Result decide(const GameState& state)
{
	Result result;

	// In every mode, a team with no units left has lost.
	int alive = 0, lastAlive = NO_TEAM;
	for (int t = 0; t < NUM_TEAMS; t++)
		if (state.teams[t].playing && state.teams[t].liveUnits > 0) {
			alive++;
			lastAlive = t;
		}
	if (alive <= 1) {
		result.over = true;
		result.winner = lastAlive;
		return result;
	}

	if (state.mode == MODE_CAPTURE_BASE) {
		int bases = 0, lastBase = NO_TEAM, losers = 0;
		for (int t = 0; t < NUM_TEAMS; t++) {
			const TeamState& team = state.teams[t];
			if (!team.playing || !team.hasBase)
				continue;
			if (team.lostBase)
				losers++;
			else {
				bases++;
				lastBase = t;
			}
		}
		if (losers > 0 && bases <= 1) {
			result.over = true;
			result.winner = lastBase;
			return result;
		}
	}

	if (state.timeUp) {
		result.over = true;
		switch (state.mode) {
			case MODE_LAST_MAN_STANDING:
				result.winner = bestTeam(state, byUnits);
				break;
			case MODE_LAST_MAN_ON_THE_HILL:
				result.winner = state.hillTeam;
				break;
			default:
				result.winner = bestTeam(state, byScore);
				break;
		}
	}
	return result;
}

long pointsPerSecond(int mode, const TeamState& team)
{
	if (mode == MODE_KING_OF_THE_HILL || mode == MODE_LAST_MAN_ON_THE_HILL || mode == MODE_TERRITORIES)
		return team.held;
	return 0;
}

}
