// tests/unit/test_mc2xsupport.cpp
//
// wolfman-MC2X's support units (code/mc2xsupport.h).

#include "doctest.h"

#include "mc2xsupport.h"

#include <cstring>
#include <set>
#include <string>

TEST_SUITE("MC2XSupport")
{

TEST_CASE("unitForBlock: blocks 7 to 20 are MC2X's own units, the rest none")
{
	CHECK(mc2xsupport::unitForBlock(6) == nullptr);	// mobile artillery: stock's
	CHECK(mc2xsupport::unitForBlock(21) == nullptr);	// Cancel
	CHECK(mc2xsupport::unitForBlock(-1) == nullptr);
	CHECK(mc2xsupport::unitForBlock(22) == nullptr);
	for (int b = 7; b <= 20; b++)
	{
		const mc2xsupport::Unit* u = mc2xsupport::unitForBlock(b);
		REQUIRE(u != nullptr);
		CHECK(u->block == b);
		CHECK(u->objectNumber > 0);
		CHECK(u->cost > 0);
		CHECK(u->count >= 1);
		CHECK(std::strncmp(u->pilot, "pmw", 3) == 0);
	}
}

TEST_CASE("the units: inventory object numbers and help-text prices")
{
	const mc2xsupport::Unit* jamfly = mc2xsupport::unitForBlock(7);
	CHECK(jamfly->objectNumber == 1464);
	CHECK(jamfly->cost == 250000);
	CHECK(jamfly->missionSwitch == mc2xsupport::kSpotter);

	const mc2xsupport::Unit* swarm = mc2xsupport::unitForBlock(19);
	CHECK(swarm->count == 5);
	CHECK(swarm->objectNumber == mc2xsupport::unitForBlock(18)->objectNumber);
	CHECK(swarm->cost == 5 * mc2xsupport::unitForBlock(18)->cost);

	const mc2xsupport::Unit* deployer = mc2xsupport::unitForBlock(20);
	CHECK(deployer->objectNumber == 1473);
	CHECK(deployer->cost == 1020000);
	CHECK(deployer->missionSwitch == mc2xsupport::kDeployer);
}

TEST_CASE("unitForObject: finds the unit, the single kamikaze for the swarm's type")
{
	CHECK(mc2xsupport::unitForObject(1366)->block == 11);	// drone
	CHECK(mc2xsupport::unitForObject(1478)->block == 18);
	CHECK(mc2xsupport::unitForObject(182) == nullptr);	// stock's repair truck
}

TEST_CASE("pages: 14 units and Cancel fill the first tab's 15 slots, 7 on the second")
{
	int first = 0, second = 0;
	for (int b = 0; b < mc2xsupport::kButtonCount; b++)
	{
		const int page = mc2xsupport::pageForBlock(b);
		if (page == 1)
			first++;
		else if (page == 2)
			second++;
	}
	CHECK(first == 14);
	CHECK(second == 7);
	CHECK(mc2xsupport::pageForBlock(mc2xsupport::kCancel) == 0);
}

TEST_CASE("costForBlock: stock's seven at MC2X's prices, Cancel free")
{
	CHECK(mc2xsupport::costForBlock(0) == 20000);	// airstrike
	CHECK(mc2xsupport::costForBlock(1) == 3000);	// sensor probe
	CHECK(mc2xsupport::costForBlock(5) == 25000);	// salvage craft
	CHECK(mc2xsupport::costForBlock(13) == 20000);	// mercenaries
	CHECK(mc2xsupport::costForBlock(mc2xsupport::kCancel) == 0);
	CHECK(mc2xsupport::costForBlock(99) == 0);
}

TEST_CASE("switchKey: one distinct mission key per switch")
{
	std::set<std::string> keys;
	for (int s = 0; s < mc2xsupport::kSwitchCount; s++)
	{
		const char* key = mc2xsupport::switchKey((mc2xsupport::Switch)s);
		REQUIRE(key != nullptr);
		keys.insert(key);
	}
	CHECK(keys.size() == (size_t)mc2xsupport::kSwitchCount);
	CHECK(std::string(mc2xsupport::switchKey(mc2xsupport::kDrone)) == "DroneEnabledDefault");
	CHECK(mc2xsupport::switchKey(mc2xsupport::kSwitchCount) == nullptr);
}

}
