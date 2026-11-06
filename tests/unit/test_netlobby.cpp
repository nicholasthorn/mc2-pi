// tests/unit/test_netlobby.cpp
//
// The lobby: its rules with no network, its messages, and the session's lobby mode on the
// loopback transport, including what a client should not be able to do.

#include "doctest.h"

#include "netlobby.h"
#include "netsession.h"

#include <memory>
#include <vector>

using namespace netwire;
using netlobby::Lobby;

namespace {

const uint32_t MAP_HASH = 0xAAAA5555u;
const uint32_t DATA_HASH = 0x1234ABCDu;
const uint16_t PORT = 1234;

LobbyChoice readyOn(const Lobby& lobby, uint8_t slot, uint32_t mapHash = MAP_HASH)
{
	const LobbyPlayer* player = lobby.player(slot);
	REQUIRE(player);
	LobbyChoice choice;
	choice.revision = lobby.state().revision;
	choice.team = player->team;
	choice.baseColor = player->baseColor;
	choice.stripeColor = player->stripeColor;
	choice.ready = true;
	choice.mapHash = mapHash;
	return choice;
}

Lobby twoPlayers()
{
	Lobby lobby(4);
	REQUIRE(lobby.addPlayer(0, "Host"));
	REQUIRE(lobby.addPlayer(1, "Ace"));
	lobby.setMap("mc2_m08", MAP_HASH);
	return lobby;
}

}

