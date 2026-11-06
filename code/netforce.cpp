// code/netforce.cpp: see netforce.h

#include "netforce.h"

#include <algorithm>
#include <cctype>

using namespace netwire;

namespace netforce {

static std::string lowered(const std::string& text)
{
	std::string out = text;
	for (char& c : out)
		c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
	return out;
}

bool check(const Force& force, const Rules& rules, const LookUpUnit& lookUp, const KnowsPilot& knowsPilot, std::string* why)
{
	const char* reason = nullptr;
	int64_t cost = 0;
	int64_t tons = 0;
	std::vector<std::string> pilots;
	if (force.units.size() > rules.maxUnits)
		reason = "too many units";
	for (size_t i = 0; !reason && i < force.units.size(); i++) {
		const ForceUnit& unit = force.units[i];
		const UnitFacts facts = lookUp(unit);
		const std::string pilot = lowered(unit.pilot);
		if (!facts.known)
			reason = "a unit this data does not have";
		else if (facts.cost < 0 || facts.tons < 0)
			reason = "a unit with no cost or weight";
		else if (!knowsPilot(unit.pilot))
			reason = "a pilot this data does not have";
		else if (std::find(pilots.begin(), pilots.end(), pilot) != pilots.end())
			reason = "a pilot in two units";
		else {
			pilots.push_back(pilot);
			cost += facts.cost;
			tons += facts.tons;
		}
	}
	if (!reason && cost > rules.cBills)
		reason = "more C-Bills than the game gives";
	if (!reason && rules.dropWeight > 0 && tons > rules.dropWeight)
		reason = "heavier than the drop weight";
	if (why)
		*why = reason ? reason : "";
	return !reason;
}

size_t unitsPerPlayer(int players)
{
	// as MechBayScreen and PilotReadyScreen share it out
	const int force = static_cast<int>(MAX_FORCE_UNITS);
	return static_cast<size_t>((players > 0) ? std::min(LOGISTICS_UNITS / players, force) : force);
}

std::vector<Spot> placeUnits(const std::vector<Spot>& places, size_t count, float spacing)
{
	std::vector<Spot> spots;
	if (places.empty())
		return spots;
	for (size_t i = 0; i < count; i++) {
		Spot spot = places[i % places.size()];
		const size_t row = i / places.size();
		spot.x += spacing * static_cast<float>(row);
		spots.push_back(spot);
	}
	return spots;
}

}
