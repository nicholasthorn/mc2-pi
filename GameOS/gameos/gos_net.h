// GameOS/gameos/gos_net.h
//
// The network transport, behind a small interface so the choice of library can
// change (planning/future-features/NETWORKING.md, Decision 4): listen, connect,
// send with a delivery type, poll. It moves opaque messages; their format is
// code/netwire.h. Not thread-safe: use a transport from one thread.
//
// Two backends: GameNetworkingSockets, the real one, in builds with MC2_NET; and a
// loopback for tests, which delivers within one process and can drop unreliable
// messages to exercise the code above it.

#ifndef GOS_NET_H
#define GOS_NET_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace gos_net {

typedef uint32_t ConnectionId;
constexpr ConnectionId INVALID_CONNECTION = 0;

// Anything larger needs splitting by the caller. Every backend must carry this
// much in one message, for all delivery types. An Unreliable message over about
// 1200 bytes goes out in fragments and is lost if any one is, so keep those small.
constexpr size_t MAX_MESSAGE_SIZE = 8192;

enum class Delivery {
	ReliableOrdered,	// orders, hits, world events, the lobby
	// Weapon fire, radio. Not held up by ReliableOrdered traffic, and may not
	// assume any order: a backend is free to deliver these in order among themselves.
	ReliableUnordered,
	Unreliable			// mover updates, newest wins; may arrive out of order
};

enum class EventType {
	Connected,		// on both ends
	Disconnected,	// the other end went away, or never answered
	Message
};

struct Event {
	EventType type = EventType::Message;
	ConnectionId connection = INVALID_CONNECTION;
	Delivery delivery = Delivery::ReliableOrdered;	// of a Message
	std::vector<uint8_t> data;						// of a Message
};

class Transport {
public:
	virtual ~Transport() {}

	// Accept connections on this port. False if the port cannot be had, which may
	// be for a moment after whatever had it closed. Not reliable the other way: on
	// Windows the real backend has reported true for a port already taken.
	virtual bool listen(uint16_t port) = 0;

	// Start connecting. True means started and sets connection; a Connected or
	// Disconnected event for it says how it went.
	virtual bool connect(const char* address, uint16_t port, ConnectionId& connection) = 0;

	// False if the message cannot be queued: unknown connection, too large, or a send
	// queue already full (a slow network; a real backend holds about 512 kB). A true
	// does not promise an Unreliable message arrives.
	virtual bool send(ConnectionId connection, Delivery delivery, const void* data, size_t size) = 0;

	// Close from this end. The other end gets Disconnected; this end gets nothing
	// more for this connection, not even messages that had already arrived. What was
	// sent still gets there, if this process keeps a transport open while it does.
	virtual void disconnect(ConnectionId connection) = 0;

	// The next event, oldest first. False when there are none. A connection's Connected
	// comes before anything else about it. A received message is never over
	// MAX_MESSAGE_SIZE. Nothing limits how many wait here: poll until it returns false.
	virtual bool poll(Event& event) = 0;
};

// A little network in one process: its transports reach each other by port, and
// any address is accepted. The network and its transports may be destroyed in
// any order.
class LoopbackNetwork {
public:
	// dropPercent of Unreliable messages are lost, repeatably for a given seed.
	explicit LoopbackNetwork(unsigned dropPercent = 0, uint32_t seed = 1);
	std::unique_ptr<Transport> createTransport();

	// Every send on this connection now fails, as for a queue that is full.
	void failSends(ConnectionId connection);

	struct State;

private:
	std::shared_ptr<State> state_;
};

// The real network, over UDP: an IPv4 or IPv6 address, not a name. Anyone who can
// reach a listening port can connect, in any number: encrypted, but not authenticated,
// so limits and identity are the caller's until internet play. Returns null, with
// the reason in error if given, when the library will not start. connectTimeoutMs
// over 0 sets how long a connection may take to start, for every transport in the
// process. A build without MC2_NET always returns null. For testing, MC2_NET_LAG_MS and
// MC2_NET_LOSS_PERCENT in the environment make this end lag or lose what it sends.
std::unique_ptr<Transport> createGnsTransport(std::string* error = nullptr, int connectTimeoutMs = 0);

}

#endif
