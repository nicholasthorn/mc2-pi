#ifndef MERCCONTRACT_H
#define MERCCONTRACT_H

#include <cstdint>
#include <string>
#include <vector>

// The rules of the mercenary mode (planning/future-features/OPEN-WORLD.md):
// worlds, licences, contract rolls, and turning a stock mission into a contract.
// No GL, SDL or game dependency, so it can be unit tested; code/mercmode.cpp
// connects it to the game.

struct MercUnit {
	long objectNumber = 0;
	std::string csvFile;
};

struct MercWorld {
	std::string name;
	std::string faction;
	std::string description;
	std::string championName;
	std::string shopFile;			// purchase file offered here, without path or extension
	std::string operationFile;		// star map view of this world
	std::vector<std::string> maps;	// stock missions whose terrain contracts use
	std::vector<MercUnit> enemies;
	std::vector<MercUnit> champions;
	long requiredLicences = 0;		// licences held before you can jump here
	long jumpCost = 0;
	long tier = 1;					// sets contract size and pay
};

struct MercState {
	long world = 0;
	long licences = 0;				// champions beaten, each opens the next world
	uint32_t seed = 1;
	long contractsDone = 0;			// changes what the board offers

	// the rival's memory of you
	long rivalRank = 0;				// rises after every fight, whoever wins
	long rivalWins = 0;				// fights the rival won
	long rivalLosses = 0;			// fights you won: MERC_RIVAL_FIGHTS_TO_END ends it
	long rivalNextFight = 2;		// on the board once contractsDone reaches this
	long rivalLastWorld = -1;		// where you last fought, -1 before the first fight
	bool rivalWonLast = false;
};

// The rival of a campaign: the same person for the whole game, with one ride per rank.
struct MercRival {
	std::string name;
	std::string unit;
	std::vector<MercUnit> mechs;
};

const long MERC_RIVAL_FIGHTS_TO_END = 3;
// jobs and jumps before the rival comes back
const long MERC_RIVAL_COOLDOWN = 3;

struct MercContract {
	std::string fileName;			// merc_c<slot>_<map>: see mercContractMap
	std::string map;
	std::string title;
	std::string blurb;
	std::string pilotProfile;
	std::vector<MercUnit> lance;
	long pay = 0;
	long dropWeight = 0;
	long resourcePoints = 0;
	bool champion = false;
	bool rival = false;
};

// parses "48:Raven,46:Wolfhound"; entries without a number are dropped
std::vector<MercUnit> mercParseUnits(const std::string& text);
std::vector<std::string> mercParseList(const std::string& text);

bool mercCanJump(const MercState& state, const std::vector<MercWorld>& worlds, long world);
// beating the champion of a world opens the next: a repeat fight opens nothing
bool mercChampionAvailable(const MercState& state, const MercWorld& world);
void mercChampionBeaten(MercState& state, const MercWorld& world);

// the same state and slot always give the same contract, so a save holds only the state
MercContract mercRollContract(const MercState& state, const MercWorld& world, long slot);

bool mercRivalDue(const MercState& state, const MercRival& rival);
const char* mercRivalRankName(long rank);
// the rival's fight on the current world: the taunt remembers the last one
MercContract mercRollRivalContract(const MercState& state, const std::vector<MercWorld>& worlds,
	const MercRival& rival, long slot);
void mercRivalBeaten(MercState& state);		// you won the fight
void mercRivalWon(MercState& state);		// the rival won it
// one line for the board about where things stand with the rival, "" before the first fight
std::string mercRivalNews(const MercState& state, const MercRival& rival);

bool mercIsContractName(const std::string& name);
// the stock mission whose terrain a contract uses, "" if name is not a contract
std::string mercContractMap(const std::string& name);

// the cost of a mech that came home with this fraction of its armour gone
long mercRepairBill(float damageFraction, long tons);

// the guild's stake for a company with no mechs: enough to buy one
const long MERC_GROUNDED_STAKE = 100000;
// C-Bills once the repairs are paid: never debt, and never stuck with no way to fight
long mercCBillsAfterJob(long cBills, long repairs, bool hasMechs);

// Cell grids hold 1 for true, row by row. The cells a walk from the start can get to, on
// passable cells, and the true cell nearest to (row, col); false if there is none.
std::vector<uint8_t> mercReachableCells(const std::vector<uint8_t>& passable, long width, long height,
	long startRow, long startCol);
bool mercNearestCell(const std::vector<uint8_t>& cells, long width, long height, long row, long col,
	long& outRow, long& outCol);

// The text of a mission file for the contract: the terrain settings and player
// units of the stock mission, with its enemies, objectives and script replaced.
// Enemies start at the spot farthest from the player that the mission used;
// Mission::init moves any the player could not walk to. Returns "" if the source has no player units,
// enemies or objective to work from.
std::string mercBuildContractFit(const std::string& sourceFit, const MercContract& contract);

#endif