TEST_SUITE("Lobby") {

TEST_CASE("Lobby: a player arrives on a team and in a colour nobody has")
{
	Lobby lobby(4);
	REQUIRE(lobby.addPlayer(0, "Host"));
	REQUIRE(lobby.addPlayer(1, "Ace"));
	REQUIRE(lobby.addPlayer(2, "Bo"));
	CHECK(lobby.player(0)->team != lobby.player(1)->team);
	CHECK(lobby.player(1)->team != lobby.player(2)->team);
	CHECK(lobby.player(0)->baseColor != lobby.player(1)->baseColor);
	CHECK(lobby.player(1)->baseColor != lobby.player(2)->baseColor);
	CHECK_FALSE(lobby.player(1)->ready);
	CHECK(lobby.player(3) == nullptr);
}

TEST_CASE("Lobby: a place is taken once, and frees when its player leaves")
{
	Lobby lobby(2);
	REQUIRE(lobby.addPlayer(0, "Host"));
	REQUIRE(lobby.addPlayer(1, "Ace"));
	CHECK_FALSE(lobby.addPlayer(1, "Twin"));
	CHECK_FALSE(lobby.addPlayer(2, "Past the limit"));
	lobby.removePlayer(1);
	CHECK(lobby.player(1) == nullptr);
	CHECK(lobby.state().players.size() == 1);
	CHECK(lobby.addPlayer(1, "Bo"));
	CHECK(lobby.player(1)->name == "Bo");
}

TEST_CASE("Lobby: the same choice again is no change, so a client answering each state cannot loop")
{
	Lobby lobby = twoPlayers();
	REQUIRE(lobby.applyChoice(1, readyOn(lobby, 1, MAP_HASH + 1)));	// another map: not ready
	const uint32_t last = lobby.state().revision;
	CHECK(lobby.applyChoice(1, readyOn(lobby, 1, MAP_HASH + 1)));
	CHECK(lobby.state().revision == last);
	CHECK(lobby.applyChoice(1, readyOn(lobby, 1)));
	CHECK(lobby.state().revision != last);
}

TEST_CASE("Lobby: every change is a new revision, for a screen to notice")
{
	Lobby lobby = twoPlayers();
	uint32_t last = lobby.state().revision;
	lobby.setMap("mc2_m05", MAP_HASH);
	CHECK(lobby.state().revision != last);
	last = lobby.state().revision;
	lobby.setSettings(600, 1000, 0);
	CHECK(lobby.state().revision != last);
	last = lobby.state().revision;
	CHECK(lobby.applyChoice(1, readyOn(lobby, 1)));
	CHECK(lobby.state().revision != last);
	last = lobby.state().revision;
	CHECK_FALSE(lobby.addPlayer(1, "Twin"));
	CHECK(lobby.state().revision == last);
}

TEST_CASE("Lobby: what no screen offers is refused, and changes nothing")
{
	Lobby lobby = twoPlayers();
	const LobbyPlayer before = *lobby.player(1);
	LobbyChoice choice = readyOn(lobby, 1);

	choice.team = MAX_PLAYERS;
	CHECK_FALSE(lobby.applyChoice(1, choice));
	choice = readyOn(lobby, 1);
	choice.baseColor = MAX_LOBBY_COLORS;
	CHECK_FALSE(lobby.applyChoice(1, choice));
	choice = readyOn(lobby, 1);
	choice.stripeColor = MAX_LOBBY_COLORS;
	CHECK_FALSE(lobby.applyChoice(1, choice));
	choice = readyOn(lobby, 1);
	choice.baseColor = lobby.player(0)->baseColor;	// somebody's
	CHECK_FALSE(lobby.applyChoice(1, choice));
	CHECK_FALSE(lobby.applyChoice(3, readyOn(lobby, 1)));	// nobody's there

	CHECK(lobby.player(1)->team == before.team);
	CHECK(lobby.player(1)->baseColor == before.baseColor);
	CHECK(lobby.player(1)->stripeColor == before.stripeColor);
	CHECK_FALSE(lobby.player(1)->ready);
}

TEST_CASE("Lobby: nobody can be ready on a map they do not have")
{
	Lobby lobby = twoPlayers();
	CHECK(lobby.applyChoice(1, readyOn(lobby, 1, MAP_HASH + 1)));
	CHECK_FALSE(lobby.player(1)->ready);
	CHECK(lobby.applyChoice(1, readyOn(lobby, 1)));
	CHECK(lobby.player(1)->ready);
}

TEST_CASE("Lobby: changing the map or the settings takes everyone's ready back")
{
	Lobby lobby = twoPlayers();
	REQUIRE(lobby.applyChoice(1, readyOn(lobby, 1)));
	lobby.setSettings(600, 1000, 0);
	CHECK_FALSE(lobby.player(1)->ready);

	REQUIRE(lobby.applyChoice(1, readyOn(lobby, 1)));
	lobby.setMap("mc2_m05", MAP_HASH + 1);
	CHECK_FALSE(lobby.player(1)->ready);
	// a ready from before the change is for the old map
	CHECK(lobby.applyChoice(1, readyOn(lobby, 1)));
	CHECK_FALSE(lobby.player(1)->ready);
}

TEST_CASE("Lobby: a ready on its way as the settings change does not count for the new ones")
{
	Lobby lobby = twoPlayers();
	const LobbyChoice sent = readyOn(lobby, 1);	// answering the state as it was
	lobby.setSettings(600, 1000, 0);
	CHECK(lobby.applyChoice(1, sent));
	CHECK_FALSE(lobby.player(1)->ready);
	// the answer to the new state does
	CHECK(lobby.applyChoice(1, readyOn(lobby, 1)));
	CHECK(lobby.player(1)->ready);
	// and a change of someone else's keeps it, as they agreed to the same map and settings
	REQUIRE(lobby.addPlayer(2, "Bo"));
	CHECK(lobby.applyChoice(2, readyOn(lobby, 2)));
	CHECK(lobby.player(1)->ready);
	CHECK(lobby.player(2)->ready);
}

TEST_CASE("Lobby: a map or settings that no state could carry are refused, and change nothing")
{
	Lobby lobby = twoPlayers();
	REQUIRE(lobby.applyChoice(1, readyOn(lobby, 1)));
	const uint32_t last = lobby.state().revision;
	CHECK_FALSE(lobby.setMap("../../secret", MAP_HASH));
	CHECK_FALSE(lobby.setMap("100%", MAP_HASH));
	CHECK_FALSE(lobby.setSettings(-2, 0, 0));
	CHECK_FALSE(lobby.setSettings(600, -1, 0));
	CHECK(lobby.state().map == "mc2_m08");
	CHECK(lobby.state().revision == last);
	CHECK(lobby.player(1)->ready);
	std::vector<uint8_t> out;
	CHECK(encode(lobby.state(), out));
}

TEST_CASE("Lobby: a choice can rename its player; no name leaves it as it is")
{
	Lobby lobby = twoPlayers();
	LobbyChoice choice = readyOn(lobby, 1);
	CHECK(lobby.applyChoice(1, choice));
	CHECK(lobby.player(1)->name == "Ace");

	uint32_t last = lobby.state().revision;
	choice.name = "Bo";
	CHECK(lobby.applyChoice(1, choice));
	CHECK(lobby.player(1)->name == "Bo");
	CHECK(lobby.state().revision != last);

	last = lobby.state().revision;
	CHECK(lobby.applyChoice(1, choice));	// the same again
	CHECK(lobby.state().revision == last);

	choice.name = "100%";	// no state could carry it
	CHECK_FALSE(lobby.applyChoice(1, choice));
	CHECK(lobby.player(1)->name == "Bo");
}

TEST_CASE("Lobby: nobody is on a team past what the map takes")
{
	Lobby lobby(4);
	REQUIRE(lobby.addPlayer(0, "Host"));
	REQUIRE(lobby.addPlayer(1, "Ace"));
	REQUIRE(lobby.addPlayer(2, "Bo"));
	CHECK(lobby.player(2)->team == 2);

	// a map for two puts the third on the last
	REQUIRE(lobby.setMap("mc2_m08", MAP_HASH, 2));
	CHECK(lobby.player(0)->team == 0);
	CHECK(lobby.player(1)->team == 1);
	CHECK(lobby.player(2)->team == 1);

	LobbyChoice choice = readyOn(lobby, 2);
	choice.team = 2;
	CHECK_FALSE(lobby.applyChoice(2, choice));
	CHECK(lobby.player(2)->team == 1);

	// and whoever joins now
	REQUIRE(lobby.addPlayer(3, "Cy"));
	CHECK(lobby.player(3)->team == 1);

	// a map that says nothing sensible cannot empty the list
	REQUIRE(lobby.setMap("mc2_m05", MAP_HASH, 0));
	CHECK(lobby.player(1)->team == 0);
	REQUIRE(lobby.setMap("mc2_m05", MAP_HASH, 200));
	choice = readyOn(lobby, 2);
	choice.team = MAX_PLAYERS - 1;
	CHECK(lobby.applyChoice(2, choice));
}

TEST_CASE("Lobby: a game can start with a map, two sides, and everyone ready")
{
	Lobby lobby(4);
	std::string why;
	REQUIRE(lobby.addPlayer(0, "Host"));
	CHECK_FALSE(lobby.canStart(&why));
	CHECK_FALSE(why.empty());

	REQUIRE(lobby.addPlayer(1, "Ace"));
	CHECK_FALSE(lobby.canStart());	// no map
	lobby.setMap("mc2_m08", MAP_HASH);
	CHECK_FALSE(lobby.canStart());	// not ready
	REQUIRE(lobby.applyChoice(1, readyOn(lobby, 1)));
	CHECK(lobby.canStart());

	// everyone on one side: nobody to fight
	LobbyChoice sameTeam = readyOn(lobby, 1);
	sameTeam.team = lobby.player(0)->team;
	REQUIRE(lobby.applyChoice(1, sameTeam));
	CHECK_FALSE(lobby.canStart(&why));
	sameTeam.team = lobby.player(0)->team + 1;
	REQUIRE(lobby.applyChoice(1, sameTeam));
	CHECK(lobby.canStart());

	// someone new is not ready yet
	REQUIRE(lobby.addPlayer(2, "Bo"));
	CHECK_FALSE(lobby.canStart());
	lobby.removePlayer(2);
	CHECK(lobby.canStart());
}

TEST_CASE("Lobby: commanders run from 0 with no gaps, whoever left")
{
	Lobby lobby(4);
	REQUIRE(lobby.addPlayer(0, "Host"));
	REQUIRE(lobby.addPlayer(1, "Ace"));
	REQUIRE(lobby.addPlayer(2, "Bo"));
	REQUIRE(lobby.addPlayer(3, "Cy"));
	lobby.removePlayer(1);
	uint8_t commanders[MAX_PLAYERS];
	CHECK(lobby.commanders(commanders) == 3);
	CHECK(commanders[0] == 0);
	CHECK(commanders[1] == NO_COMMANDER);
	CHECK(commanders[2] == 1);
	CHECK(commanders[3] == 2);
}

TEST_CASE("LobbyState, LobbyChoice and LobbyLaunch: round trips")
{
	Lobby lobby = twoPlayers();
	lobby.setSettings(600, 3000, LOBBY_AIRSTRIKE | LOBBY_MINELAYER | LOBBY_QUICKSTART, 250000, 180);
	std::vector<uint8_t> out;
	REQUIRE(encode(lobby.state(), out));
	LobbyState state;
	REQUIRE(decode(out.data(), out.size(), state));
	CHECK(state.revision == lobby.state().revision);
	CHECK(state.maxPlayers == 4);
	CHECK(state.map == "mc2_m08");
	CHECK(state.mapHash == MAP_HASH);
	CHECK(state.timeLimit == 600);
	CHECK(state.resourcePoints == 3000);
	CHECK(state.flags == (LOBBY_AIRSTRIKE | LOBBY_MINELAYER | LOBBY_QUICKSTART));
	CHECK(state.cBills == 250000);
	CHECK(state.dropWeight == 180);
	REQUIRE(state.players.size() == 2);
	CHECK(state.players[1].slot == 1);
	CHECK(state.players[1].name == "Ace");
	CHECK(state.players[1].team == lobby.player(1)->team);

	LobbyChoice choice;
	choice.revision = 77;
	choice.team = 3;
	choice.baseColor = 4;
	choice.stripeColor = 5;
	choice.ready = true;
	choice.mapHash = MAP_HASH;
	choice.name = "Bo";
	REQUIRE(encode(choice, out));
	LobbyChoice backChoice;
	REQUIRE(decode(out.data(), out.size(), backChoice));
	CHECK(backChoice.revision == 77);
	CHECK(backChoice.team == 3);
	CHECK(backChoice.baseColor == 4);
	CHECK(backChoice.stripeColor == 5);
	CHECK(backChoice.ready);
	CHECK(backChoice.mapHash == MAP_HASH);
	CHECK(backChoice.name == "Bo");

	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[0] = 0;
	launch.commanders[3] = 1;
	REQUIRE(encode(launch, out));
	LobbyLaunch backLaunch;
	REQUIRE(decode(out.data(), out.size(), backLaunch));
	CHECK(backLaunch.numPlayers == 2);
	CHECK(backLaunch.commanders[0] == 0);
	CHECK(backLaunch.commanders[1] == NO_COMMANDER);
	CHECK(backLaunch.commanders[3] == 1);
}

TEST_CASE("Lobby messages: what no game sends is refused, whole or cut short")
{
	std::vector<uint8_t> out;
	Lobby lobby = twoPlayers();
	const LobbyState good = lobby.state();
	LobbyState state;

	REQUIRE(encode(good, out));
	for (size_t size = 0; size < out.size(); size++)
		CHECK_FALSE(decode(out.data(), size, state));
	std::vector<uint8_t> longer = out;
	longer.push_back(0);
	CHECK_FALSE(decode(longer.data(), longer.size(), state));

	state = good;
	state.players[1].slot = state.players[0].slot;	// one place twice
	CHECK_FALSE(encode(state, out));
	state = good;
	state.players[1].slot = MAX_PLAYERS;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.players[1].team = MAX_PLAYERS;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.players[1].baseColor = MAX_LOBBY_COLORS;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.players[1].name = "100%";
	CHECK_FALSE(encode(state, out));
	state = good;
	state.map = "bad%map";
	CHECK_FALSE(encode(state, out));
	state = good;
	state.map = "../../secret";
	CHECK_FALSE(encode(state, out));
	state = good;
	state.timeLimit = -2;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.resourcePoints = -1;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.cBills = -1;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.dropWeight = -1;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.flags = LOBBY_ALL_FLAGS + 1u;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.maxPlayers = 1;	// fewer than are in it
	CHECK_FALSE(encode(state, out));

	LobbyChoice choice;
	choice.team = MAX_PLAYERS;
	CHECK_FALSE(encode(choice, out));

	// two with one commander, a commander past the count, or too few
	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[0] = 0;
	launch.commanders[1] = 0;
	CHECK_FALSE(encode(launch, out));
	launch.commanders[1] = 2;
	CHECK_FALSE(encode(launch, out));
	launch.commanders[1] = NO_COMMANDER;
	CHECK_FALSE(encode(launch, out));
	launch.numPlayers = 0;
	launch.commanders[0] = NO_COMMANDER;
	CHECK_FALSE(encode(launch, out));
}

}

