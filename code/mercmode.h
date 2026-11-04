#ifndef MERCMODE_H
#define MERCMODE_H

#include <string>
#include <vector>

#include "merccontract.h"

class FitIniFile;
class FullPathFileName;

// The mercenary mode's game side (planning/future-features/OPEN-WORLD.md): the
// worlds of a campaign file with a [Merc] block, and the board of entries the
// mission selection screen shows for the current world. merccontract.cpp holds the rules.
class MercMode {
public:
	struct Entry {
		std::string fileName;
		std::string title;			// jumps only: a contract's is in its file
		std::string description;	// jumps only
		long jumpWorld = -1;		// -1: a contract
		bool jumpOpen = false;
		bool champion = false;
		bool rival = false;
	};

	// false if this campaign file is not a mercenary one
	bool init(FitIniFile& campaign);

	// the state's board for the current world; writes each contract's mission file
	const std::vector<Entry>& rebuild(long cBills);
	const Entry* entry(const char* fileName) const;

	const MercWorld& world() const { return worlds[static_cast<size_t>(state.world)]; }
	const MercWorld& worldAt(long index) const { return worlds[static_cast<size_t>(index)]; }
	long worldCount() const { return static_cast<long>(worlds.size()); }

	MercState state;
	MercRival rival;
	long lastPay = 0;
	long lastRepairs = 0;

private:
	std::vector<MercWorld> worlds;
	std::vector<Entry> board;
};

// Contract missions live in the save directory and use a stock mission's terrain.
// These are what the mission loaders use, so a stock mission name passes straight through.
const char* mercMissionTerrain(const char* missionName);
void mercMissionFitPath(FullPathFileName& path, const char* missionName);
long mercOpenMissionFit(FitIniFile& file, const char* missionName);

#endif
