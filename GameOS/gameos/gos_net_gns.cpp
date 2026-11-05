// GameOS/gameos/gos_net_gns.cpp: the GameNetworkingSockets backend of gos_net.h.
//
// GNS has one global interface and reports connection changes through one global
// callback from RunCallbacks, so a small registry finds the transport each belongs
// to. There is normally one transport; tests run a host and a client in a process.

#include "gos_net.h"

#include <steam/isteamnetworkingutils.h>
#include <steam/steamnetworkingsockets.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <set>
#include <utility>

namespace gos_net {

namespace {

class GnsTransport;

std::map<HSteamListenSocket, GnsTransport*> g_listeners;
std::map<HSteamNetConnection, GnsTransport*> g_connections;
int g_transports = 0;

// Lane 0 carries ordered reliable traffic and unreliable traffic. Lane 1 holds
// ReliableUnordered: GNS has no unordered reliable delivery, but a lane of its own
// gives what the delivery type promises, that a lost packet in one does not hold up
// the other.
const int NUM_LANES = 2;
const int LANE_UNORDERED = 1;

void onStatusChanged(SteamNetConnectionStatusChangedCallback_t* info);

// Errors only: its warnings include every few milliseconds waited for its lock.
void logMessage(ESteamNetworkingSocketsDebugOutputType, const char* message)
{
	printf("[NET] %s\n", message);
}

class GnsTransport : public Transport {
public:
	GnsTransport()
	{
		group_ = SteamNetworkingSockets()->CreatePollGroup();
	}

	~GnsTransport() override
	{
		ISteamNetworkingSockets* net = SteamNetworkingSockets();
		for (ConnectionId connection : connections_) {
			g_connections.erase(connection);
			net->CloseConnection(connection, 0, nullptr, false);
		}
		for (HSteamListenSocket socket : listeners_) {
			g_listeners.erase(socket);
			net->CloseListenSocket(socket);
		}
		net->DestroyPollGroup(group_);
		// GNS shuts down with the last transport
		if (--g_transports == 0)
			GameNetworkingSockets_Kill();
	}

	bool listen(uint16_t port) override
	{
		SteamNetworkingIPAddr address;
		address.Clear();
		address.m_port = port;
		const HSteamListenSocket socket = SteamNetworkingSockets()->CreateListenSocketIP(address, 0, nullptr);
		if (socket == k_HSteamListenSocket_Invalid)
			return false;
		listeners_.insert(socket);
		g_listeners[socket] = this;
		return true;
	}

	bool connect(const char* address, uint16_t port, ConnectionId& connection) override
	{
		SteamNetworkingIPAddr to;
		to.Clear();
		if (!to.ParseString(address))
			return false;
		to.m_port = port;
		const HSteamNetConnection c = SteamNetworkingSockets()->ConnectByIPAddress(to, 0, nullptr);
		if (c == k_HSteamNetConnection_Invalid)
			return false;
		adopt(c);
		connection = c;
		return true;
	}

	bool send(ConnectionId connection, Delivery delivery, const void* data, size_t size) override
	{
		if (size > MAX_MESSAGE_SIZE || !connections_.count(connection))
			return false;
		SteamNetworkingMessage_t* message = SteamNetworkingUtils()->AllocateMessage(static_cast<int>(size));
		if (size > 0)
			memcpy(message->m_pData, data, size);
		message->m_conn = connection;
		// a caller sends a tick's worth at once, so holding it back gains nothing
		message->m_nFlags = (delivery == Delivery::Unreliable) ? k_nSteamNetworkingSend_UnreliableNoNagle : k_nSteamNetworkingSend_ReliableNoNagle;
		message->m_idxLane = (delivery == Delivery::ReliableUnordered) ? LANE_UNORDERED : 0;
		int64 result = 0;
		SteamNetworkingSockets()->SendMessages(1, &message, &result, true);	// frees it on failure
		return result > 0;
	}

	void disconnect(ConnectionId connection) override
	{
		if (!connections_.erase(connection))
			return;
		incoming_.erase(connection);
		connecting_.erase(connection);
		dropHeld(connection);
		g_connections.erase(connection);
		// linger: what was already sent still gets there
		SteamNetworkingSockets()->CloseConnection(connection, 0, nullptr, true);
		events_.erase(std::remove_if(events_.begin(), events_.end(),
			[connection](const Event& e) { return e.connection == connection; }), events_.end());
	}

	bool poll(Event& event) override
	{
		receive();
		SteamNetworkingSockets()->RunCallbacks();
		receive();
		if (events_.empty())
			return false;
		event = std::move(events_.front());
		events_.pop_front();
		return true;
	}

	void onConnecting(HSteamNetConnection connection)
	{
		if (SteamNetworkingSockets()->AcceptConnection(connection) != k_EResultOK) {
			SteamNetworkingSockets()->CloseConnection(connection, 0, nullptr, false);
			return;
		}
		adopt(connection);
		incoming_.insert(connection);
	}

