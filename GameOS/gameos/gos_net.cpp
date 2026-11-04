// GameOS/gameos/gos_net.cpp: see gos_net.h

#include "gos_net.h"

#include <cstring>
#include <deque>
#include <map>

namespace gos_net {

namespace {

class LoopbackTransport;

struct Link {
	LoopbackTransport* ends[2];
};

}

struct LoopbackNetwork::State {
	unsigned dropPercent = 0;
	uint32_t random = 1;
	ConnectionId nextConnection = 1;
	std::map<uint16_t, LoopbackTransport*> listeners;
	std::map<ConnectionId, Link> links;

	bool dropThisOne()
	{
		random = random * 1664525u + 1013904223u;
		return ((random >> 8) % 100u) < dropPercent;
	}
};

namespace {

class LoopbackTransport : public Transport {
public:
	explicit LoopbackTransport(const std::shared_ptr<LoopbackNetwork::State>& state) : state_(state) {}

	~LoopbackTransport() override
	{
		for (auto it = state_->listeners.begin(); it != state_->listeners.end();) {
			if (it->second == this)
				it = state_->listeners.erase(it);
			else
				++it;
		}
		for (auto it = state_->links.begin(); it != state_->links.end();) {
			const Link& link = it->second;
			if (link.ends[0] == this || link.ends[1] == this) {
				LoopbackTransport* other = (link.ends[0] == this) ? link.ends[1] : link.ends[0];
				other->pushState(EventType::Disconnected, it->first);
				it = state_->links.erase(it);
			}
			else
				++it;
		}
	}

	bool listen(uint16_t port) override
	{
		if (state_->listeners.count(port))
			return false;
		state_->listeners[port] = this;
		return true;
	}

	bool connect(const char*, uint16_t port, ConnectionId& connection) override
	{
		connection = state_->nextConnection++;
		auto host = state_->listeners.find(port);
		if (host == state_->listeners.end()) {
			pushState(EventType::Disconnected, connection);
			return true;
		}
		Link link;
		link.ends[0] = this;
		link.ends[1] = host->second;
		state_->links[connection] = link;
		pushState(EventType::Connected, connection);
		host->second->pushState(EventType::Connected, connection);
		return true;
	}

	bool send(ConnectionId connection, Delivery delivery, const void* data, size_t size) override
	{
		auto it = state_->links.find(connection);
		if (it == state_->links.end() || size > MAX_MESSAGE_SIZE)
			return false;
		const Link& link = it->second;
		if (link.ends[0] != this && link.ends[1] != this)
			return false;
		if (delivery == Delivery::Unreliable && state_->dropThisOne())
			return true;
		Event event;
		event.type = EventType::Message;
		event.connection = connection;
		event.delivery = delivery;
		event.data.resize(size);
		if (size > 0)
			memcpy(event.data.data(), data, size);
		LoopbackTransport* other = (link.ends[0] == this) ? link.ends[1] : link.ends[0];
		other->events_.push_back(std::move(event));
		return true;
	}

	void disconnect(ConnectionId connection) override
	{
		auto it = state_->links.find(connection);
		if (it == state_->links.end())
			return;
		const Link& link = it->second;
		if (link.ends[0] != this && link.ends[1] != this)
			return;
		LoopbackTransport* other = (link.ends[0] == this) ? link.ends[1] : link.ends[0];
		other->pushState(EventType::Disconnected, connection);
		state_->links.erase(it);
	}

	bool poll(Event& event) override
	{
		if (events_.empty())
			return false;
		event = std::move(events_.front());
		events_.pop_front();
		return true;
	}

private:
	void pushState(EventType type, ConnectionId connection)
	{
		Event event;
		event.type = type;
		event.connection = connection;
		events_.push_back(std::move(event));
	}

	std::shared_ptr<LoopbackNetwork::State> state_;
	std::deque<Event> events_;
};

}

LoopbackNetwork::LoopbackNetwork(unsigned dropPercent, uint32_t seed) : state_(std::make_shared<State>())
{
	state_->dropPercent = dropPercent;
	state_->random = seed;
}

LoopbackNetwork::~LoopbackNetwork()
{
}

std::unique_ptr<Transport> LoopbackNetwork::createTransport()
{
	return std::unique_ptr<Transport>(new LoopbackTransport(state_));
}

}
