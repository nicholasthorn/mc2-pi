// tests/unit/test_gos_net_gns.cpp
//
// gos_net's real backend, GameNetworkingSockets, over 127.0.0.1 in one process: the
// behaviour test_gos_net.cpp asks of the loopback. Nothing leaves the machine. Waits
// are bounded and only long if something is broken; the two-process run is
// tests/net/two_process.cmake.

#ifdef MC2_NET

#include "doctest.h"

#include "gos_net.h"

#include <chrono>
#include <cstdint>
#include <memory>
#include <random>
#include <thread>
#include <vector>

using namespace gos_net;

namespace {

// A port of this run's own: WSL is one machine shared by every session, and another
// run's client could otherwise reach this run's server.
const uint16_t TEST_PORT = static_cast<uint16_t>(20000u + std::random_device()() % 20000u);
const char* const LOCALHOST = "127.0.0.1";

std::unique_ptr<Transport> create()
{
	std::unique_ptr<Transport> transport = createGnsTransport(nullptr, 500);
	REQUIRE(transport);
	return transport;
}

// GNS's own thread closes a socket a moment after its transport has gone, so a test
// that follows another may have to wait for the port.
void listenOnTestPort(Transport& transport)
{
	bool listening = false;
	for (int i = 0; i < 500 && !listening; i++) {
		listening = transport.listen(TEST_PORT);
		if (!listening)
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	REQUIRE(listening);
}

// The next event. GNS's own thread keeps the network going meanwhile, and any poll
// runs the callbacks for every transport in the process.
bool waitFor(Transport& transport, Event& event)
{
	for (int i = 0; i < 2000; i++) {
		if (transport.poll(event))
			return true;
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
	return false;
}

ConnectionId connectPair(Transport& host, Transport& client)
{
	ConnectionId clientSide = INVALID_CONNECTION;
	REQUIRE(client.connect(LOCALHOST, TEST_PORT, clientSide));
	Event event;
	REQUIRE(waitFor(host, event));
	REQUIRE(event.type == EventType::Connected);
	const ConnectionId hostSide = event.connection;
	REQUIRE(waitFor(client, event));
	REQUIRE(event.type == EventType::Connected);
	REQUIRE(event.connection == clientSide);
	CHECK(hostSide != clientSide);	// each end has its own id for a connection
	return clientSide;
}

}

TEST_SUITE("gos_net_gns") {

TEST_CASE("messages go both ways, with their delivery type")
{
	std::unique_ptr<Transport> host = create();
	std::unique_ptr<Transport> client = create();
	listenOnTestPort(*host);
	const ConnectionId toHost = connectPair(*host, *client);

	const uint8_t data[3] = { 1, 2, 3 };
	CHECK(client->send(toHost, Delivery::ReliableOrdered, data, sizeof(data)));
	CHECK(client->send(toHost, Delivery::ReliableUnordered, data, sizeof(data)));
	CHECK(client->send(toHost, Delivery::Unreliable, data, sizeof(data)));

	int ordered = 0, unordered = 0, unreliable = 0;
	ConnectionId toClient = INVALID_CONNECTION;
	Event event;
	for (int i = 0; i < 3; i++) {
		REQUIRE(waitFor(*host, event));
		REQUIRE(event.type == EventType::Message);
		CHECK(event.data == std::vector<uint8_t>({ 1, 2, 3 }));
		toClient = event.connection;
		ordered += event.delivery == Delivery::ReliableOrdered;
		unordered += event.delivery == Delivery::ReliableUnordered;
		unreliable += event.delivery == Delivery::Unreliable;
	}
	CHECK(ordered == 1);
	CHECK(unordered == 1);
	CHECK(unreliable == 1);

	const uint8_t back = 9;
	CHECK(host->send(toClient, Delivery::ReliableOrdered, &back, 1));
	REQUIRE(waitFor(*client, event));
	CHECK(event.connection == toHost);
	CHECK(event.data == std::vector<uint8_t>({ 9 }));
}

TEST_CASE("ordered messages stay in order, up to the largest size")
{
	std::unique_ptr<Transport> host = create();
	std::unique_ptr<Transport> client = create();
	listenOnTestPort(*host);
	const ConnectionId toHost = connectPair(*host, *client);

	// fewer than would fill the send queue (a full one makes send return false)
	const int count = 40;
	std::vector<uint8_t> big(MAX_MESSAGE_SIZE, 7);
	for (int i = 0; i < count; i++) {
		big[0] = static_cast<uint8_t>(i);
		REQUIRE(client->send(toHost, Delivery::ReliableOrdered, big.data(), big.size()));
	}
	Event event;
	for (int i = 0; i < count; i++) {
		REQUIRE(waitFor(*host, event));
		REQUIRE(event.type == EventType::Message);
		REQUIRE(event.data.size() == MAX_MESSAGE_SIZE);
		REQUIRE(event.data[0] == static_cast<uint8_t>(i));
	}
}

TEST_CASE("connecting to nobody ends in Disconnected")
{
	std::unique_ptr<Transport> client = create();
	ConnectionId id = INVALID_CONNECTION;
	REQUIRE(client->connect(LOCALHOST, TEST_PORT, id));
	Event event;
	REQUIRE(waitFor(*client, event));
	CHECK(event.type == EventType::Disconnected);
	CHECK(event.connection == id);
	const uint8_t data = 9;
	CHECK_FALSE(client->send(id, Delivery::ReliableOrdered, &data, 1));
}

TEST_CASE("a bad address or a taken port is refused")
{
	std::unique_ptr<Transport> a = create();
	std::unique_ptr<Transport> b = create();
	ConnectionId id = INVALID_CONNECTION;
	CHECK_FALSE(a->connect("not an address", TEST_PORT, id));
	listenOnTestPort(*a);
	CHECK_FALSE(b->listen(TEST_PORT));
	a.reset();
	listenOnTestPort(*b);	// gone with its transport
}

TEST_CASE("send refuses what it cannot carry")
{
	std::unique_ptr<Transport> host = create();
	std::unique_ptr<Transport> client = create();
	std::unique_ptr<Transport> stranger = create();
	listenOnTestPort(*host);
	const ConnectionId id = connectPair(*host, *client);

	std::vector<uint8_t> big(MAX_MESSAGE_SIZE + 1);
	CHECK_FALSE(client->send(id, Delivery::ReliableOrdered, big.data(), big.size()));
	CHECK_FALSE(client->send(id + 100, Delivery::ReliableOrdered, big.data(), 1));
	CHECK_FALSE(stranger->send(id, Delivery::ReliableOrdered, big.data(), 1));	// someone else's
	stranger->disconnect(id);
	CHECK(client->send(id, Delivery::ReliableOrdered, big.data(), 1));
}

TEST_CASE("disconnect tells the other end, after what was already sent")
{
	std::unique_ptr<Transport> host = create();
	std::unique_ptr<Transport> client = create();
	listenOnTestPort(*host);
	const ConnectionId id = connectPair(*host, *client);

	const uint8_t data = 9;
	REQUIRE(client->send(id, Delivery::ReliableOrdered, &data, 1));
	client->disconnect(id);

	Event event;
	CHECK_FALSE(client->poll(event));
	REQUIRE(waitFor(*host, event));
	CHECK(event.type == EventType::Message);
	REQUIRE(waitFor(*host, event));
	CHECK(event.type == EventType::Disconnected);
	CHECK_FALSE(client->send(id, Delivery::ReliableOrdered, &data, 1));
	client->disconnect(id);		// already gone: nothing happens
}

TEST_CASE("nothing more arrives for a connection this end closed")
{
	std::unique_ptr<Transport> host = create();
	std::unique_ptr<Transport> client = create();
	listenOnTestPort(*host);
	const ConnectionId toHost = connectPair(*host, *client);

	const uint8_t data = 9;
	REQUIRE(client->send(toHost, Delivery::ReliableOrdered, &data, 1));
	REQUIRE(client->send(toHost, Delivery::ReliableOrdered, &data, 1));
	Event event;
	REQUIRE(waitFor(*host, event));		// the first, which names the connection
	host->disconnect(event.connection);
	for (int i = 0; i < 25; i++) {
		CHECK_FALSE(host->poll(event));
		std::this_thread::sleep_for(std::chrono::milliseconds(2));
	}
}

TEST_CASE("a transport going away disconnects its peers, in either order")
{
	for (int hostFirst = 0; hostFirst < 2; hostFirst++) {
		CAPTURE(hostFirst);
		std::unique_ptr<Transport> host = create();
		std::unique_ptr<Transport> client = create();
		listenOnTestPort(*host);
		connectPair(*host, *client);

		std::unique_ptr<Transport>& leaving = hostFirst ? host : client;
		std::unique_ptr<Transport>& staying = hostFirst ? client : host;
		leaving.reset();
		Event event;
		REQUIRE(waitFor(*staying, event));
		CHECK(event.type == EventType::Disconnected);
		staying.reset();
	}
}

TEST_CASE("the library starts again after the last transport has gone")
{
	create().reset();
	std::unique_ptr<Transport> again = create();
	CHECK(again->listen(TEST_PORT + 1));
}

}

#endif