// ------------------------------------------------------------------------- on the session

namespace {

struct LobbyGame {
	gos_net::LoopbackNetwork network;
	std::unique_ptr<gos_net::Transport> hostTransport = network.createTransport();
	netsession::Session host{*hostTransport};
	std::vector<std::unique_ptr<gos_net::Transport>> transports;
	std::vector<std::unique_ptr<netsession::Session>> clients;

	LobbyGame(uint8_t maxPlayers = 4)
	{
		REQUIRE(host.hostLobby(PORT, maxPlayers, DATA_HASH));
	}

	// the map and who is in, which a launch needs a client to have heard
	void sayState(const std::vector<uint8_t>& slots)
	{
		Lobby lobby(4);
		for (uint8_t slot : slots)
			REQUIRE(lobby.addPlayer(slot, "Someone"));
		REQUIRE(lobby.setMap("mc2_m08", MAP_HASH));
		REQUIRE(host.sendLobbyState(lobby.state()));
		pump();
	}

	netsession::Session& addClient(const char* name = "Ace", uint32_t hash = DATA_HASH)
	{
		transports.push_back(network.createTransport());
		clients.emplace_back(new netsession::Session(*transports.back()));
		REQUIRE(clients.back()->join("127.0.0.1", PORT, hash, name));
		pump();
		return *clients.back();
	}

