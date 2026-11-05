// code/netsession.cpp: see netsession.h

#include "netsession.h"

#include <algorithm>
#include <cstdio>

using namespace gos_net;
using namespace netwire;

namespace netsession {

bool Session::host(uint16_t port, uint8_t numPlayers, uint32_t dataHash)
{
	if (numPlayers < 1 || numPlayers > MAX_PLAYERS || !transport_.listen(port))
		return false;
	isHost_ = true;
	numPlayers_ = numPlayers;
	dataHash_ = dataHash;
	commander_ = 0;
	commanderTaken_[0] = true;
	return true;
}

bool Session::join(const char* address, uint16_t port, uint32_t dataHash, const std::string& name)
{
	isHost_ = false;
	dataHash_ = dataHash;
	name_ = name;
	return transport_.connect(address, port, server_);
}

void Session::update()
{
	Event event;
	while (transport_.poll(event)) {
		if (isHost_)
			handleHost(event);
		else
			handleClient(event);
	}
}

Session::Client* Session::findClient(ConnectionId connection)
{
	for (Client& client : clients_)
		if (client.connection == connection)
			return &client;
	return nullptr;
}

void Session::reject(ConnectionId connection, RejectReason reason)
{
	Welcome welcome;
	welcome.reject = reason;
	if (encode(welcome, scratch_))
		transport_.send(connection, Delivery::ReliableOrdered, scratch_.data(), scratch_.size());
	transport_.disconnect(connection);
	Client* client = findClient(connection);
	if (client) {
		if (client->hello && !startSent_)
			commanderTaken_[client->commander] = false;
		clients_.erase(clients_.begin() + (client - clients_.data()));
	}
}

void Session::drop(ConnectionId connection, const char* why)
{
	Client* client = findClient(connection);
	if (client) {
		printf("[NET] dropping commander %d: %s\n", client->hello ? client->commander : -1, why);
		if (client->hello && !startSent_)
			commanderTaken_[client->commander] = false;
		clients_.erase(clients_.begin() + (client - clients_.data()));
	}
	transport_.disconnect(connection);
}

void Session::handleHello(Client& client, const Event& event)
{
	uint16_t protocol = 0;
	if (!peekProtocol(event.data.data(), event.data.size(), protocol)) {
		drop(event.connection, "bad first message");
		return;
	}
	if (protocol != PROTOCOL_VERSION) {
		reject(event.connection, RejectReason::WrongVersion);
		return;
	}
	Hello hello;
	if (!decode(event.data.data(), event.data.size(), hello)) {
		drop(event.connection, "bad Hello");
		return;
	}
	if (hello.dataHash != dataHash_) {
		reject(event.connection, RejectReason::WrongData);
		return;
	}
	uint8_t place = 0;
	while (place < numPlayers_ && commanderTaken_[place])
		place++;
	if (place == numPlayers_ || startSent_) {
		reject(event.connection, RejectReason::GameFull);
		return;
	}
	commanderTaken_[place] = true;
	client.commander = place;
	client.hello = true;
	Welcome welcome;
	welcome.commanderId = place;
	welcome.numPlayers = numPlayers_;
	if (encode(welcome, scratch_))
		transport_.send(event.connection, Delivery::ReliableOrdered, scratch_.data(), scratch_.size());
	printf("[NET] commander %d joined: %s\n", place, hello.name.c_str());
}

void Session::handleHost(const Event& event)
{
	switch (event.type) {
	case EventType::Connected:
		if (!findClient(event.connection)) {
			Client client;
			client.connection = event.connection;
			clients_.push_back(client);
		}
		break;
	case EventType::Disconnected: {
		Client* client = findClient(event.connection);
		if (client) {
			if (client->hello) {
				printf("[NET] commander %d left\n", client->commander);
				// before the mission starts the place is free again; after, its units stay
				if (!startSent_)
					commanderTaken_[client->commander] = false;
			}
			clients_.erase(clients_.begin() + (client - clients_.data()));
		}
		break;
	}
	case EventType::Message: {
		Client* client = findClient(event.connection);
		if (!client)
			break;
		MessageType type;
		if (!peekType(event.data.data(), event.data.size(), type)) {
			drop(event.connection, "unknown message");
			break;
		}
		if (!client->hello) {
			if (type == MessageType::Hello)
				handleHello(*client, event);
			else
				drop(event.connection, "message before Hello");
			break;
		}
		if (type == MessageType::Ready) {
			Ready ready;
			if (!decode(event.data.data(), event.data.size(), ready) || client->sentReady) {
				drop(event.connection, "bad Ready");
				break;
			}
			client->sentReady = true;
			client->rosterHash = ready.rosterHash;
			// with three players one can finish loading while the host waits for another
			if (haveRosterHash_)
				judgeReady(event.connection);
		}
		else if (type == MessageType::ChunkBatch) {
			Received received;
			if (!startSent_ || !decode(event.data.data(), event.data.size(), received.batch)
				|| received.batch.kind != ChunkKind::Order) {
				drop(event.connection, "bad batch");
				break;
			}
			received.commander = client->commander;
			received.delivery = event.delivery;
			inbox_.push_back(std::move(received));
		}
		else
			drop(event.connection, "message only a host may send");
		break;
	}
	}
}

void Session::judgeReady(ConnectionId connection)
{
	Client* client = findClient(connection);
	if (!client)
		return;
	if (client->rosterHash != rosterHash_) {
		printf("[NET] commander %d loaded other units: %08x, here %08x\n", client->commander, client->rosterHash, rosterHash_);
		reject(connection, RejectReason::WrongRoster);
		return;
	}
	client->ready = true;
}

void Session::handleClient(const Event& event)
{
	if (event.connection != server_)
		return;
	switch (event.type) {
	case EventType::Connected: {
		Hello hello;
		hello.dataHash = dataHash_;
		hello.name = name_;
		if (encode(hello, scratch_))
			transport_.send(server_, Delivery::ReliableOrdered, scratch_.data(), scratch_.size());
		break;
	}
	case EventType::Disconnected:
		hostLost_ = true;
		break;
	case EventType::Message: {
		MessageType type;
		if (!peekType(event.data.data(), event.data.size(), type))
			break;
		switch (type) {
		case MessageType::Welcome: {
			Welcome welcome;
			if (!decode(event.data.data(), event.data.size(), welcome))
				break;
			// a refusal can also come after joining, at the load barrier
			if (welcome.reject != RejectReason::None) {
				rejected_ = welcome.reject;
				break;
			}
			if (joined_)
				break;
			commander_ = welcome.commanderId;
			numPlayers_ = welcome.numPlayers;
			joined_ = true;
			break;
		}
		case MessageType::Start: {
			Start start;
			if (joined_ && decode(event.data.data(), event.data.size(), start))
				started_ = true;
			break;
		}
		case MessageType::EndGame: {
			EndGame end;
			if (started_ && decode(event.data.data(), event.data.size(), end)) {
				winningTeam_ = end.winningTeam;
				gameOver_ = true;
			}
			break;
		}
		case MessageType::ChunkBatch: {
			Received received;
			// state can overtake the Start on the way here: there is nothing yet to apply it to
			if (started_ && decode(event.data.data(), event.data.size(), received.batch)) {
				received.delivery = event.delivery;
				inbox_.push_back(std::move(received));
			}
			break;
		}
		default:
			break;
		}
		break;
	}
	}
}

bool Session::allJoined() const
{
	int joined = 1;	// the host
	for (const Client& client : clients_)
		if (client.hello)
			joined++;
	return joined == numPlayers_;
}

bool Session::allReady() const
{
	if (!allJoined())
		return false;
	for (const Client& client : clients_)
		if (client.hello && !client.ready)
			return false;
	return true;
}

void Session::setRosterHash(uint32_t hash)
{
	rosterHash_ = hash;
	haveRosterHash_ = true;
	// judging one can remove it from the list
	std::vector<ConnectionId> waiting;
	for (const Client& client : clients_)
		if (client.sentReady && !client.ready)
			waiting.push_back(client.connection);
	for (ConnectionId connection : waiting)
		judgeReady(connection);
}

bool Session::sendAll(Delivery delivery, const std::vector<uint8_t>& message)
{
	std::vector<ConnectionId> behind;
	bool all = true;
	for (const Client& client : clients_) {
		if (!client.hello)
			continue;
		if (!transport_.send(client.connection, delivery, message.data(), message.size())) {
			all = false;
			// an unreliable update can be missed; a client that misses a hit never recovers
			if (delivery != Delivery::Unreliable)
				behind.push_back(client.connection);
		}
	}
	for (ConnectionId connection : behind)
		drop(connection, "too far behind");
	return all;
}

bool Session::start()
{
	if (startSent_ || !allReady() || !encode(Start(), scratch_))
		return false;
	startSent_ = true;
	started_ = true;
	return sendAll(Delivery::ReliableOrdered, scratch_);
}

bool Session::sendToClients(Delivery delivery, const ChunkBatch& batch)
{
	return encode(batch, scratch_) && sendAll(delivery, scratch_);
}

void Session::endGame(uint8_t winningTeam)
{
	EndGame end;
	end.winningTeam = winningTeam;
	if (encode(end, scratch_))
		sendAll(Delivery::ReliableOrdered, scratch_);
	winningTeam_ = winningTeam;
	gameOver_ = true;
}

int Session::connectedClients() const
{
	int count = 0;
	for (const Client& client : clients_)
		if (client.hello)
			count++;
	return count;
}

bool Session::clientConnected(uint8_t commander) const
{
	for (const Client& client : clients_)
		if (client.hello && client.commander == commander)
			return true;
	return false;
}

bool Session::sendReady(uint32_t rosterHash)
{
	Ready ready;
	ready.rosterHash = rosterHash;
	return joined_ && encode(ready, scratch_)
		&& transport_.send(server_, Delivery::ReliableOrdered, scratch_.data(), scratch_.size());
}

bool Session::sendToHost(Delivery delivery, const ChunkBatch& batch)
{
	return started_ && encode(batch, scratch_)
		&& transport_.send(server_, delivery, scratch_.data(), scratch_.size());
}

bool Session::nextBatch(Received& out)
{
	if (inbox_.empty())
		return false;
	out = std::move(inbox_.front());
	inbox_.pop_front();
	return true;
}

}
