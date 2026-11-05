// tests/unit/test_netsession.cpp
//
// netsession: joining, the load barrier and starting, on the loopback transport,
// including clients that break the order. Run under ASan + UBSan by CI.

#include "doctest.h"

#include "netsession.h"

#include <cstdint>
#include <memory>
#include <vector>

using namespace netsession;
using namespace netwire;

namespace {

const uint16_t PORT = 1234;
const uint32_t HASH = 0x1234ABCDu;
const uint32_t ROSTER = 0x55AA55AAu;

struct Game {
	gos_net::LoopbackNetwork network;
	std::unique_ptr<gos_net::Transport> hostTransport = network.createTransport();
	Session host{*hostTransport};
	std::vector<std::unique_ptr<gos_net::Transport>> transports;
	std::vector<std::unique_ptr<Session>> clients;

	Session& addClient(uint32_t hash = HASH)
	{
		transports.push_back(network.createTransport());
		clients.emplace_back(new Session(*transports.back()));
		REQUIRE(clients.back()->join("127.0.0.1", PORT, hash, "Ace"));
		return *clients.back();
	}

	void pump()
	{
		for (int i = 0; i < 8; i++) {
			host.update();
			for (auto& client : clients)
				client->update();
		}
	}
};

ChunkBatch orderBatch()
{
	ChunkBatch batch;
	batch.kind = ChunkKind::Order;
	batch.subject = 3;
	batch.words = { 1, 2 };
	return batch;
}

}

