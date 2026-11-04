#include "merccontract.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {

const char* const kContractPrefix = "merc_c";

// xorshift over a hash of the inputs: rolls must not depend on the platform's rand()
struct Rng {
	uint32_t s;
	Rng(uint32_t seed, uint32_t a, uint32_t b)
		: s((seed ^ (a * 2654435761u) ^ (b * 40503u)) | 1u)
	{
		for (int i = 0; i < 4; i++)
			next();
	}
	uint32_t next()
	{
		s ^= s << 13;
		s ^= s >> 17;
		s ^= s << 5;
		return s;
	}
	size_t pick(size_t count) { return count ? next() % count : 0; }
};

std::string trim(const std::string& text)
{
	const size_t first = text.find_first_not_of(" \t\r\n");
	if (first == std::string::npos)
		return "";
	return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
}

struct Block {
	std::string name;
	std::string text;		// everything after the [name] line
};

std::vector<Block> splitBlocks(const std::string& fit)
{
	std::vector<Block> blocks;
	size_t pos = 0;
	while (pos < fit.size()) {
		size_t end = fit.find('\n', pos);
		if (end == std::string::npos)
			end = fit.size();
		const std::string line = fit.substr(pos, end - pos);
		const size_t close = line.find(']');
		if (!line.empty() && line[0] == '[' && close != std::string::npos) {
			Block b;
			b.name = line.substr(1, close - 1);
			blocks.push_back(b);
		} else if (!blocks.empty() && line.rfind("FITend", 0) != 0) {
			blocks.back().text += line;
			blocks.back().text += '\n';
		}
		pos = end + 1;
	}
	return blocks;
}

const Block* findBlock(const std::vector<Block>& blocks, const std::string& name)
{
	for (const Block& b : blocks)
		if (b.name == name)
			return &b;
	return nullptr;
}

// "<type> <key> = <value>": the value of key, "" if absent
std::string valueOf(const std::string& text, const std::string& key)
{
	size_t pos = 0;
	while (pos < text.size()) {
		size_t end = text.find('\n', pos);
		if (end == std::string::npos)
			end = text.size();
		const std::string line = text.substr(pos, end - pos);
		const size_t eq = line.find('=');
		if (eq != std::string::npos) {
			const std::string left = trim(line.substr(0, eq));
			const size_t space = left.rfind(' ');
			if (space != std::string::npos && left.substr(space + 1) == key)
				return trim(line.substr(eq + 1));
		}
		pos = end + 1;
	}
	return "";
}

// replaces the value of key, keeping its type; a key not there is added, as `type key`
void setValue(std::string& text, const char* type, const std::string& key, const std::string& value)
{
	size_t pos = 0;
	while (pos < text.size()) {
		size_t end = text.find('\n', pos);
		if (end == std::string::npos)
			end = text.size();
		const std::string line = text.substr(pos, end - pos);
		const size_t eq = line.find('=');
		if (eq != std::string::npos) {
			const std::string left = trim(line.substr(0, eq));
			const size_t space = left.rfind(' ');
			if (space != std::string::npos && left.substr(space + 1) == key) {
				text.replace(pos, end - pos, left + " = " + value);
				return;
			}
		}
		pos = end + 1;
	}
	text += std::string(type) + " " + key + " = " + value + "\n";
}

std::string quoted(const std::string& text)
{
	std::string out = text;
	std::replace(out.begin(), out.end(), '"', '\'');
	return "\"" + out + "\"";
}

float numberOf(const std::string& text, const std::string& key)
{
	return static_cast<float>(std::atof(valueOf(text, key).c_str()));
}

bool startsWithNumberedBlock(const std::string& name, const char* prefix, const char* suffix)
{
	const size_t len = std::strlen(prefix);
	if (name.compare(0, len, prefix) != 0)
		return false;
	size_t i = len;
	while (i < name.size() && name[i] >= '0' && name[i] <= '9')
		i++;
	return i > len && name.compare(i, std::string::npos, suffix) == 0;
}

bool isUnitBlock(const std::string& name)
{
	return startsWithNumberedBlock(name, "Part", "") || startsWithNumberedBlock(name, "Warrior", "")
		|| (name.rfind("Warrior", 0) == 0 && name.find("Cell") != std::string::npos);
}