	void pump()
	{
		for (int i = 0; i < 8; i++) {
			host.update();
			for (auto& client : clients)
				if (client)
					client->update();
		}
	}
};

}

TEST_SUITE("LobbySession") {

TEST_CASE("Host hears who joins, with their names, and who leaves")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	CHECK(ace.joined());
	CHECK(ace.commander() == 1);

	netsession::LobbyEvent event;
	REQUIRE(game.host.nextLobbyEvent(event));
	CHECK(event.type == netsession::LobbyEventType::Joined);
	CHECK(event.commander == 1);
	CHECK(event.name == "Ace");
	CHECK_FALSE(game.host.nextLobbyEvent(event));

	game.transports[0]->disconnect(ace.serverConnection());
	game.clients[0].reset();
	game.pump();
	REQUIRE(game.host.nextLobbyEvent(event));
	CHECK(event.type == netsession::LobbyEventType::Left);
	CHECK(event.commander == 1);
}

TEST_CASE("A client sees the state the host sends, and the host sees its choices")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient();
	CHECK_FALSE(ace.hasLobbyState());

	Lobby lobby(4);
	REQUIRE(lobby.addPlayer(0, "Host"));
	REQUIRE(lobby.addPlayer(1, "Ace"));
	lobby.setMap("mc2_m08", MAP_HASH);
	REQUIRE(game.host.sendLobbyState(lobby.state()));
	game.pump();
	REQUIRE(ace.hasLobbyState());
	CHECK(ace.lobbyState().map == "mc2_m08");
	CHECK(ace.lobbyState().players.size() == 2);

	netsession::LobbyEvent event;
	REQUIRE(game.host.nextLobbyEvent(event));	// the join
	LobbyChoice choice = readyOn(lobby, 1);
	REQUIRE(ace.sendLobbyChoice(choice));
	game.pump();
	REQUIRE(game.host.nextLobbyEvent(event));
	CHECK(event.type == netsession::LobbyEventType::Choice);
	CHECK(event.commander == 1);
	CHECK(event.choice.ready);
	CHECK(event.choice.mapHash == MAP_HASH);
}