TEST_SUITE("netsession") {

TEST_CASE("two clients join, load, start, and exchange batches")
{
	Game game;
	REQUIRE(game.host.host(PORT, 3, HASH));
	CHECK_FALSE(game.host.allJoined());

	Session& a = game.addClient();
	Session& b = game.addClient();
	game.pump();
	CHECK(game.host.allJoined());
	CHECK(a.joined());
	CHECK(b.joined());
	CHECK(a.commander() == 1);
	CHECK(b.commander() == 2);
	CHECK(a.numPlayers() == 3);

	game.host.setRosterHash(ROSTER);
	CHECK_FALSE(game.host.allReady());
	CHECK_FALSE(game.host.start());

	CHECK(a.sendReady(ROSTER));
	game.pump();
	CHECK_FALSE(game.host.allReady());
	CHECK(b.sendReady(ROSTER));
	game.pump();
	CHECK(game.host.allReady());
	CHECK(game.host.start());
	game.pump();
	CHECK(a.started());
	CHECK(b.started());

	CHECK(b.sendToHost(gos_net::Delivery::ReliableOrdered, orderBatch()));
	game.pump();
	Received received;
	REQUIRE(game.host.nextBatch(received));
	CHECK(received.commander == 2);
	CHECK(received.batch.subject == 3);
	CHECK_FALSE(game.host.nextBatch(received));

	ChunkBatch state;
	state.kind = ChunkKind::Move;
	state.words = { 9 };
	CHECK(game.host.sendToClients(gos_net::Delivery::Unreliable, state));
	game.pump();
	REQUIRE(a.nextBatch(received));
	CHECK(received.batch.words == state.words);
	REQUIRE(b.nextBatch(received));

	game.host.endGame(1);
	game.pump();
	CHECK(a.gameOver());
	CHECK(a.winningTeam() == 1);
}

TEST_CASE("a one-player host has nobody to wait for")
{
	Game game;
	REQUIRE(game.host.host(PORT, 1, HASH));
	CHECK(game.host.allJoined());
	game.host.setRosterHash(ROSTER);
	CHECK(game.host.allReady());
	CHECK(game.host.start());
}

TEST_CASE("the host can dial a client that waits")
{
	Game game;
	const uint16_t clientPort = PORT + 1;
	REQUIRE(game.host.host(PORT, 2, HASH));
	game.transports.push_back(game.network.createTransport());
	game.clients.emplace_back(new Session(*game.transports.back()));
	Session& a = *game.clients.back();
	REQUIRE(a.joinWaiting(clientPort, HASH, "Ace"));
	game.pump();
	CHECK_FALSE(a.joined());

	CHECK(game.host.dialClient("127.0.0.1", clientPort));
	game.pump();
	CHECK(a.joined());
	CHECK(a.commander() == 1);
	CHECK(game.host.allJoined());

	game.host.setRosterHash(ROSTER);
	CHECK(a.sendReady(ROSTER));
	game.pump();
	CHECK(game.host.start());
	game.pump();
	CHECK(a.started());

	Received received;
	ChunkBatch order = orderBatch();
	CHECK(a.sendToHost(gos_net::Delivery::ReliableOrdered, order));
	game.pump();
	REQUIRE(game.host.nextBatch(received));
	CHECK(received.commander == 1);
}

TEST_CASE("a client that waits takes only the first to arrive as the host")
{
	Game game;
	const uint16_t clientPort = PORT + 1;
	game.transports.push_back(game.network.createTransport());
	game.clients.emplace_back(new Session(*game.transports.back()));
	Session& a = *game.clients.back();
	REQUIRE(a.joinWaiting(clientPort, HASH, "Ace"));

	REQUIRE(game.host.host(PORT, 2, HASH));
	CHECK(game.host.dialClient("127.0.0.1", clientPort));
	game.pump();
	CHECK(a.joined());

	// a second caller says nothing the client listens to
	auto other = game.network.createTransport();
	gos_net::ConnectionId connection = gos_net::INVALID_CONNECTION;
	REQUIRE(other->connect("127.0.0.1", clientPort, connection));
	game.pump();
	Start start;
	std::vector<uint8_t> out;
	REQUIRE(encode(start, out));
	other->send(connection, gos_net::Delivery::ReliableOrdered, out.data(), out.size());
	game.pump();
	CHECK_FALSE(a.started());
}

TEST_CASE("only a host can dial")
{
	Game game;
	CHECK_FALSE(game.host.dialClient("127.0.0.1", PORT));
}

TEST_CASE("the wrong data is refused")
{
	Game game;
	REQUIRE(game.host.host(PORT, 2, HASH));
	Session& a = game.addClient(HASH + 1);
	game.pump();
	CHECK_FALSE(a.joined());
	CHECK(a.rejected() == RejectReason::WrongData);
	CHECK_FALSE(game.host.allJoined());
}

TEST_CASE("a full game is refused, and a place opens when someone leaves before the start")
{
	Game game;
	REQUIRE(game.host.host(PORT, 2, HASH));
	Session& a = game.addClient();
	game.pump();
	CHECK(a.joined());
	Session& b = game.addClient();
	game.pump();
	CHECK(b.rejected() == RejectReason::GameFull);

	game.clients.erase(game.clients.begin());
	game.transports.erase(game.transports.begin());
	game.pump();
	CHECK_FALSE(game.host.allJoined());
	Session& c = game.addClient();
	game.pump();
	CHECK(c.joined());
	CHECK(c.commander() == 1);
}

TEST_CASE("a client can finish loading before the host has")
{
	Game game;
	REQUIRE(game.host.host(PORT, 3, HASH));
	Session& a = game.addClient();
	game.pump();
	CHECK(a.sendReady(ROSTER));
	game.pump();	// the host is still waiting for the third player
	CHECK(game.host.connectedClients() == 1);

	Session& b = game.addClient();
	game.pump();
	CHECK(b.sendReady(ROSTER + 1));
	game.pump();
	CHECK(game.host.allJoined());
	CHECK_FALSE(game.host.allReady());

	// only now does the host know what it loaded: a is right, and b is not
	game.host.setRosterHash(ROSTER);
	game.pump();
	CHECK(b.rejected() == RejectReason::WrongRoster);
	CHECK(game.host.connectedClients() == 1);
	CHECK(a.rejected() == RejectReason::None);
}

TEST_CASE("other units are refused at the barrier")
{
	Game game;
	REQUIRE(game.host.host(PORT, 2, HASH));
	Session& a = game.addClient();
	game.pump();
	game.host.setRosterHash(ROSTER);
	CHECK(a.sendReady(ROSTER + 1));
	game.pump();
	CHECK(a.rejected() == RejectReason::WrongRoster);
	CHECK_FALSE(game.host.allReady());
}

TEST_CASE("a client that breaks the order is dropped, and the host survives")
{
	Game game;
	REQUIRE(game.host.host(PORT, 2, HASH));
	auto transport = game.network.createTransport();
	gos_net::ConnectionId connection = gos_net::INVALID_CONNECTION;
	REQUIRE(transport->connect("127.0.0.1", PORT, connection));

	SUBCASE("a batch before Hello")
	{
		std::vector<uint8_t> out;
		REQUIRE(encode(orderBatch(), out));
		CHECK(transport->send(connection, gos_net::Delivery::ReliableOrdered, out.data(), out.size()));
	}
	SUBCASE("garbage")
	{
		const uint8_t junk[3] = { 200, 1, 2 };
		CHECK(transport->send(connection, gos_net::Delivery::Unreliable, junk, sizeof(junk)));
	}
	SUBCASE("a message from a client that only a host sends")
	{
		Start start;
		std::vector<uint8_t> out;
		REQUIRE(encode(start, out));
		CHECK(transport->send(connection, gos_net::Delivery::ReliableOrdered, out.data(), out.size()));
	}
	game.pump();
	gos_net::Event event;
	bool sawDisconnect = false;
	while (transport->poll(event))
		sawDisconnect = sawDisconnect || event.type == gos_net::EventType::Disconnected;
	game.host.update();
	CHECK(sawDisconnect);
	CHECK(game.host.connectedClients() == 0);
}

TEST_CASE("a client may only send orders, and only after the start")
{
	Game game;
	REQUIRE(game.host.host(PORT, 2, HASH));
	Session& a = game.addClient();
	game.pump();
	game.host.setRosterHash(ROSTER);
	CHECK(a.sendReady(ROSTER));
	game.pump();
	CHECK(game.host.start());
	game.pump();

	ChunkBatch forged;
	forged.kind = ChunkKind::WeaponHit;
	CHECK(a.sendToHost(gos_net::Delivery::ReliableOrdered, forged));
	game.pump();
	Received received;
	CHECK_FALSE(game.host.nextBatch(received));
	CHECK(game.host.connectedClients() == 0);
	CHECK(a.hostLost());
}

// Anything from a peer, at any stage, must neither crash a session nor reach the game
// as anything but a batch that decoded.
TEST_CASE("garbage from a peer never crashes a session")
{
	uint32_t seed = 4242u;
	const auto next = [&seed]() {
		seed = seed * 1664525u + 1013904223u;
		return seed >> 8;
	};
	std::vector<uint8_t> bytes;
	for (int round = 0; round < 400; round++) {
		Game game;
		REQUIRE(game.host.host(PORT, 2, HASH));
		game.host.setRosterHash(ROSTER);

		// a real client, or one that has reached a later stage: its session is what is attacked
		Session& a = game.addClient();
		game.pump();
		if (round % 2) {
			a.sendReady(ROSTER);
			game.pump();
			game.host.start();
			game.pump();
		}
		// a second connection throwing junk at the host
		auto attacker = game.network.createTransport();
		gos_net::ConnectionId connection = gos_net::INVALID_CONNECTION;
		REQUIRE(attacker->connect("127.0.0.1", PORT, connection));
		game.pump();
		for (int i = 0; i < 16; i++) {
			bytes.resize(next() % 48u);
			for (uint8_t& b : bytes)
				b = static_cast<uint8_t>(next());
			if (!bytes.empty() && (next() & 1u))
				bytes[0] = static_cast<uint8_t>(1u + next() % 6u);
			attacker->send(connection, gos_net::Delivery::ReliableOrdered, bytes.data(), bytes.size());
			game.pump();
		}
		Received received;
		while (game.host.nextBatch(received))
			CHECK(received.batch.kind == ChunkKind::Order);
		// the real client is unharmed unless it took the attacker's place, which it cannot
		CHECK_FALSE(a.hostLost());
	}
}

TEST_CASE("ranks: higher is better, level players share, and the next skips")
{
	const long standings[4] = { 10, 30, 10, 5 };
	long ranks[4] = {};
	rankPlayers(standings, 4, ranks);
	CHECK(ranks[0] == 2);
	CHECK(ranks[1] == 1);
	CHECK(ranks[2] == 2);
	CHECK(ranks[3] == 4);
}

TEST_CASE("a client takes no batches after the game is over")
{
	Game game;
	REQUIRE(game.host.host(PORT, 2, HASH));
	Session& a = game.addClient();
	game.pump();
	game.host.setRosterHash(ROSTER);
	CHECK(a.sendReady(ROSTER));
	game.pump();
	CHECK(game.host.start());
	game.pump();

	ChunkBatch batch;
	batch.kind = ChunkKind::World;
	batch.words = { 1, 2 };
	CHECK(game.host.sendToClients(gos_net::Delivery::ReliableOrdered, batch));
	game.host.endGame(0);
	CHECK(game.host.sendToClients(gos_net::Delivery::ReliableOrdered, batch));
	game.pump();

	Received received;
	CHECK(a.nextBatch(received));	// the one before the end
	CHECK_FALSE(a.nextBatch(received));
	CHECK(a.gameOver());
}

TEST_CASE("a client hears when the host goes")
{
	Game game;
	REQUIRE(game.host.host(PORT, 2, HASH));
	Session& a = game.addClient();
	game.pump();
	CHECK(a.joined());
	game.hostTransport.reset();
	for (int i = 0; i < 8; i++)
		a.update();
	CHECK(a.hostLost());
}

}
