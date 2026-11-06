// The units players choose in a game without quickstart (code/netforce.*, netwire::Force).

#include "doctest.h"

#include "netforce.h"
#include "netwire.h"

#include <string>
#include <vector>

using namespace netwire;
using netforce::Rules;
using netforce::Spot;
using netforce::UnitFacts;

namespace {

ForceUnit unit(const char* chassis, uint8_t variant, const char* pilot)
{
	ForceUnit u;
	u.chassis = chassis;
	u.variant = variant;
	u.pilot = pilot;
	return u;
}

// a small data set: two chassis with designer variants
UnitFacts lookUp(const ForceUnit& u)
{
	UnitFacts facts;
	if (u.chassis == "data\\objects\\atlas.csv" && u.variant <= 1) {
		facts.known = true;
		facts.cost = 3000000;
		facts.tons = 100;
	}
	else if (u.chassis == "data\\objects\\raven.csv" && u.variant == 0) {
		facts.known = true;
		facts.cost = 1000000;
		facts.tons = 35;
	}
	return facts;
}

bool knowsPilot(const std::string& pilot)
{
	return pilot == "pmw00031" || pilot == "pmw00032" || pilot == "PMW00033";
}

Rules generous()
{
	Rules rules;
	rules.maxUnits = 4;
	rules.cBills = 10000000;
	rules.dropWeight = 300;
	return rules;
}

std::string why(const Force& force, const Rules& rules)
{
	std::string reason;
	netforce::check(force, rules, lookUp, knowsPilot, &reason);
	return reason;
}

}

