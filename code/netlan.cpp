// code/netlan.cpp: see netlan.h

#include "netlan.h"

#include <algorithm>

namespace netlan {

namespace {
// larger than any valid message, so that one cut short by the socket cannot decode
constexpr size_t BUFFER_SIZE = 512;
constexpr uint32_t LOOPBACK = 0x7F000001u;
// each frame: anyone can send to these ports, and a flood must not stall the game
constexpr int MAX_DATAGRAMS_PER_UPDATE = 32;
}

void Beacon::update(const netwire::LanAnnounce& announce)
{
	uint8_t buffer[BUFFER_SIZE];
	size_t size = 0;
	gos_net::LanAddress from;
	std::vector<uint8_t> answer;
	for (int i = 0; (i < MAX_DATAGRAMS_PER_UPDATE) && socket_.receive(buffer, sizeof(buffer), size, from); i++) {
		netwire::LanQuery query;
		if (!netwire::decode(buffer, size, query))
			continue;
		// whatever the version it asks with: the announce says which this one is
		if (answer.empty() && !netwire::encode(announce, answer))
			return;
		socket_.sendTo(from, answer.data(), answer.size());
	}
}

void Browser::update(double now, uint16_t discoveryPort)
{
	if (now >= nextQuery_) {
		nextQuery_ = now + QUERY_SECONDS;
		std::vector<uint8_t> query;
		if (netwire::encode(netwire::LanQuery(), query))
			socket_.broadcast(discoveryPort, query.data(), query.size());
	}

	uint8_t buffer[BUFFER_SIZE];
	size_t size = 0;
	gos_net::LanAddress from;
	for (int i = 0; (i < MAX_DATAGRAMS_PER_UPDATE) && socket_.receive(buffer, sizeof(buffer), size, from); i++) {
		netwire::LanAnnounce announce;
		if (!netwire::decode(buffer, size, announce))
			continue;
		// a host on this machine answers twice, by broadcast and by loopback: one game
		auto known = std::find_if(found_.begin(), found_.end(), [&](const Found& f) {
			return f.announce.instance == announce.instance;
		});
		if (known == found_.end()) {
			if (found_.size() >= MAX_FOUND)
				continue;
			found_.push_back(Found());
			known = found_.end() - 1;
		}
		// its network address if it has one, which others could use too
		if ((known->ip == 0) || (known->ip == LOOPBACK)) {
			known->ip = from.ip;
			known->address = gos_net::LanSocket::addressText(from.ip);
		}
		known->announce = announce;
		known->lastSeen = now;
	}

	found_.erase(std::remove_if(found_.begin(), found_.end(), [&](const Found& f) {
		return now - f.lastSeen > FORGET_SECONDS;
	}), found_.end());
}

}