TEST_CASE("Launch renumbers around whoever left, and the game goes on from there")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	netsession::Session& bo = game.addClient("Bo");
	CHECK(ace.commander() == 1);
	CHECK(bo.commander() == 2);

	// Ace leaves: Bo is commander 1 in the game
	game.transports[0]->disconnect(ace.serverConnection());
	game.clients[0].reset();
	game.pump();
	game.sayState({ 0, 2 });

	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[0] = 0;
	launch.commanders[2] = 1;
	REQUIRE(game.host.launch(launch));
	CHECK(game.host.numPlayers() == 2);
	game.pump();
	CHECK(bo.launched());
	CHECK(bo.commander() == 1);
	CHECK(bo.numPlayers() == 2);
	CHECK(bo.lobbyLaunch().commanders[2] == 1);

	// the load barrier and start, as without a lobby
	game.host.setRosterHash(0x77u);
	REQUIRE(bo.sendReady(0x77u));
	game.pump();
	CHECK(game.host.allJoined());
	CHECK(game.host.allReady());
	REQUIRE(game.host.start());
	game.pump();
	CHECK(bo.started());

	// its orders come in under its commander in the game
	ChunkBatch batch;
	batch.kind = ChunkKind::Order;
	batch.words = { 1u };
	REQUIRE(bo.sendToHost(gos_net::Delivery::ReliableOrdered, batch));
	game.pump();
	netsession::Received received;
	REQUIRE(game.host.nextBatch(received));
	CHECK(received.commander == 1);
}

TEST_CASE("Someone who leaves while loading is not waited for")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	netsession::Session& bo = game.addClient("Bo");
	game.sayState({ 0, 1, 2 });
	LobbyLaunch launch;
	launch.numPlayers = 3;
	launch.commanders[0] = 0;
	launch.commanders[1] = 1;
	launch.commanders[2] = 2;
	REQUIRE(game.host.launch(launch));
	game.pump();
	REQUIRE(bo.launched());

	game.host.setRosterHash(0x77u);
	REQUIRE(bo.sendReady(0x77u));
	game.pump();
	CHECK_FALSE(game.host.allReady());	// Ace is still loading

	game.transports[0]->disconnect(ace.serverConnection());
	game.clients[0].reset();
	game.pump();
	CHECK(game.host.allReady());
	REQUIRE(game.host.start());
	game.pump();
	CHECK(bo.started());
}

