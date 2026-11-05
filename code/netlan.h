// code/netlan.h
//
// Finding games on the local network. A host's Beacon listens on a known port and answers
// a query with what it is hosting; a Browser asks everyone, and keeps a list of who
// answered. No GL, SDL or game dependency. What comes in is untrusted: a datagram that
// does not decode is ignored, and the list has a limit whoever is answering.

#ifndef NETLAN_H
#define NETLAN_H

#include "gos_net_lan.h"
#include "netwire.h"

#include <cstdint>
#include <string>
#include <vector>

namespace netlan {

constexpr uint16_t DISCOVERY_PORT = 27816;
constexpr size_t MAX_FOUND = 32;
constexpr double QUERY_SECONDS = 1.0;	// how often a Browser asks
constexpr double FORGET_SECONDS = 4.0;	// and how long a host that stops answering stays listed

class Beacon {
public:
	// False if the port is taken, say by another host on this machine. A host still works
	// without a beacon: it is reached by its address.
	bool open(uint16_t port = DISCOVERY_PORT) { return socket_.open(port); }
	bool isOpen() const { return socket_.isOpen(); }
	uint16_t port() const { return socket_.port(); }

	// Answers whatever has asked. Call often.
	void update(const netwire::LanAnnounce& announce);

private:
	gos_net::LanSocket socket_;
};

struct Found {
	std::string address;
	netwire::LanAnnounce announce;
	uint32_t ip = 0;
	double lastSeen = 0.0;
};

class Browser {
public:
	bool open() { return socket_.open(0); }

	// The time is the caller's, in seconds: it asks, listens, and forgets by it.
	void update(double now, uint16_t discoveryPort = DISCOVERY_PORT);

	const std::vector<Found>& found() const { return found_; }

private:
	gos_net::LanSocket socket_;
	std::vector<Found> found_;
	double nextQuery_ = 0.0;
};

}

#endif