bool isReplacedBlock(const std::string& name)
{
	if (isUnitBlock(name) || name == "Warriors" || name == "Parts" || name == "Script")
		return true;
	if (name.rfind("Commander", 0) == 0 && name.find("Group:") != std::string::npos)
		return true;
	// every team's objectives: the enemy keeps none of the old mission's
	return name.rfind("Team", 0) == 0 && name.find("Objective") != std::string::npos;
}

std::string matesLine(long first, long count)
{
	std::string out;
	for (long i = 0; i < 12; i++) {
		out += std::to_string(i < count ? first + i : 0);
		out += ',';
	}
	return out;
}

char* formatFloat(char* buffer, size_t size, float value)
{
	std::snprintf(buffer, size, "%.6f", static_cast<double>(value));
	return buffer;
}

} // namespace

std::vector<std::string> mercParseList(const std::string& text)
{
	std::vector<std::string> out;
	size_t pos = 0;
	while (pos <= text.size()) {
		size_t end = text.find(',', pos);
		if (end == std::string::npos)
			end = text.size();
		const std::string item = trim(text.substr(pos, end - pos));
		if (!item.empty())
			out.push_back(item);
		pos = end + 1;
	}
	return out;
}

std::vector<MercUnit> mercParseUnits(const std::string& text)
{
	std::vector<MercUnit> out;
	for (const std::string& item : mercParseList(text)) {
		const size_t colon = item.find(':');
		if (colon == std::string::npos || colon == 0 || colon + 1 == item.size())
			continue;
		MercUnit unit;
		unit.objectNumber = std::atol(item.c_str());
		unit.csvFile = trim(item.substr(colon + 1));
		if (unit.objectNumber > 0)
			out.push_back(unit);
	}
	return out;
}

bool mercCanJump(const MercState& state, const std::vector<MercWorld>& worlds, long world)
{
	if (world < 0 || world >= static_cast<long>(worlds.size()) || world == state.world)
		return false;
	return state.licences >= worlds[static_cast<size_t>(world)].requiredLicences;
}

bool mercChampionAvailable(const MercState& state, const MercWorld& world)
{
	return !world.champions.empty() && state.licences < world.tier;
}

void mercChampionBeaten(MercState& state, const MercWorld& world)
{
	if (mercChampionAvailable(state, world))
		state.licences = world.tier;
}

MercContract mercRollContract(const MercState& state, const MercWorld& world, long slot)
{
	static const char* const kJobs[] = {
		"Clear the raiders",
		"Break the blockade",
		"Hunt the deserters",
		"Secure the supply line",
		"Drive off the pirates",
		"Answer the distress call",
	};
	static const char* const kOpeners[] = {
		"A local garrison will not ask where you came from, only whether the job gets done.",
		"Nobody else would take this one. The money is good because the odds are not.",
		"Word in the bar is that a lance has been bleeding the settlements dry.",
		"The client wants this quiet and quick, and will pay on proof.",
	};

	Rng rng(state.seed, static_cast<uint32_t>(state.contractsDone) * 7u + static_cast<uint32_t>(state.world) * 131u,
		static_cast<uint32_t>(slot));

	MercContract c;
	c.champion = slot == 2 && mercChampionAvailable(state, world);
	if (!world.maps.empty())
		c.map = world.maps[rng.pick(world.maps.size())];
	c.fileName = std::string(kContractPrefix) + std::to_string(slot) + "_" + c.map;

	const long tier = std::max(1L, world.tier);
	c.dropWeight = 150 + 50 * tier;
	c.resourcePoints = 12000 + 4000 * tier;

	if (c.champion) {
		c.title = "Licence fight: " + world.championName;
		c.blurb = world.championName + " holds the mercenary licence on " + world.name
			+ ". Beat this lance and the guild will open the next jump to you.";
		c.pilotProfile = "PMWVeteran";
		c.lance = world.champions;
		c.pay = 90000 * tier;
		c.dropWeight += 50;
	} else {
		c.title = kJobs[rng.pick(sizeof(kJobs) / sizeof(kJobs[0]))];
		c.blurb = std::string(kOpeners[rng.pick(sizeof(kOpeners) / sizeof(kOpeners[0]))])
			+ " Client: " + world.faction + ". Destroy the enemy lance on " + world.name + ".";
		c.pilotProfile = "PMWGreen";
		const size_t size = static_cast<size_t>(1 + tier) + rng.pick(2);
		for (size_t i = 0; i < size && !world.enemies.empty(); i++)
			c.lance.push_back(world.enemies[rng.pick(world.enemies.size())]);
		c.pay = (30000 + static_cast<long>(rng.pick(11)) * 2000) * tier;
	}
	return c;
}