TEST_CASE("If everyone else leaves while loading, there is nobody to play")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	game.sayState({ 0, 1 });
	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[0] = 0;
	launch.commanders[1] = 1;
	REQUIRE(game.host.launch(launch));
	game.pump();
	CHECK_FALSE(game.host.allLeft());

	game.transports[0]->disconnect(ace.serverConnection());
	game.clients[0].reset();
	game.pump();
	CHECK(game.host.allLeft());
}

TEST_CASE("Nobody has left a lobby that has not launched, or a game with one player")
{
	LobbyGame game;
	CHECK_FALSE(game.host.allLeft());
	LobbyLaunch launch;
	launch.numPlayers = 1;
	launch.commanders[0] = 0;
	REQUIRE(game.host.launch(launch));
	CHECK_FALSE(game.host.allLeft());
}

TEST_CASE("A launch that one client cannot be told of goes ahead for the others")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	netsession::Session& bo = game.addClient("Bo");
	game.sayState({ 0, 1, 2 });
	game.network.failSends(ace.serverConnection());

	LobbyLaunch launch;
	launch.numPlayers = 3;
	launch.commanders[0] = 0;
	launch.commanders[1] = 1;
	launch.commanders[2] = 2;
	REQUIRE(game.host.launch(launch));
	game.pump();
	CHECK(game.host.connectedClients() == 1);
	CHECK(game.host.numPlayers() == 3);
	CHECK(bo.launched());
	CHECK(bo.commander() == 2);
	CHECK_FALSE(ace.launched());

	// and Bo is not left waiting for Ace
	game.host.setRosterHash(0x77u);
	REQUIRE(bo.sendReady(0x77u));
	game.pump();
	CHECK(game.host.allReady());
	game.host.start();
	game.pump();
	CHECK(bo.started());
}

TEST_CASE("A client does not follow a launch other than what the state said")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	game.addClient("Bo");
	game.sayState({ 0, 1, 2 });	// three in the state, and two in the launch: Bo is left out
	game.transports[1]->disconnect(game.clients[1]->serverConnection());
	game.clients[1].reset();
	game.pump();

	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[0] = 0;
	launch.commanders[1] = 1;
	REQUIRE(game.host.launch(launch));
	game.pump();
	CHECK_FALSE(ace.launched());
}

TEST_CASE("A client does not follow a launch that makes it the host")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	game.sayState({ 0, 1 });
	// what the host's own launch() refuses, from a host that does not
	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[0] = 1;
	launch.commanders[1] = 0;
	std::vector<uint8_t> out;
	REQUIRE(encode(launch, out));
	REQUIRE(game.hostTransport->send(ace.serverConnection(), gos_net::Delivery::ReliableOrdered, out.data(), out.size()));
	game.pump();
	CHECK_FALSE(ace.launched());
}

TEST_CASE("A launch with no map heard is no launch")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[0] = 0;
	launch.commanders[1] = 1;
	REQUIRE(game.host.launch(launch));
	game.pump();
	CHECK_FALSE(ace.launched());
}

TEST_CASE("After launch nobody else gets in")
{
	LobbyGame game;
	game.addClient("Ace");
	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[0] = 0;
	launch.commanders[1] = 1;
	REQUIRE(game.host.launch(launch));
	netsession::Session& late = game.addClient("Late");
	CHECK_FALSE(late.joined());
	CHECK(late.rejected() == RejectReason::GameFull);
}

TEST_CASE("A lobby's limit holds, and a place comes free")
{
	LobbyGame game(2);
	netsession::Session& ace = game.addClient("Ace");
	CHECK(ace.joined());
	netsession::Session& third = game.addClient("Third");
	CHECK(third.rejected() == RejectReason::GameFull);
}

TEST_CASE("A launch that leaves out the host, or names a client twice, is not sent")
{
	LobbyGame game;
	game.addClient("Ace");
	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[1] = 0;	// the host has none
	launch.commanders[2] = 1;
	CHECK_FALSE(game.host.launch(launch));
	LobbyLaunch hostFirst;
	hostFirst.numPlayers = 2;
	hostFirst.commanders[0] = 1;	// the host is always 0
	hostFirst.commanders[1] = 0;
	CHECK_FALSE(game.host.launch(hostFirst));
}

