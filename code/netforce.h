// code/netforce.h
//
// The units each player chooses in a game without quickstart (netwire::Force): what the host
// takes from a client, and where each machine puts them on the map. No GL, SDL or game
// dependency; the game says what a name means in its own data.

#ifndef NETFORCE_H
#define NETFORCE_H

#include "netwire.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace netforce {

// What this machine's data says of a unit's chassis and variant: known only for a designer
// variant the data has, not one a player made in the mech lab.
struct UnitFacts {
	bool known = false;
	int32_t cost = 0;			// C-Bills
	int32_t tons = 0;
};

struct Rules {
	size_t maxUnits = netwire::MAX_FORCE_UNITS;
	int32_t cBills = 0;
	int32_t dropWeight = 0;		// 0 for no limit
};

using LookUpUnit = std::function<UnitFacts(const netwire::ForceUnit&)>;
using KnowsPilot = std::function<bool(const std::string&)>;

// What the mech bay allows, checked again by the host: no more units than the limit, each
// known to the data, each with a pilot the data has and no pilot twice, and no more
// C-Bills or tons than the game gives. An empty force is allowed: that player has no units.
// If not, why not.
bool check(const netwire::Force& force, const Rules& rules, const LookUpUnit& lookUp, const KnowsPilot& knowsPilot, std::string* why = nullptr);

// How many units a player may have, as the mech bay and pilot screens count it: these shared
// out, and never more than a force holds. (MultiPlayer::moversPerCommander comes to the same.)
constexpr int LOGISTICS_UNITS = 36;		// MAX_MULTIPLAYER_MECHS_IN_LOGISTICS
size_t unitsPerPlayer(int players);

struct Spot {
	float x = 0.0f;
	float y = 0.0f;
};

// Where a player's units start: on the places the map gives that player's units, in order, then
// in square rings round them a spacing apart, each spot one the game can use (on the map, not
// blocked) and no nearer another unit than half the spacing. Every machine works this out for
// itself, so the offsets are whole multiples of the spacing: with a spacing a float holds
// exactly, every machine gets the same spots. A unit with no spot left goes on the place it
// would have had. None with no places.
using SpotUsable = std::function<bool(const Spot&)>;
std::vector<Spot> placeUnits(const std::vector<Spot>& places, size_t count, float spacing, const SpotUsable& usable);

}

#endif
