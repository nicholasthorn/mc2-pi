// tests/net/net_ping.cpp
//
// Two real processes over gos_net's real backend: a client sends every delivery type
// and a server echoes it. CTest runs both on one machine (two_process.cmake), and the
// same program checks a Quest over Wi-Fi by hand (scripts/android/net_ping.sh).
//
//   net_ping server <port> [seconds]
//   net_ping client <address> <port> [seconds]
//
// The server ends when its client leaves, and fails if none came in time. The client
// fails unless every reliable message comes back, the ordered ones in order, and at
// least one unreliable message does: on a real network some may be lost, and the
// count is printed.

#include "gos_net.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace gos_net;

namespace {

typedef std::chrono::steady_clock Clock;

const size_t MESSAGE_SIZE = 100;	// about a batch of mover updates

double secondsSince(Clock::time_point start)
{
	return std::chrono::duration<double>(Clock::now() - start).count();
}

void pause2ms()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(2));
}

int runServer(Transport& net, uint16_t port, int seconds)
{
	if (!net.listen(port)) {
		fprintf(stderr, "[server] cannot listen on %u\n", port);
		return 2;
	}
	fprintf(stderr, "[server] listening on %u\n", port);
	const Clock::time_point start = Clock::now();
	bool seen = false;
	while (secondsSince(start) < seconds) {
		Event event;
		if (!net.poll(event)) {
			pause2ms();
			continue;
		}
		if (event.type == EventType::Connected) {
			seen = true;
		}
		else if (event.type == EventType::Disconnected) {
			fprintf(stderr, "[server] client left\n");
			return 0;
		}
		else if (!net.send(event.connection, event.delivery, event.data.data(), event.data.size())) {
			fprintf(stderr, "[server] echo failed\n");
			return 1;
		}
	}
	fprintf(stderr, "[server] time up, client seen: %d\n", seen ? 1 : 0);
	return 1;
}

int percentile(std::vector<int> values, size_t percent)
{
	if (values.empty())
		return -1;
	std::sort(values.begin(), values.end());
	return values[std::min(values.size() - 1, values.size() * percent / 100)];
}

struct Message {
	uint32_t sequence;
	double sentAt;
};

int runClient(Transport& net, const char* address, uint16_t port, int seconds)
{
	ConnectionId connection = INVALID_CONNECTION;
	if (!net.connect(address, port, connection)) {
		printf("[client] bad address %s\n", address);
		return 2;
	}
	const Clock::time_point start = Clock::now();
	bool connected = false;
	double nextSend = 0.0;
	const uint32_t total = static_cast<uint32_t>(seconds) * 30;
	uint32_t sequence = 0;
	uint32_t sent[3] = { 0, 0, 0 }, received[3] = { 0, 0, 0 }, outOfOrder = 0;
	int64_t lastOrdered = -1;
	std::vector<int> roundTrips[3];

	for (;;) {
		const double now = secondsSince(start);
		Event event;
		while (net.poll(event)) {
			if (event.type == EventType::Connected) {
				connected = true;
				nextSend = now;
				printf("[client] connected after %d ms\n", static_cast<int>(now * 1000.0));
			}
			else if (event.type == EventType::Disconnected) {
				printf("[client] lost the connection\n");
				return 1;
			}
			else if (event.data.size() == MESSAGE_SIZE) {
				Message m;
				memcpy(&m, event.data.data(), sizeof(m));
				const int d = static_cast<int>(event.delivery);
				received[d]++;
				roundTrips[d].push_back(static_cast<int>((now - m.sentAt) * 1000.0));
				if (event.delivery == Delivery::ReliableOrdered) {
					if (static_cast<int64_t>(m.sequence) <= lastOrdered)
						outOfOrder++;
					lastOrdered = m.sequence;
				}
			}
		}
		if (!connected && now > 15.0) {
			printf("[client] nobody answered\n");
			return 1;
		}
		while (connected && sequence < total && now >= nextSend) {
			// mostly unreliable, as the game's traffic will be
			const Delivery delivery = (sequence % 6 == 0) ? Delivery::ReliableOrdered
				: (sequence % 6 == 3) ? Delivery::ReliableUnordered : Delivery::Unreliable;
			uint8_t data[MESSAGE_SIZE] = { 0 };
			const Message m = { sequence, now };
			memcpy(data, &m, sizeof(m));
			if (!net.send(connection, delivery, data, sizeof(data))) {
				printf("[client] send failed\n");
				return 1;
			}
			sent[static_cast<int>(delivery)]++;
			sequence++;
			nextSend += 1.0 / 30.0;
		}
		const bool allBack = sequence >= total && received[0] == sent[0] && received[1] == sent[1] && received[2] == sent[2];
		if (allBack || (sequence >= total && now > nextSend + 3.0))
			break;
		pause2ms();
	}

	static const char* const names[3] = { "reliable ordered", "reliable unordered", "unreliable" };
	for (int d = 0; d < 3; d++)
		printf("[result] %s %u/%u, round trip ms p50 %d p95 %d max %d\n", names[d], received[d], sent[d],
			percentile(roundTrips[d], 50), percentile(roundTrips[d], 95), percentile(roundTrips[d], 100));
	printf("[result] out of order %u\n", outOfOrder);
	net.disconnect(connection);
	std::this_thread::sleep_for(std::chrono::milliseconds(200));	// let the goodbye go
	const bool ok = received[0] == sent[0] && received[1] == sent[1] && outOfOrder == 0 && received[2] > 0;
	printf("[result] %s\n", ok ? "PASS" : "FAIL");
	return ok ? 0 : 1;
}

}

int main(int argc, char** argv)
{
	setvbuf(stdout, nullptr, _IOLBF, 1024);
	const std::string mode = argc > 1 ? argv[1] : "";
	const bool server = mode == "server" && argc >= 3;
	const bool client = mode == "client" && argc >= 4;
	if (!server && !client) {
		fprintf(stderr, "usage: net_ping server <port> [seconds] | net_ping client <address> <port> [seconds]\n");
		return 2;
	}
	std::string error;
	std::unique_ptr<Transport> net = createGnsTransport(&error);
	if (!net) {
		fprintf(stderr, "net_ping: %s\n", error.c_str());
		return 2;
	}
	const int secondsArg = server ? 3 : 4;
	const int seconds = argc > secondsArg ? atoi(argv[secondsArg]) : (server ? 60 : 5);
	if (server)
		return runServer(*net, static_cast<uint16_t>(atoi(argv[2])), seconds);
	return runClient(*net, argv[2], static_cast<uint16_t>(atoi(argv[3])), seconds);
}