TEST_CASE("A client cannot send what only a host may send")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient();
	// what its own socket could send, whatever its code says: a state, a launch
	std::vector<uint8_t> out;
	Lobby lobby(4);
	REQUIRE(lobby.addPlayer(0, "Host"));
	REQUIRE(encode(lobby.state(), out));
	gos_net::ConnectionId server = ace.serverConnection();
	REQUIRE(game.transports[0]->send(server, gos_net::Delivery::ReliableOrdered, out.data(), out.size()));
	game.pump();
	CHECK(ace.hostLost());
	CHECK(game.host.connectedClients() == 0);
}

TEST_CASE("A bad choice from a client drops it")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient();
	const uint8_t junk[] = { static_cast<uint8_t>(MessageType::LobbyChoice), 200, 0, 0, 0 };
	REQUIRE(game.transports[0]->send(ace.serverConnection(), gos_net::Delivery::ReliableOrdered, junk, sizeof(junk)));
	game.pump();
	CHECK(game.host.connectedClients() == 0);

	// and the lobby hears of it: a player it kept would stop any game starting
	netsession::LobbyEvent event;
	REQUIRE(game.host.nextLobbyEvent(event));
	CHECK(event.type == netsession::LobbyEventType::Joined);
	REQUIRE(game.host.nextLobbyEvent(event));
	CHECK(event.type == netsession::LobbyEventType::Left);
	CHECK(event.commander == 1);
}

TEST_CASE("Without a lobby, lobby messages are not a thing a client may send")
{
	gos_net::LoopbackNetwork network;
	std::unique_ptr<gos_net::Transport> hostTransport = network.createTransport();
	std::unique_ptr<gos_net::Transport> clientTransport = network.createTransport();
	netsession::Session host(*hostTransport);
	netsession::Session client(*clientTransport);
	REQUIRE(host.host(PORT, 2, DATA_HASH));
	REQUIRE(client.join("127.0.0.1", PORT, DATA_HASH, "Ace"));
	for (int i = 0; i < 8; i++) {
		host.update();
		client.update();
	}
	REQUIRE(client.joined());
	CHECK_FALSE(client.sendLobbyChoice(LobbyChoice()));	// its own code says no
	std::vector<uint8_t> out;
	REQUIRE(encode(LobbyChoice(), out));
	REQUIRE(clientTransport->send(client.serverConnection(), gos_net::Delivery::ReliableOrdered, out.data(), out.size()));
	for (int i = 0; i < 8; i++) {
		host.update();
		client.update();
	}
	CHECK(host.connectedClients() == 0);
}

namespace {

Force forceOf(uint8_t commander, const char* pilot)
{
	Force force;
	force.commander = commander;
	ForceUnit unit;
	unit.chassis = "data\\objects\\atlas.csv";
	unit.pilot = pilot;
	force.units.push_back(unit);
	return force;
}

// a lobby of the host and two clients, launched
void launchThree(LobbyGame& game)
{
	game.sayState({ 0, 1, 2 });
	LobbyLaunch launch;
	launch.numPlayers = 3;
	launch.commanders[0] = 0;
	launch.commanders[1] = 1;
	launch.commanders[2] = 2;
	REQUIRE(game.host.launch(launch));
	game.pump();
}

}

