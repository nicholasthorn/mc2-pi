#include "mclib.h"
#include "mercmode.h"

#include <algorithm>
#include <cstdio>

namespace {

std::string readString(FitIniFile& file, const char* key)
{
	char buffer[1024];
	buffer[0] = 0;
	if (file.readIdString(key, buffer, sizeof(buffer) - 1) != NO_ERR)
		return "";
	return buffer;
}

long readLong(FitIniFile& file, const char* key, long fallback)
{
	long value = fallback;
	return file.readIdLong(key, value) == NO_ERR ? value : fallback;
}

std::string readWholeFile(const char* path)
{
	File file;
	if (file.open(path) != NO_ERR)
		return "";
	const unsigned long length = file.getLength();
	std::string text(length, '\0');
	if (length && file.read(reinterpret_cast<MemoryPtr>(&text[0]), length) != length)
		text.clear();
	file.close();
	return text;
}

// false on a stock mission that gave no contract, so the board can skip it
bool writeContract(const MercContract& contract)
{
	FullPathFileName source;
	source.init(missionPath, contract.map.c_str(), ".fit");
	const std::string fit = mercBuildContractFit(readWholeFile(source), contract);
	if (fit.empty()) {
		PAUSE(("Mercenary: no contract can be made from mission %s", contract.map.c_str()));
		return false;
	}

	FullPathFileName path;
	path.init(savePath, contract.fileName.c_str(), ".fit", true);
	FILE* out = fopen(path, "wb");
	if (!out) {
		PAUSE(("Mercenary: cannot write %s", (const char*)path));
		return false;
	}
	for (const char c : fit) {
		if (c == '\n')
			fputc('\r', out);
		fputc(c, out);
	}
	fclose(out);
	return true;
}

} // namespace

bool MercMode::init(FitIniFile& campaign)
{
	if (campaign.seekBlock("Merc") != NO_ERR)
		return false;

	const long count = readLong(campaign, "NumWorlds", 0);
	state = MercState();
	state.world = readLong(campaign, "StartWorld", 0);
	state.seed = static_cast<uint32_t>(readLong(campaign, "Seed", 1));

	worlds.clear();
	for (long i = 0; i < count; i++) {
		char blockName[32];
		sprintf(blockName, "MercWorld%ld", i);
		if (campaign.seekBlock(blockName) != NO_ERR) {
			PAUSE(("Mercenary: no block %s in the campaign file", blockName));
			continue;
		}
		MercWorld w;
		w.name = readString(campaign, "Name");
		w.faction = readString(campaign, "Faction");
		w.description = readString(campaign, "Description");
		w.championName = readString(campaign, "ChampionName");
		w.shopFile = readString(campaign, "Shop");
		w.operationFile = readString(campaign, "OperationFile");
		w.maps = mercParseList(readString(campaign, "Maps"));
		w.enemies = mercParseUnits(readString(campaign, "Enemies"));
		w.champions = mercParseUnits(readString(campaign, "Champions"));
		w.requiredLicences = readLong(campaign, "RequiredLicences", 0);
		w.jumpCost = readLong(campaign, "JumpCost", 0);
		w.tier = readLong(campaign, "Tier", 1);
		worlds.push_back(w);
	}
	if (worlds.empty()) {
		PAUSE(("Mercenary: the campaign file has no worlds"));
		return false;
	}
	state.world = std::max(0L, std::min(state.world, worldCount() - 1));
	return true;
}

const std::vector<MercMode::Entry>& MercMode::rebuild(long cBills)
{
	board.clear();

	// a save from a changed campaign file must not index past the list
	state.world = std::max(0L, std::min(state.world, worldCount() - 1));

	// three contracts then the worlds: the star map's buttons are in this order
	for (long slot = 0; slot < 3; slot++) {
		const MercContract contract = mercRollContract(state, world(), slot);
		if (!writeContract(contract))
			continue;
		Entry e;
		e.fileName = contract.fileName;
		e.champion = contract.champion;
		board.push_back(e);
	}

	for (long i = 0; i < worldCount(); i++) {
		const MercWorld& w = worldAt(i);
		Entry e;
		char text[1024];
		sprintf(text, "merc_jump%ld", i);
		e.fileName = text;
		e.jumpWorld = i;
		e.title = "Jump to " + w.name;
		e.jumpOpen = mercCanJump(state, worlds, i) && cBills >= w.jumpCost;
		if (i == state.world)
			e.description = "You are here.";
		else if (state.licences < w.requiredLicences)
			e.description = "The guild will not clear this jump until you hold more licences.";
		else if (cBills < w.jumpCost)
			e.description = "You cannot afford this jump.";
		else {
			S_snprintf(text, sizeof(text), "%s, held by %s. %s Jump cost: %ld C-Bills. Press Next to jump.",
				w.name.c_str(), w.faction.c_str(), w.description.c_str(), w.jumpCost);
			e.description = text;
		}
		board.push_back(e);
	}
	return board;
}

const MercMode::Entry* MercMode::entry(const char* fileName) const
{
	for (const Entry& e : board)
		if (S_stricmp(e.fileName.c_str(), fileName) == 0)
			return &e;
	return nullptr;
}

const char* mercMissionTerrain(const char* missionName)
{
	static std::string terrain;
	terrain = mercContractMap(missionName);
	return terrain.empty() ? missionName : terrain.c_str();
}

void mercMissionFitPath(FullPathFileName& path, const char* missionName)
{
	if (mercIsContractName(missionName))
		path.init(savePath, missionName, ".fit", true);
	else
		path.init(missionPath, missionName, ".fit");
}

long mercOpenMissionFit(FitIniFile& file, const char* missionName)
{
	FullPathFileName path;
	mercMissionFitPath(path, missionName);
	// the save directory can have capitals: File::open lowers names unless told not to
	return file.open(path, READ, 50, mercIsContractName(missionName));
}