TEST_SUITE("NetForce") {

TEST_CASE("A force within the rules is taken, and so is none")
{
	Force force;
	CHECK(netforce::check(force, generous(), lookUp, knowsPilot));
	force.units.push_back(unit("data\\objects\\atlas.csv", 1, "pmw00031"));
	force.units.push_back(unit("data\\objects\\raven.csv", 0, "pmw00032"));
	std::string reason = "unchanged";
	CHECK(netforce::check(force, generous(), lookUp, knowsPilot, &reason));
	CHECK(reason.empty());
}

TEST_CASE("What the mech bay would not allow is refused, with why")
{
	Force force;
	force.units.push_back(unit("data\\objects\\atlas.csv", 2, "pmw00031"));
	CHECK(why(force, generous()) == "a unit this data does not have");

	// a variant of a player's own, which the data's designer variants do not have
	force.units[0] = unit("data\\objects\\raven.csv", 7, "pmw00031");
	CHECK(why(force, generous()) == "a unit this data does not have");

	force.units[0] = unit("data\\objects\\raven.csv", 0, "nobody");
	CHECK(why(force, generous()) == "a pilot this data does not have");

	// one pilot in two mechs, whatever the case of the name
	force.units[0] = unit("data\\objects\\raven.csv", 0, "PMW00033");
	force.units.push_back(unit("data\\objects\\raven.csv", 0, "PMW00033"));
	CHECK(why(force, generous()) == "a pilot in two units");
}

TEST_CASE("No more units, C-Bills or tons than the game gives")
{
	Force force;
	force.units.push_back(unit("data\\objects\\atlas.csv", 0, "pmw00031"));
	force.units.push_back(unit("data\\objects\\atlas.csv", 1, "pmw00032"));
	force.units.push_back(unit("data\\objects\\atlas.csv", 0, "PMW00033"));

	Rules rules = generous();
	CHECK(netforce::check(force, rules, lookUp, knowsPilot));
	rules.maxUnits = 2;
	CHECK(why(force, rules) == "too many units");

	rules = generous();
	rules.cBills = 8999999;
	CHECK(why(force, rules) == "more C-Bills than the game gives");
	rules.cBills = 9000000;
	CHECK(netforce::check(force, rules, lookUp, knowsPilot));

	rules = generous();
	rules.dropWeight = 299;
	CHECK(why(force, rules) == "heavier than the drop weight");
	rules.dropWeight = 0;	// no limit
	CHECK(netforce::check(force, rules, lookUp, knowsPilot));
}

TEST_CASE("Units per player: the mech bay's share, never more than a force holds")
{
	// the same as MultiPlayer::moversPerCommander for every count of players
	const size_t movers[8] = { 12, 12, 12, 9, 7, 6, 5, 4 };
	for (int players = 1; players <= 8; players++)
		CHECK(netforce::unitsPerPlayer(players) == movers[players - 1]);
	CHECK(netforce::unitsPerPlayer(0) == 12);
}

TEST_CASE("Units start on the map's places, then in rows beside them")
{
	std::vector<Spot> places(2);
	places[0].x = 100.0f;
	places[0].y = 200.0f;
	places[1].x = 300.0f;
	places[1].y = 200.0f;
	const std::vector<Spot> spots = netforce::placeUnits(places, 5, 64.0f);
	REQUIRE(spots.size() == 5);
	CHECK(spots[0].x == 100.0f);
	CHECK(spots[1].x == 300.0f);
	CHECK(spots[2].x == 164.0f);
	CHECK(spots[3].x == 364.0f);
	CHECK(spots[4].x == 228.0f);
	CHECK(spots[4].y == 200.0f);
	CHECK(netforce::placeUnits({}, 3, 64.0f).empty());
	CHECK(netforce::placeUnits(places, 0, 64.0f).empty());
}

TEST_CASE("Force: round trip")
{
	Force force;
	force.commander = 3;
	force.units.push_back(unit("data\\objects\\atlas.csv", 1, "pmw00031"));
	force.units.push_back(unit("data\\objects\\raven.csv", 0, "pmw00032"));
	std::vector<uint8_t> out;
	REQUIRE(encode(force, out));
	MessageType type;
	REQUIRE(peekType(out.data(), out.size(), type));
	CHECK(type == MessageType::Force);
	Force back;
	REQUIRE(decode(out.data(), out.size(), back));
	CHECK(back.commander == 3);
	REQUIRE(back.units.size() == 2);
	CHECK(back.units[0].chassis == "data\\objects\\atlas.csv");
	CHECK(back.units[0].variant == 1);
	CHECK(back.units[1].pilot == "pmw00032");

	Force none;
	REQUIRE(encode(none, out));
	REQUIRE(decode(out.data(), out.size(), back));
	CHECK(back.units.empty());
}

TEST_CASE("Force: the largest fits a message, and what no game sends is refused")
{
	Force force;
	force.commander = MAX_PLAYERS - 1;
	for (size_t i = 0; i < MAX_FORCE_UNITS; i++)
		force.units.push_back(unit(std::string(MAX_FORCE_TEXT_LENGTH, 'c').c_str(), 255, std::string(MAX_FORCE_TEXT_LENGTH, 'p').c_str()));
	std::vector<uint8_t> out;
	REQUIRE(encode(force, out));
	CHECK(out.size() <= MAX_MESSAGE_SIZE);
	for (size_t size = 0; size < out.size(); size++) {
		Force back;
		CHECK_FALSE(decode(out.data(), size, back));
	}
	std::vector<uint8_t> longer = out;
	longer.push_back(0);
	Force back;
	CHECK_FALSE(decode(longer.data(), longer.size(), back));

	Force bad = force;
	bad.units.push_back(force.units[0]);	// one too many
	CHECK_FALSE(encode(bad, out));
	bad = force;
	bad.commander = MAX_PLAYERS;
	CHECK_FALSE(encode(bad, out));
	bad = force;
	bad.units[0].chassis += "c";	// too long
	CHECK_FALSE(encode(bad, out));
	bad = force;
	bad.units[0].pilot = "";
	CHECK_FALSE(encode(bad, out));
	bad = force;
	bad.units[0].pilot = "100%";
	CHECK_FALSE(encode(bad, out));
	bad = force;
	bad.units[0].chassis = "tab\there";
	CHECK_FALSE(encode(bad, out));

	// a count past the limit, on the wire
	const uint8_t tooMany[] = { static_cast<uint8_t>(MessageType::Force), 0, static_cast<uint8_t>(MAX_FORCE_UNITS + 1) };
	CHECK_FALSE(decode(tooMany, sizeof(tooMany), back));
}

}