TEST_CASE("Forces: each client sends its own, and the host tells everyone everyone's")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	netsession::Session& bo = game.addClient("Bo");
	Force out;
	CHECK_FALSE(ace.sendForce(forceOf(1, "pmw00031")));	// not before the launch
	launchThree(game);
	REQUIRE(ace.launched());

	REQUIRE(ace.sendForce(forceOf(1, "pmw00031")));
	CHECK_FALSE(ace.sendForce(forceOf(1, "pmw00031")));	// once
	CHECK_FALSE(bo.sendForce(forceOf(1, "pmw00032")));	// not another's
	REQUIRE(bo.sendForce(forceOf(2, "pmw00032")));
	game.pump();
	REQUIRE(game.host.nextForce(out));
	CHECK(out.commander == 1);
	CHECK(out.units[0].pilot == "pmw00031");
	REQUIRE(game.host.nextForce(out));
	CHECK(out.commander == 2);
	CHECK_FALSE(game.host.nextForce(out));

	CHECK_FALSE(ace.hasForce(0));
	REQUIRE(game.host.sendForce(forceOf(0, "pmw00040")));
	REQUIRE(game.host.sendForce(forceOf(1, "pmw00031")));
	REQUIRE(game.host.sendForce(forceOf(2, "pmw00032")));
	game.pump();
	for (netsession::Session* client : { &ace, &bo }) {
		REQUIRE(client->hasForce(0));
		REQUIRE(client->hasForce(2));
		CHECK(client->force(0).units[0].pilot == "pmw00040");
		CHECK(client->force(1).units[0].pilot == "pmw00031");
		CHECK_FALSE(client->hasForce(3));
		CHECK(client->force(3).units.empty());
	}

	// the host holds what it said, and its first word stands: it says no other, and a client
	// would take no other
	CHECK(game.host.hasForce(2));
	CHECK(game.host.force(0).units[0].pilot == "pmw00040");
	CHECK_FALSE(game.host.sendForce(forceOf(0, "pmw00099")));
	std::vector<uint8_t> again;
	REQUIRE(encode(forceOf(0, "pmw00099"), again));
	REQUIRE(game.hostTransport->send(ace.serverConnection(), gos_net::Delivery::ReliableOrdered, again.data(), again.size()));
	game.pump();
	CHECK(ace.force(0).units[0].pilot == "pmw00040");

	// and none after the start
	game.host.setRosterHash(0x77u);
	REQUIRE(ace.sendReady(0x77u));
	REQUIRE(bo.sendReady(0x77u));
	game.pump();
	REQUIRE(game.host.start());
	CHECK_FALSE(game.host.sendForce(forceOf(0, "pmw00040")));
}

TEST_CASE("Forces: a client that sends another's, or a second, by its own socket is dropped")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	netsession::Session& bo = game.addClient("Bo");
	launchThree(game);
	REQUIRE(bo.launched());

	std::vector<uint8_t> bytes;
	REQUIRE(encode(forceOf(2, "pmw00031"), bytes));	// Bo's, from Ace
	REQUIRE(game.transports[0]->send(ace.serverConnection(), gos_net::Delivery::ReliableOrdered, bytes.data(), bytes.size()));
	game.pump();
	CHECK_FALSE(game.host.clientConnected(1));
	CHECK(game.host.clientConnected(2));

	REQUIRE(bo.sendForce(forceOf(2, "pmw00032")));
	game.pump();
	REQUIRE(game.transports[1]->send(bo.serverConnection(), gos_net::Delivery::ReliableOrdered, bytes.data(), bytes.size()));
	game.pump();
	CHECK_FALSE(game.host.clientConnected(2));
	Force out;
	REQUIRE(game.host.nextForce(out));	// the first was taken
	CHECK(out.commander == 2);
}

TEST_CASE("Forces: a client cannot send one before a launch, or speak for the host")
{
	LobbyGame game;
	netsession::Session& ace = game.addClient("Ace");
	std::vector<uint8_t> bytes;
	REQUIRE(encode(forceOf(1, "pmw00031"), bytes));
	REQUIRE(game.transports[0]->send(ace.serverConnection(), gos_net::Delivery::ReliableOrdered, bytes.data(), bytes.size()));
	game.pump();
	CHECK(game.host.connectedClients() == 0);

	// and a host's force for a commander past the game is not taken
	LobbyGame other;
	netsession::Session& bo = other.addClient("Bo");
	other.sayState({ 0, 1 });
	LobbyLaunch launch;
	launch.numPlayers = 2;
	launch.commanders[0] = 0;
	launch.commanders[1] = 1;
	REQUIRE(other.host.launch(launch));
	other.pump();
	CHECK_FALSE(other.host.sendForce(forceOf(2, "pmw00031")));
	REQUIRE(encode(forceOf(5, "pmw00031"), bytes));
	REQUIRE(other.hostTransport->send(bo.serverConnection(), gos_net::Delivery::ReliableOrdered, bytes.data(), bytes.size()));
	other.pump();
	CHECK_FALSE(bo.hasForce(5));
}

TEST_CASE("A lobby-less game plays as it did: no events pile up")
{
	gos_net::LoopbackNetwork network;
	std::unique_ptr<gos_net::Transport> hostTransport = network.createTransport();
	std::unique_ptr<gos_net::Transport> clientTransport = network.createTransport();
	netsession::Session host(*hostTransport);
	netsession::Session client(*clientTransport);
	REQUIRE(host.host(PORT, 2, DATA_HASH));
	REQUIRE(client.join("127.0.0.1", PORT, DATA_HASH, "Ace"));
	for (int i = 0; i < 8; i++) {
		host.update();
		client.update();
	}
	netsession::LobbyEvent event;
	CHECK_FALSE(host.nextLobbyEvent(event));
	CHECK(host.allJoined());
}

}
