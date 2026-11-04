#include "doctest.h"

#include "merccontract.h"

#include <cstring>

namespace {

MercWorld testWorld()
{
	MercWorld w;
	w.name = "Tarsus";
	w.faction = "Steiner";
	w.championName = "Iron Duke";
	w.shopFile = "purchase02";
	w.maps = {"mc2_02", "mc2_05", "mc2_07"};
	w.enemies = mercParseUnits("48:Raven,46:Wolfhound,16:Hunchback");
	w.champions = mercParseUnits("51:Atlas,58:Zeus");
	w.tier = 2;
	w.jumpCost = 15000;
	return w;
}

const char* const kSource =
	"FITini \n\n"
	"[Water]\nf Elevation = -100.000000\n\n"
	"[Warriors]\nul NumWarriors = 3\n\n"
	"[Parts]\nul NumParts = 3\n\n"
	"[Warrior1]\nst Profile = \"pmp_bubba\"\nst Brain = \"PBrain\"\nl NumCells = 0\nl NumStaticVars = 0\n\n"
	"[Part1]\nul ObjectNumber = 45\nb PlayerPart = TRUE\nc TeamID = 0\nc CommanderID = 0\nul Pilot = 1\n"
	"f PositionX = 100.0\nf PositionY = 100.0\nst CSVFile = \"Werewolf\"\n\n"
	"[Warrior2]\nst Profile = \"PMWGreen\"\nst Brain = \"mc2_03_dred\"\nl NumCells = 3\nl NumStaticVars = 0\n\n"
	"[Part2]\nul ObjectNumber = 247\nb PlayerPart = FALSE\nc TeamID = 1\nc CommanderID = 1\nul Pilot = 2\n"
	"f PositionX = 200.0\nf PositionY = 100.0\nst CSVFile = \"Flea\"\n\n"
	"[Warrior3]\nst Profile = \"PMWGreen\"\nst Brain = \"mc2_03_dred\"\nl NumCells = 3\nl NumStaticVars = 0\n\n"
	"[Warrior3Cell0]\nl Cell = 2\nl MemType = 1\nf Value = 150.0\n\n"
	"[Part3]\nul ObjectNumber = 247\nb PlayerPart = FALSE\nc TeamID = 1\nc CommanderID = 1\nul Pilot = 3\n"
	"f PositionX = 5000.0\nf PositionY = -3000.0\nst CSVFile = \"Flea\"\n\n"
	"[Commander0Group:0]\nl[12] Mates = 1,0,0,0,0,0,0,0,0,0,0,0,\n\n"
	"[Commander1Group:0]\nl[12] Mates = 2,3,0,0,0,0,0,0,0,0,0,0,\n\n"
	"[MissionSettings]\nst MissionName = \"Ambush\"\nb MissionNameUseResourceString = TRUE\n"
	"ul MissionNameResourceStringID = 57003\nst Blurb = \"Old\"\nb BlurbUseResourceString = TRUE\n"
	"st Blurb2 = \"Old\"\nb Blurb2UseResourceString = TRUE\nf DropWeightLimit = 190.0\n"
	"l ResourcePoints = 16000\nl AdditionalCBills = 500\n\n"
	"[Team0Objectives]\nul NumObjectives = 2\n\n"
	"[Team0Objective0]\nst Title = \"Old\"\nb TitleUseResourceString = TRUE\nst Description = \"Old\"\n"
	"b DescriptionUseResourceString = TRUE\nul Priority = 1\nul ResourcePoints = 7000\nb DisplayMarker = FALSE\n"
	"f MarkerX = 0.0\nf MarkerY = 0.0\nul NumConditions = 2\nul NumActions = 1\n\n"
	"[Team0Objective0Condition0]\nul ConditionSpecies = 4\n\n"
	"[Team0Objective1]\nst Title = \"Gone\"\n\n"
	"[Team0Objective1Condition0]\nul ConditionSpecies = 4\n\n"
	"[Team1Objectives]\nul NumObjectives = 0\n\n"
	"[Script]\nst ScenarioScript = \"mc2_03\"\n\n"
	"FITend \n";

bool has(const std::string& text, const char* what)
{
	return text.find(what) != std::string::npos;
}

// the text of one [block], without the following block
std::string block(const std::string& text, const std::string& name)
{
	const size_t start = text.find("[" + name + "]");
	if (start == std::string::npos)
		return "";
	const size_t end = text.find("\n[", start + 1);
	return text.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

MercContract testContract()
{
	MercState state;
	MercContract c = mercRollContract(state, testWorld(), 0);
	c.title = "Hold the line";
	c.pay = 90000;
	c.dropWeight = 200;
	c.resourcePoints = 12000;
	return c;
}

} // namespace

TEST_SUITE("Merc")
{
	TEST_CASE("units and lists parse, skipping bad entries")
	{
		const std::vector<MercUnit> u = mercParseUnits("48:Raven, 46:Wolfhound,bad,:Zeus");
		REQUIRE(u.size() == 2);
		CHECK(u[0].objectNumber == 48);
		CHECK(u[0].csvFile == "Raven");
		CHECK(u[1].csvFile == "Wolfhound");
		CHECK(mercParseUnits("").empty());

		const std::vector<std::string> l = mercParseList("mc2_02, mc2_05,");
		REQUIRE(l.size() == 2);
		CHECK(l[1] == "mc2_05");
	}

	TEST_CASE("a jump needs enough licences and somewhere else to go")
	{
		std::vector<MercWorld> worlds(3);
		worlds[1].requiredLicences = 1;
		worlds[2].requiredLicences = 2;
		MercState s;
		CHECK_FALSE(mercCanJump(s, worlds, 0));	// already there
		CHECK_FALSE(mercCanJump(s, worlds, 1));
		s.licences = 1;
		CHECK(mercCanJump(s, worlds, 1));
		CHECK_FALSE(mercCanJump(s, worlds, 2));
		CHECK_FALSE(mercCanJump(s, worlds, -1));
		CHECK_FALSE(mercCanJump(s, worlds, 3));
	}

	TEST_CASE("each champion gives one licence")
	{
		MercWorld w;
		w.tier = 1;
		w.champions = mercParseUnits("51:Atlas");
		MercState s;
		CHECK(mercChampionAvailable(s, w));
		mercChampionBeaten(s, w);
		CHECK(s.licences == 1);
		CHECK_FALSE(mercChampionAvailable(s, w));
		mercChampionBeaten(s, w);
		CHECK(s.licences == 1);

		w.champions.clear();
		s.licences = 0;
		CHECK_FALSE(mercChampionAvailable(s, w));
	}

	TEST_CASE("contract rolls depend only on the state and slot")
	{
		const MercWorld w = testWorld();
		MercState s;
		s.seed = 1234;
		s.contractsDone = 3;
		const MercContract a = mercRollContract(s, w, 0);
		const MercContract b = mercRollContract(s, w, 0);
		CHECK(a.fileName == b.fileName);
		CHECK(a.title == b.title);
		REQUIRE(a.lance.size() == b.lance.size());
		for (size_t i = 0; i < a.lance.size(); i++)
			CHECK(a.lance[i].csvFile == b.lance[i].csvFile);

		// a board changes as jobs get done
		bool changed = false;
		for (long done = 4; done < 40 && !changed; done++) {
			s.contractsDone = done;
			const MercContract c = mercRollContract(s, w, 0);
			changed = c.fileName != a.fileName || c.title != a.title || c.pay != a.pay;
		}
		CHECK(changed);
	}

	TEST_CASE("the board's third slot is the champion until beaten")
	{
		const MercWorld w = testWorld();
		MercState s;
		s.licences = 1;		// tier 2 world: its champion is still open
		const MercContract champ = mercRollContract(s, w, 2);
		CHECK(champ.champion);
		CHECK(has(champ.title, "Iron Duke"));
		CHECK(champ.lance.size() == w.champions.size());

		s.licences = 2;
		CHECK_FALSE(mercRollContract(s, w, 2).champion);
		CHECK_FALSE(mercRollContract(s, w, 0).champion);
	}

	TEST_CASE("higher tiers pay more and bring more")
	{
		MercWorld low = testWorld();
		low.tier = 1;
		MercWorld high = testWorld();
		high.tier = 3;
		MercState s;
		const MercContract a = mercRollContract(s, low, 0);
		const MercContract b = mercRollContract(s, high, 0);
		CHECK(b.pay > a.pay);
		CHECK(b.lance.size() > a.lance.size());
		CHECK(b.dropWeight > a.dropWeight);
	}

	TEST_CASE("a contract names its terrain")
	{
		const MercContract c = mercRollContract(MercState(), testWorld(), 1);
		CHECK(mercIsContractName(c.fileName));
		CHECK(mercContractMap(c.fileName) == c.map);
		CHECK(mercContractMap("merc_c1_mc2_17") == "mc2_17");
		CHECK(mercContractMap("mc2_17") == "");
		CHECK_FALSE(mercIsContractName("mc2_17"));
		CHECK_FALSE(mercIsContractName("merc_c1_"));
	}

	TEST_CASE("repair bills grow with damage and size and are never negative")
	{
		CHECK(mercRepairBill(0.0f, 50) == 0);
		CHECK(mercRepairBill(-1.0f, 50) == 0);
		CHECK(mercRepairBill(0.5f, 50) > 0);
		CHECK(mercRepairBill(1.0f, 50) > mercRepairBill(0.5f, 50));
		CHECK(mercRepairBill(0.5f, 100) > mercRepairBill(0.5f, 50));
		CHECK(mercRepairBill(2.0f, 50) == mercRepairBill(1.0f, 50));
	}

	TEST_CASE("repairs never ground a company: no debt, and a stake for one with no mechs")
	{
		CHECK(mercCBillsAfterJob(50000, 10000, true) == 40000);
		CHECK(mercCBillsAfterJob(5000, 10000, true) == 0);
		// no mechs left, and not enough to buy one
		CHECK(mercCBillsAfterJob(5000, 10000, false) == MERC_GROUNDED_STAKE);
		CHECK(mercCBillsAfterJob(MERC_GROUNDED_STAKE + 500, 0, false) == MERC_GROUNDED_STAKE + 500);
	}

	TEST_CASE("building a contract keeps the terrain and player, replaces the rest")
	{
		const MercContract c = testContract();
		const std::string fit = mercBuildContractFit(kSource, c);
		REQUIRE_FALSE(fit.empty());
		CHECK(fit.rfind("FITini", 0) == 0);
		CHECK(has(fit, "FITend"));
		CHECK(has(fit, "[Water]"));

		const std::string player = block(fit, "Part1");
		CHECK(has(player.c_str(), "b PlayerPart = TRUE"));
		CHECK(has(player.c_str(), "Werewolf"));

		// old enemies, their brains and the mission's own script are gone
		CHECK_FALSE(has(fit, "Flea"));
		CHECK_FALSE(has(fit, "mc2_03_dred"));
		CHECK_FALSE(has(fit, "f Value = 150.0\n"));
		CHECK(has(block(fit, "Warrior2").c_str(), "l NumCells = 3"));
		CHECK(has(block(fit, "Warrior2Cell0").c_str(), "l Cell = 2"));
		CHECK(block(fit, "Warrior1Cell0").empty());
		CHECK_FALSE(has(fit, "mc2_03\""));
		CHECK(has(block(fit, "Script").c_str(), "mc2_gencon"));

		const long total = 1 + static_cast<long>(c.lance.size());
		CHECK(has(block(fit, "Parts").c_str(), ("NumParts = " + std::to_string(total)).c_str()));
		CHECK(has(block(fit, "Warriors").c_str(), ("NumWarriors = " + std::to_string(total)).c_str()));
		for (long i = 2; i <= total; i++) {
			const std::string part = block(fit, "Part" + std::to_string(i));
			REQUIRE_FALSE(part.empty());
			CHECK(has(part.c_str(), "b PlayerPart = FALSE"));
			CHECK(has(part.c_str(), "c TeamID = 1"));
			CHECK(has(part.c_str(), ("ul Pilot = " + std::to_string(i)).c_str()));
			CHECK(has(part.c_str(), c.lance[static_cast<size_t>(i - 2)].csvFile.c_str()));
			CHECK(has(block(fit, "Warrior" + std::to_string(i)).c_str(), "dredattack01"));
		}
	}

	TEST_CASE("enemies start at the spot farthest from the player, marked for them")
	{
		const std::string fit = mercBuildContractFit(kSource, testContract());
		CHECK(has(block(fit, "Part2").c_str(), "PositionX = 5000"));
		CHECK(has(block(fit, "Part2").c_str(), "PositionY = -3000"));
		const std::string obj = block(fit, "Team0Objective0");
		CHECK(has(obj.c_str(), "b DisplayMarker = TRUE"));
		CHECK(has(obj.c_str(), "MarkerX = 5000"));
		CHECK(has(obj.c_str(), "MarkerY = -3000"));
	}

	TEST_CASE("the lance is spread out around its start")
	{
		MercContract c = testContract();
		c.lance = mercParseUnits("48:Raven,46:Wolfhound,16:Hunchback,58:Zeus");
		const std::string fit = mercBuildContractFit(kSource, c);
		const std::string a = block(fit, "Part2"), b = block(fit, "Part3");
		const auto line = [](const std::string& s, const char* key) {
			const size_t at = s.find(key);
			return s.substr(at, s.find('\n', at) - at);
		};
		CHECK((line(a, "PositionX") != line(b, "PositionX") || line(a, "PositionY") != line(b, "PositionY")));
	}

	TEST_CASE("objectives become one paid job with no resource strings")
	{
		MercContract c = testContract();
		const std::string fit = mercBuildContractFit(kSource, c);
		CHECK(has(block(fit, "Team0Objectives").c_str(), "NumObjectives = 1"));
		CHECK_FALSE(has(fit, "Gone"));
		CHECK_FALSE(has(fit, "[Team0Objective1]"));
		const std::string obj = block(fit, "Team0Objective0");
		CHECK(has(obj.c_str(), "Hold the line"));
		CHECK(has(obj.c_str(), "b TitleUseResourceString = FALSE"));
		CHECK(has(obj.c_str(), "b DescriptionUseResourceString = FALSE"));
		CHECK(has(obj.c_str(), "ul ResourcePoints = 90000"));
		CHECK(has(obj.c_str(), "ul NumConditions = 1"));
		CHECK(has(obj.c_str(), "ul NumActions = 0"));
		CHECK(has(block(fit, "Team0Objective0Condition0").c_str(), "DestroyAllEnemyUnits"));

		const std::string settings = block(fit, "MissionSettings");
		CHECK(has(settings.c_str(), "Hold the line"));
		CHECK(has(settings.c_str(), "b MissionNameUseResourceString = FALSE"));
		CHECK(has(settings.c_str(), "b BlurbUseResourceString = FALSE"));
		CHECK(has(settings.c_str(), "b Blurb2UseResourceString = FALSE"));
		CHECK(has(settings.c_str(), "f DropWeightLimit = 200"));
		CHECK(has(settings.c_str(), "l ResourcePoints = 12000"));
		CHECK(has(settings.c_str(), "l AdditionalCBills = 0"));
	}

	TEST_CASE("command groups list the new units")
	{
		MercContract c = testContract();
		c.lance = mercParseUnits("48:Raven,46:Wolfhound");
		const std::string fit = mercBuildContractFit(kSource, c);
		CHECK(has(block(fit, "Commander0Group:0").c_str(), "Mates = 1,0,"));
		CHECK(has(block(fit, "Commander1Group:0").c_str(), "Mates = 2,3,0,"));
	}

	TEST_CASE("a mission that cannot be used gives no contract")
	{
		const MercContract c = testContract();
		CHECK(mercBuildContractFit("", c).empty());
		CHECK(mercBuildContractFit("FITini \n[Water]\nFITend \n", c).empty());
		MercContract none = c;
		none.lance.clear();
		CHECK(mercBuildContractFit(kSource, none).empty());
	}

	TEST_CASE("line endings in a stock file are handled")
	{
		std::string crlf;
		for (const char* p = kSource; *p; p++) {
			if (*p == '\n')
				crlf += '\r';
			crlf += *p;
		}
		const std::string fit = mercBuildContractFit(crlf, testContract());
		REQUIRE_FALSE(fit.empty());
		CHECK(has(block(fit, "Part1").c_str(), "Werewolf"));
	}
}
