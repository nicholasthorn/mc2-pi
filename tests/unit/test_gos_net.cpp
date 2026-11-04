// tests/unit/test_gos_net.cpp
//
// gos_net: what the rest of the game may assume of any transport, checked on the
// loopback backend. A real backend should pass the same behaviour tests.

#include "doctest.h"

#include "gos_net.h"

#include <cstdint>
#include <memory>
#include <vector>

using namespace gos_net;

static const uint16_t TEST_PORT = 28000;

// Connects a client to a listening host, and consumes the Connected events.
static ConnectionId connectPair(Transport& host, Transport& client)
{
	ConnectionId id = INVALID_CONNECTION;
	REQUIRE(client.connect("loopback", TEST_PORT, id));
	Event event;
	REQUIRE(client.poll(event));
	REQUIRE(event.type == EventType::Connected);
	REQUIRE(host.poll(event));
	REQUIRE(event.type == EventType::Connected);
	REQUIRE(event.connection == id);
	return id;
}

TEST_SUITE("gos_net") {

TEST_CASE("a client connects, and messages go both ways, in order")
{
	LoopbackNetwork net;
	std::unique_ptr<Transport> host = net.createTransport();
	std::unique_ptr<Transport> client = net.createTransport();
	REQUIRE(host->listen(TEST_PORT));
	const ConnectionId id = connectPair(*host, *client);
	CHECK(id != INVALID_CONNECTION);

	const uint8_t first[] = { 1, 2, 3 };
	const uint8_t second[] = { 4 };
	CHECK(client->send(id, Delivery::ReliableOrdered, first, sizeof(first)));
	CHECK(client->send(id, Delivery::ReliableUnordered, second, sizeof(second)));
	CHECK(host->send(id, Delivery::Unreliable, second, sizeof(second)));

	Event event;
	REQUIRE(host->poll(event));
	CHECK(event.type == EventType::Message);
	CHECK(event.connection == id);
	CHECK(event.delivery == Delivery::ReliableOrdered);
	CHECK(event.data == std::vector<uint8_t>({ 1, 2, 3 }));
	REQUIRE(host->poll(event));
	CHECK(event.delivery == Delivery::ReliableUnordered);
	CHECK(event.data == std::vector<uint8_t>({ 4 }));
	CHECK_FALSE(host->poll(event));

	REQUIRE(client->poll(event));
	CHECK(event.delivery == Delivery::Unreliable);
	CHECK(event.data == std::vector<uint8_t>({ 4 }));
	CHECK_FALSE(client->poll(event));
}

TEST_CASE("a host tells several clients apart")
{
	LoopbackNetwork net;
	std::unique_ptr<Transport> host = net.createTransport();
	std::unique_ptr<Transport> a = net.createTransport();
	std::unique_ptr<Transport> b = net.createTransport();
	REQUIRE(host->listen(TEST_PORT));
	const ConnectionId idA = connectPair(*host, *a);
	const ConnectionId idB = connectPair(*host, *b);
	CHECK(idA != idB);

	const uint8_t data = 9;
	REQUIRE(host->send(idB, Delivery::ReliableOrdered, &data, 1));
	Event event;
	CHECK_FALSE(a->poll(event));
	CHECK(b->poll(event));
}

TEST_CASE("connecting to nobody ends in Disconnected")
{
	LoopbackNetwork net;
	std::unique_ptr<Transport> client = net.createTransport();
	ConnectionId id = INVALID_CONNECTION;
	REQUIRE(client->connect("loopback", TEST_PORT, id));
	Event event;
	REQUIRE(client->poll(event));
	CHECK(event.type == EventType::Disconnected);
	CHECK(event.connection == id);
	const uint8_t data = 9;
	CHECK_FALSE(client->send(id, Delivery::ReliableOrdered, &data, 1));
}

TEST_CASE("a port can only be listened on once")
{
	LoopbackNetwork net;
	std::unique_ptr<Transport> a = net.createTransport();
	std::unique_ptr<Transport> b = net.createTransport();
	CHECK(a->listen(TEST_PORT));
	CHECK_FALSE(b->listen(TEST_PORT));
	a.reset();
	CHECK(b->listen(TEST_PORT));	// gone with its transport
}

TEST_CASE("send refuses what it cannot carry")
{
	LoopbackNetwork net;
	std::unique_ptr<Transport> host = net.createTransport();
	std::unique_ptr<Transport> client = net.createTransport();
	std::unique_ptr<Transport> stranger = net.createTransport();
	REQUIRE(host->listen(TEST_PORT));
	const ConnectionId id = connectPair(*host, *client);

	std::vector<uint8_t> big(MAX_MESSAGE_SIZE + 1);
	CHECK_FALSE(client->send(id, Delivery::ReliableOrdered, big.data(), big.size()));
	big.pop_back();
	CHECK(client->send(id, Delivery::ReliableOrdered, big.data(), big.size()));
	CHECK_FALSE(client->send(id + 100, Delivery::ReliableOrdered, big.data(), big.size()));
	// someone else's connection
	CHECK_FALSE(stranger->send(id, Delivery::ReliableOrdered, big.data(), big.size()));
	stranger->disconnect(id);
	CHECK(client->send(id, Delivery::ReliableOrdered, big.data(), big.size()));
}

TEST_CASE("disconnect tells the other end only")
{
	LoopbackNetwork net;
	std::unique_ptr<Transport> host = net.createTransport();
	std::unique_ptr<Transport> client = net.createTransport();
	REQUIRE(host->listen(TEST_PORT));
	const ConnectionId id = connectPair(*host, *client);

	client->disconnect(id);
	Event event;
	CHECK_FALSE(client->poll(event));
	REQUIRE(host->poll(event));
	CHECK(event.type == EventType::Disconnected);
	CHECK(event.connection == id);

	const uint8_t data = 9;
	CHECK_FALSE(host->send(id, Delivery::ReliableOrdered, &data, 1));
	client->disconnect(id);		// already gone: nothing happens
	CHECK_FALSE(host->poll(event));
}

TEST_CASE("nothing more arrives for a connection this end closed")
{
	LoopbackNetwork net;
	std::unique_ptr<Transport> host = net.createTransport();
	std::unique_ptr<Transport> a = net.createTransport();
	std::unique_ptr<Transport> b = net.createTransport();
	REQUIRE(host->listen(TEST_PORT));
	const ConnectionId idA = connectPair(*host, *a);
	const ConnectionId idB = connectPair(*host, *b);

	const uint8_t data = 9;
	REQUIRE(a->send(idA, Delivery::ReliableOrdered, &data, 1));
	REQUIRE(b->send(idB, Delivery::ReliableOrdered, &data, 1));
	host->disconnect(idA);

	// the other client's message is still there, and is the only one
	Event event;
	REQUIRE(host->poll(event));
	CHECK(event.connection == idB);
	CHECK_FALSE(host->poll(event));
}

TEST_CASE("a transport going away disconnects its peers, in either order")
{
	for (int hostFirst = 0; hostFirst < 2; hostFirst++) {
		CAPTURE(hostFirst);
		LoopbackNetwork net;
		std::unique_ptr<Transport> host = net.createTransport();
		std::unique_ptr<Transport> client = net.createTransport();
		REQUIRE(host->listen(TEST_PORT));
		const ConnectionId id = connectPair(*host, *client);

		std::unique_ptr<Transport>& leaving = hostFirst ? host : client;
		std::unique_ptr<Transport>& staying = hostFirst ? client : host;
		leaving.reset();
		Event event;
		REQUIRE(staying->poll(event));
		CHECK(event.type == EventType::Disconnected);
		CHECK(event.connection == id);
		staying.reset();
	}
}

TEST_CASE("a transport outlives its network object")
{
	std::unique_ptr<Transport> host;
	{
		LoopbackNetwork net;
		host = net.createTransport();
	}
	CHECK(host->listen(TEST_PORT));
}

TEST_CASE("only unreliable messages are dropped, and repeatably")
{
	const auto delivered = [](Delivery delivery, unsigned dropPercent) {
		LoopbackNetwork net(dropPercent, 42u);
		std::unique_ptr<Transport> host = net.createTransport();
		std::unique_ptr<Transport> client = net.createTransport();
		REQUIRE(host->listen(TEST_PORT));
		const ConnectionId id = connectPair(*host, *client);
		std::vector<int> arrived;
		for (int i = 0; i < 1000; i++) {
			const uint8_t data[2] = { static_cast<uint8_t>(i & 0xFF), static_cast<uint8_t>(i >> 8) };
			REQUIRE(client->send(id, delivery, data, sizeof(data)));
		}
		Event event;
		while (host->poll(event))
			arrived.push_back(event.data[0] | (event.data[1] << 8));
		return arrived;
	};

	CHECK(delivered(Delivery::Unreliable, 0).size() == 1000);
	CHECK(delivered(Delivery::ReliableOrdered, 100).size() == 1000);
	CHECK(delivered(Delivery::ReliableUnordered, 100).size() == 1000);
	CHECK(delivered(Delivery::Unreliable, 100).empty());

	const std::vector<int> once = delivered(Delivery::Unreliable, 20);
	CHECK(once.size() > 700);
	CHECK(once.size() < 900);
	CHECK(once == delivered(Delivery::Unreliable, 20));
}

}