bool mercIsContractName(const std::string& name)
{
	return !mercContractMap(name).empty();
}

std::string mercContractMap(const std::string& name)
{
	const size_t len = std::strlen(kContractPrefix);
	if (name.compare(0, len, kContractPrefix) != 0)
		return "";
	const size_t under = name.find('_', len);
	if (under == std::string::npos)
		return "";
	return name.substr(under + 1);
}

long mercRepairBill(float damageFraction, long tons)
{
	const float damage = std::min(1.0f, std::max(0.0f, damageFraction));
	return static_cast<long>(damage * static_cast<float>(tons) * 200.0f);
}

std::string mercBuildContractFit(const std::string& sourceFit, const MercContract& contract)
{
	std::string source = sourceFit;
	source.erase(std::remove(source.begin(), source.end(), '\r'), source.end());

	const std::vector<Block> blocks = splitBlocks(source);
	if (contract.lance.empty())
		return "";

	// the player's units, and where the mission started its enemies
	std::vector<const Block*> playerParts;
	std::vector<const Block*> playerWarriors;
	float px = 0.0f, py = 0.0f;
	for (const Block& b : blocks) {
		if (!startsWithNumberedBlock(b.name, "Part", "") || valueOf(b.text, "PlayerPart") != "TRUE")
			continue;
		playerParts.push_back(&b);
		playerWarriors.push_back(findBlock(blocks, "Warrior" + b.name.substr(4)));
		px += numberOf(b.text, "PositionX");
		py += numberOf(b.text, "PositionY");
	}
	if (playerParts.empty())
		return "";
	px /= static_cast<float>(playerParts.size());
	py /= static_cast<float>(playerParts.size());

	bool haveSpot = false;
	float spotX = 0.0f, spotY = 0.0f, best = -1.0f;
	for (const Block& b : blocks) {
		if (!startsWithNumberedBlock(b.name, "Part", "") || valueOf(b.text, "PlayerPart") == "TRUE")
			continue;
		const float x = numberOf(b.text, "PositionX"), y = numberOf(b.text, "PositionY");
		const float d = (x - px) * (x - px) + (y - py) * (y - py);
		if (d > best) {
			best = d;
			spotX = x;
			spotY = y;
			haveSpot = true;
		}
	}
	const Block* objective = findBlock(blocks, "Team0Objective0");
	const Block* settings = findBlock(blocks, "MissionSettings");
	if (!haveSpot || !objective || !settings)
		return "";

	std::string out = "FITini \n\n";
	char num[32];

	for (const Block& b : blocks) {
		if (isReplacedBlock(b.name))
			continue;
		std::string text = b.text;
		if (b.name == "MissionSettings") {
			setValue(text, "st", "MissionName", quoted(contract.title));
			setValue(text, "b", "MissionNameUseResourceString", "FALSE");
			setValue(text, "st", "Blurb", quoted(contract.blurb));
			setValue(text, "b", "BlurbUseResourceString", "FALSE");
			setValue(text, "st", "Blurb2", quoted(contract.blurb));
			setValue(text, "b", "Blurb2UseResourceString", "FALSE");
			setValue(text, "f", "DropWeightLimit", formatFloat(num, sizeof(num), static_cast<float>(contract.dropWeight)));
			setValue(text, "l", "ResourcePoints", std::to_string(contract.resourcePoints));
			setValue(text, "l", "AdditionalCBills", "0");
		}
		out += "[" + b.name + "]\n" + text + "\n";
	}

	const long numPlayer = static_cast<long>(playerParts.size());
	const long numEnemy = static_cast<long>(contract.lance.size());
	out += "[Warriors]\nul NumWarriors = " + std::to_string(numPlayer + numEnemy) + "\n\n";
	out += "[Parts]\nul NumParts = " + std::to_string(numPlayer + numEnemy) + "\n\n";

	for (long i = 0; i < numPlayer; i++) {
		const std::string id = std::to_string(i + 1);
		std::string warrior = playerWarriors[static_cast<size_t>(i)] ? playerWarriors[static_cast<size_t>(i)]->text : "";
		if (warrior.empty())
			warrior = "st Profile = \"pmp_bubba\"\nst Brain = \"PBrain\"\nl NumStaticVars = 0\n";
		setValue(warrior, "l", "NumCells", "0");
		std::string part = playerParts[static_cast<size_t>(i)]->text;
		setValue(part, "ul", "Pilot", id);
		out += "[Warrior" + id + "]\n" + warrior + "\n[Part" + id + "]\n" + part + "\n";
	}

	// a tight box around the spot: mission starts are on open ground
	static const float kOffsets[][2] = {{0, 0}, {128, 0}, {0, 128}, {128, 128}, {-128, 0}, {0, -128}, {-128, -128}, {-128, 128}};
	const size_t numOffsets = sizeof(kOffsets) / sizeof(kOffsets[0]);
	for (long i = 0; i < numEnemy; i++) {
		const std::string id = std::to_string(numPlayer + i + 1);
		const MercUnit& unit = contract.lance[static_cast<size_t>(i)];
		const float* off = kOffsets[static_cast<size_t>(i) % numOffsets];
		// the brain's settings, as all 677 of its uses in the stock missions have them
		out += "[Warrior" + id + "]\nst Profile = " + quoted(contract.pilotProfile)
			+ "\nst Brain = \"dredattack01\"\nl NumCells = 3\nl NumStaticVars = 0\n\n";
		out += "[Warrior" + id + "Cell0]\nl Cell = 2\nl MemType = 1\nf Value = 150.000000\n\n";
		out += "[Warrior" + id + "Cell1]\nl Cell = 9\nl MemType = 0\nf Value = 0.000000\n\n";
		out += "[Warrior" + id + "Cell2]\nl Cell = 28\nl MemType = 0\nf Value = 0.000000\n\n";
		out += "[Part" + id + "]\nul ObjectNumber = " + std::to_string(unit.objectNumber)
			+ "\nul ControlType = 2\nb PlayerPart = FALSE\nc MyIcon = 0\nc TeamID = 1\nc CommanderID = 1\nul Pilot = " + id;
		out += std::string("\nf PositionX = ") + formatFloat(num, sizeof(num), spotX + off[0]);
		out += std::string("\nf PositionY = ") + formatFloat(num, sizeof(num), spotY + off[1]);
		out += "\nf Rotation = 0.000000\nl Active = 1\nl Exists = 1\nf Damage = 0.000000"
			"\nul BaseColor = -5232600\nul HighlightColor1 = -11646908\nul HighlightColor2 = -11646908"
			"\nul SelfRepairBehavior = 1\nul ControlDataType = 1\nst ObjectProfile = \"PM101100\"\nst CSVFile = "
			+ quoted(unit.csvFile) + "\nul VariantNumber = 0\nul SquadNum = " + std::to_string(i + 1)
			+ "\nul NumAlternatives = 0\nul AlternativeStartIndex = 0"
			"\nl[15] IndicesOfAlternatives = -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,\n\n";
	}

	out += "[Commander0Group:0]\nl[12] Mates = " + matesLine(1, numPlayer) + "\n\n";
	out += "[Commander1Group:0]\nl[12] Mates = " + matesLine(numPlayer + 1, numEnemy) + "\n\n";

	std::string objectiveText = objective->text;
	setValue(objectiveText, "st", "Title", quoted(contract.title));
	setValue(objectiveText, "b", "TitleUseResourceString", "FALSE");
	setValue(objectiveText, "st", "Description", quoted(contract.title));
	setValue(objectiveText, "b", "DescriptionUseResourceString", "FALSE");
	setValue(objectiveText, "ul", "ResourcePoints", std::to_string(contract.pay));
	setValue(objectiveText, "b", "DisplayMarker", "TRUE");
	setValue(objectiveText, "f", "MarkerX", formatFloat(num, sizeof(num), spotX));
	setValue(objectiveText, "f", "MarkerY", formatFloat(num, sizeof(num), spotY));
	setValue(objectiveText, "ul", "NumConditions", "1");
	setValue(objectiveText, "ul", "NumActions", "0");
	setValue(objectiveText, "ul", "NumFailureConditions", "0");
	setValue(objectiveText, "ul", "NumFailureActions", "0");

	out += "[Team0Objectives]\nul NumObjectives = 1\n\n";
	out += "[Team0Objective0]\n" + objectiveText + "\n";
	out += "[Team0Objective0Condition0]\nul ConditionSpecies = 0\nst ConditionSpeciesString = \"DestroyAllEnemyUnits\"\n\n";
	for (int team = 1; team < 8; team++)
		out += "[Team" + std::to_string(team) + "Objectives]\nul NumObjectives = 0\n\n";

	out += "[Script]\nst ScenarioScript = \"mc2_gencon\"\n\nFITend \n";
	return out;
}
