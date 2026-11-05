// tests/unit/test_netlan.cpp: finding a game on the local network, over a real socket on
// this machine (the loopback address: no network needed).

#include "doctest.h"

#include "netlan.h"

#include <chrono>
#include <cstring>
#include <thread>

using namespace netwire;

namespace {

// Real sockets: what was sent may take a moment to arrive.
void aMoment()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
}

LanAnnounce someGame()
{
	LanAnnounce a;
	a.gamePort = 27815;
	a.instance = 0x12345678u;
	a.numPlayers = 1;
	a.maxPlayers = 4;
	a.name = "Nick's game";
	a.map = "mc2_m08";
	a.lobby = true;
	return a;
}

}

TEST_SUITE("Lan") {

TEST_CASE("LanQuery and LanAnnounce: round trips")
{
	std::vector<uint8_t> out;
	REQUIRE(encode(LanQuery(), out));
	LanQuery query;
	REQUIRE(decode(out.data(), out.size(), query));
	CHECK(query.protocol == PROTOCOL_VERSION);

	const LanAnnounce a = someGame();
	REQUIRE(encode(a, out));
	LanAnnounce back;
	REQUIRE(decode(out.data(), out.size(), back));
	CHECK(back.protocol == PROTOCOL_VERSION);
	CHECK(back.gamePort == a.gamePort);
	CHECK(back.instance == a.instance);
	CHECK(back.numPlayers == a.numPlayers);
	CHECK(back.maxPlayers == a.maxPlayers);
	CHECK(back.inProgress == a.inProgress);
	CHECK(back.name == a.name);
	CHECK(back.map == a.map);
	CHECK(back.lobby == a.lobby);
}

TEST_CASE("LanAnnounce: what no game announces is refused")
{
	std::vector<uint8_t> out;
	LanAnnounce a = someGame();
	a.numPlayers = 5;	// more than it takes
	CHECK_FALSE(encode(a, out));
	a = someGame();
	a.maxPlayers = MAX_PLAYERS + 1;
	CHECK_FALSE(encode(a, out));
	a = someGame();
	a.gamePort = 0;
	CHECK_FALSE(encode(a, out));
	a = someGame();
	a.map = "";		// a lobby that has not chosen one yet is still a game to join
	CHECK(encode(a, out));
	for (const char* path : { "../../secret", "a/b", "a\\b", "c:map", "..", "a.b", " map", "map ", "con", "NUL", "com1", "LPT9" }) {
		a = someGame();
		a.map = path;
		CHECK_FALSE(encode(a, out));
	}
	a = someGame();
	a.name = std::string(MAX_NAME_LENGTH + 1, 'x');
	CHECK_FALSE(encode(a, out));
	a = someGame();
	a.name = "100%";	// a name goes through printf-like text on screen
	CHECK_FALSE(encode(a, out));
}

TEST_CASE("Lan messages: anything else on the port is not one, whole or cut short")
{
	std::vector<uint8_t> good;
	REQUIRE(encode(someGame(), good));
	LanAnnounce back;
	LanQuery query;
	for (size_t size = 0; size < good.size(); size++)
		CHECK_FALSE(decode(good.data(), size, back));
	CHECK_FALSE(decode(good.data(), good.size(), query));

	// another program's traffic: the magic word does not match
	std::vector<uint8_t> other = good;
	other[1] ^= 0xFFu;
	CHECK_FALSE(decode(other.data(), other.size(), back));

	// one byte on the end
	other = good;
	other.push_back(0);
	CHECK_FALSE(decode(other.data(), other.size(), back));
}

TEST_CASE("A browser finds a beacon on this machine, and forgets it when it stops")
{
	netlan::Beacon beacon;
	REQUIRE(beacon.open(0));
	netlan::Browser browser;
	REQUIRE(browser.open());

	double now = 1.0;
	for (int i = 0; (i < 2000) && browser.found().empty(); i++) {
		browser.update(now, beacon.port());
		beacon.update(someGame());
		now += 0.001;
		aMoment();
	}
	REQUIRE(browser.found().size() == 1);
	CHECK(browser.found()[0].announce.map == "mc2_m08");
	CHECK(browser.found()[0].announce.gamePort == 27815);
	CHECK_FALSE(browser.found()[0].address.empty());

	// asked again: still one host, whichever way the answer came
	for (int i = 0; i < 4; i++) {
		now += netlan::QUERY_SECONDS + 0.1;
		browser.update(now, beacon.port());
		beacon.update(someGame());
	}
	browser.update(now, beacon.port());
	CHECK(browser.found().size() == 1);

	// another game on this machine is another game
	LanAnnounce other = someGame();
	other.instance = 0x9ABCDEF0u;
	now += netlan::QUERY_SECONDS + 0.1;
	browser.update(now, beacon.port());
	beacon.update(other);
	for (int i = 0; (i < 2000) && (browser.found().size() < 2); i++) {
		browser.update(now, beacon.port());
		aMoment();
	}
	CHECK(browser.found().size() == 2);

	// whatever is still on its way from those: a query can get two answers, by broadcast and direct
	for (int i = 0; i < 50; i++) {
		browser.update(now, beacon.port());
		aMoment();
	}

	// silence
	now += netlan::FORGET_SECONDS + netlan::QUERY_SECONDS;
	browser.update(now, beacon.port());
	CHECK(browser.found().empty());
}

TEST_CASE("A beacon ignores what is not a query")
{
	netlan::Beacon beacon;
	REQUIRE(beacon.open(0));
	gos_net::LanSocket stranger;
	REQUIRE(stranger.open(0));
	gos_net::LanAddress to;
	to.ip = 0x7F000001u;
	to.port = beacon.port();
	const char junk[] = "not a query";
	REQUIRE(stranger.sendTo(to, junk, sizeof(junk)));
	std::vector<uint8_t> announce;
	REQUIRE(encode(someGame(), announce));
	REQUIRE(stranger.sendTo(to, announce.data(), announce.size()));
	for (int i = 0; i < 200; i++) {
		beacon.update(someGame());
		aMoment();
	}
	uint8_t buffer[512];
	size_t size = 0;
	gos_net::LanAddress from;
	CHECK_FALSE(stranger.receive(buffer, sizeof(buffer), size, from));
}

TEST_CASE("A flood does not hold a beacon or a browser for long")
{
	netlan::Beacon beacon;
	REQUIRE(beacon.open(0));
	gos_net::LanSocket stranger;
	REQUIRE(stranger.open(0));
	gos_net::LanAddress to;
	to.ip = 0x7F000001u;
	to.port = beacon.port();
	std::vector<uint8_t> query;
	REQUIRE(encode(LanQuery(), query));
	for (int i = 0; i < 100; i++)
		REQUIRE(stranger.sendTo(to, query.data(), query.size()));
	for (int i = 0; i < 20; i++)
		aMoment();
	beacon.update(someGame());
	for (int i = 0; i < 20; i++)
		aMoment();

	// one update answered some, not all of it
	uint8_t buffer[512];
	size_t size = 0;
	gos_net::LanAddress from;
	int answers = 0;
	while (stranger.receive(buffer, sizeof(buffer), size, from))
		answers++;
	CHECK(answers > 0);
	CHECK(answers < 100);
}

TEST_CASE("hashBytes: order and content count, and it runs on across pieces")
{
	const char whole[] = "abcdef";
	const uint32_t all = hashBytes(HASH_START, whole, 6);
	CHECK(all == hashBytes(hashBytes(HASH_START, whole, 2), whole + 2, 4));
	CHECK(all != hashBytes(HASH_START, "abcdeg", 6));
	CHECK(all != hashBytes(HASH_START, "bacdef", 6));
	// FNV-1a's own test value, so the result is the same on every platform and for good
	CHECK(hashBytes(HASH_START, "a", 1) == 0xE40C292Cu);
}

}
