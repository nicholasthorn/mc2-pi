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
	choice.baseColor = netlobby::MAX_COLORS;
	CHECK_FALSE(lobby.applyChoice(1, choice));
	choice = readyOn(lobby, 1);
	choice.stripeColor = netlobby::MAX_COLORS;
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
	lobby.setSettings(600, 3000, LOBBY_AIRSTRIKE | LOBBY_MINELAYER);
	std::vector<uint8_t> out;
	REQUIRE(encode(lobby.state(), out));
	LobbyState state;
	REQUIRE(decode(out.data(), out.size(), state));
	CHECK(state.revision == lobby.state().revision);
	CHECK(state.maxPlayers == 4);
	CHECK(state.map == "mc2_m08");
	CHECK(state.timeLimit == 600);
	CHECK(state.resourcePoints == 3000);
	CHECK(state.flags == (LOBBY_AIRSTRIKE | LOBBY_MINELAYER));
	REQUIRE(state.players.size() == 2);
	CHECK(state.players[1].slot == 1);
	CHECK(state.players[1].name == "Ace");
	CHECK(state.players[1].team == lobby.player(1)->team);

	LobbyChoice choice;
	choice.team = 3;
	choice.baseColor = 4;
	choice.stripeColor = 5;
	choice.ready = true;
	choice.mapHash = MAP_HASH;
	REQUIRE(encode(choice, out));
	LobbyChoice backChoice;
	REQUIRE(decode(out.data(), out.size(), backChoice));
	CHECK(backChoice.team == 3);
	CHECK(backChoice.baseColor == 4);
	CHECK(backChoice.stripeColor == 5);
	CHECK(backChoice.ready);
	CHECK(backChoice.mapHash == MAP_HASH);

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
	state.players[1].baseColor = netlobby::MAX_COLORS;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.players[1].name = "100%";
	CHECK_FALSE(encode(state, out));
	state = good;
	state.map = "bad%map";
	CHECK_FALSE(encode(state, out));
	state = good;
	state.timeLimit = -2;
	CHECK_FALSE(encode(state, out));
	state = good;
	state.resourcePoints = -1;
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