	void onConnected(HSteamNetConnection connection)
	{
		incoming_.erase(connection);
		connecting_.erase(connection);
		pushState(EventType::Connected, connection);
		for (Event& held : held_)
			if (held.connection == connection)
				events_.push_back(std::move(held));
		dropHeld(connection);
	}

	void onLost(HSteamNetConnection connection)
	{
		// what arrived before it went comes first, and must be read before closing
		receive();
		connections_.erase(connection);
		g_connections.erase(connection);
		SteamNetworkingSockets()->CloseConnection(connection, 0, nullptr, false);
		connecting_.erase(connection);
		dropHeld(connection);
		// one that never finished connecting to us was never announced to the caller
		if (!incoming_.erase(connection))
			pushState(EventType::Disconnected, connection);
	}

private:
	void adopt(HSteamNetConnection connection)
	{
		ISteamNetworkingSockets* net = SteamNetworkingSockets();
		net->SetConnectionPollGroup(connection, group_);
		net->ConfigureConnectionLanes(connection, NUM_LANES, nullptr, nullptr);
		connections_.insert(connection);
		connecting_.insert(connection);
		g_connections[connection] = this;
	}

	void dropHeld(ConnectionId connection)
	{
		held_.erase(std::remove_if(held_.begin(), held_.end(),
			[connection](const Event& e) { return e.connection == connection; }), held_.end());
	}

	void receive()
	{
		SteamNetworkingMessage_t* messages[32];
		for (;;) {
			const int count = SteamNetworkingSockets()->ReceiveMessagesOnPollGroup(group_, messages, 32);
			for (int i = 0; i < count; i++) {
				SteamNetworkingMessage_t* m = messages[i];
				// GNS would carry far more than the interface promises, and any peer can ask
				if (m->m_cbSize > static_cast<int>(MAX_MESSAGE_SIZE)) {
					m->Release();
					continue;
				}
				Event event;
				event.type = EventType::Message;
				event.connection = m->m_conn;
				if (!(m->m_nFlags & k_nSteamNetworkingSend_Reliable))
					event.delivery = Delivery::Unreliable;
				else
					event.delivery = (m->m_idxLane == LANE_UNORDERED) ? Delivery::ReliableUnordered : Delivery::ReliableOrdered;
				const uint8_t* data = static_cast<const uint8_t*>(m->m_pData);
				event.data.assign(data, data + m->m_cbSize);
				// GNS's thread can have a message ready before its Connected has been
				// reported, and a caller must hear of a connection before hearing from it
				if (connecting_.count(event.connection))
					held_.push_back(std::move(event));
				else
					events_.push_back(std::move(event));
				m->Release();
			}
			if (count < 32)
				return;
		}
	}

	void pushState(EventType type, ConnectionId connection)
	{
		Event event;
		event.type = type;
		event.connection = connection;
		events_.push_back(std::move(event));
	}

	HSteamNetPollGroup group_ = k_HSteamNetPollGroup_Invalid;
	std::set<HSteamListenSocket> listeners_;
	std::set<ConnectionId> connections_;
	std::set<ConnectionId> incoming_;	// accepted, and not yet Connected
	std::set<ConnectionId> connecting_;	// either way, and not yet Connected
	std::deque<Event> events_;
	std::vector<Event> held_;			// messages for those still connecting
};

void onStatusChanged(SteamNetConnectionStatusChangedCallback_t* info)
{
	const HSteamNetConnection connection = info->m_hConn;
	switch (info->m_info.m_eState) {
	case k_ESteamNetworkingConnectionState_Connecting: {
		// ours from connect() are already known; a new one comes through a listen socket
		const auto listener = g_listeners.find(info->m_info.m_hListenSocket);
		if (listener != g_listeners.end())
			listener->second->onConnecting(connection);
		break;
	}
	case k_ESteamNetworkingConnectionState_Connected: {
		const auto owner = g_connections.find(connection);
		if (owner != g_connections.end())
			owner->second->onConnected(connection);
		break;
	}
	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally: {
		const auto owner = g_connections.find(connection);
		if (owner != g_connections.end())
			owner->second->onLost(connection);
		else
			SteamNetworkingSockets()->CloseConnection(connection, 0, nullptr, false);
		break;
	}
	default:
		break;
	}
}

}

std::unique_ptr<Transport> createGnsTransport(std::string* error, int connectTimeoutMs)
{
	if (g_transports == 0) {
		SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Error, logMessage);
		SteamNetworkingErrMsg message;
		if (!GameNetworkingSockets_Init(nullptr, message)) {
			if (error)
				*error = message;
			return nullptr;
		}
		SteamNetworkingUtils()->SetGlobalCallback_SteamNetConnectionStatusChanged(onStatusChanged);
	}
	g_transports++;
	if (connectTimeoutMs > 0)
		SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_TimeoutInitial, connectTimeoutMs);

	return std::unique_ptr<Transport>(new GnsTransport());
}

}
