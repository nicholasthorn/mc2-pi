// code/netsession.h
//
// One multiplayer game's connections, above the transport and below the game: who
// has joined, who has loaded, when the mission starts and ends, and the chunk batches
// passing between. No GL, SDL or game dependency, so tests run it on a loopback.
//
// The order of things:
//   host: listen, update until allJoined(), load, setRosterHash, update until
//         allReady(), start(); then sendToClients() and nextBatch() each frame.
//   client: join, update until joined(), load, sendReady, update until started();
//         then sendToHost() and nextBatch() each frame.
//
// Everything received is untrusted. A client that breaks the order, sends what only
// a host may send, or falls so far behind that its reliable queue is full is
// disconnected; the host and the other clients carry on without it.

#ifndef NETSESSION_H
#define NETSESSION_H

#include "netwire.h"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace netsession {

constexpr uint16_t DEFAULT_PORT = 27815;

// A batch that arrived. commander is who sent it (for a client, always the host's 0).
struct Received {
	uint8_t commander = 0;
	gos_net::Delivery delivery = gos_net::Delivery::ReliableOrdered;
	netwire::ChunkBatch batch;
};

class Session {
public:
	explicit Session(gos_net::Transport& transport) : transport_(transport) {}

	// Host. Commander 0 is the host itself, and clients take 1 to numPlayers - 1 as they
	// join. False if the port cannot be had.
	bool host(uint16_t port, uint8_t numPlayers, uint32_t dataHash);

	// Client. False if the connection could not even be started.
	bool join(const char* address, uint16_t port, uint32_t dataHash, const std::string& name);

	// Handles whatever has arrived. Call often while waiting, and each frame after.
	void update();

	bool isHost() const { return isHost_; }
	uint8_t commander() const { return commander_; }
	uint8_t numPlayers() const { return numPlayers_; }

	// host
	bool allJoined() const;
	bool allReady() const;
	void setRosterHash(uint32_t hash);	// before a Ready can be judged
	bool start();
	bool sendToClients(gos_net::Delivery delivery, const netwire::ChunkBatch& batch);
	void endGame(uint8_t winningTeam);
	int connectedClients() const;
	// false if nobody is there any more: a client joined and left
	bool clientConnected(uint8_t commander) const;

	// client
	bool joined() const { return joined_; }
	bool started() const { return started_; }
	netwire::RejectReason rejected() const { return rejected_; }
	bool sendReady(uint32_t rosterHash);
	bool sendToHost(gos_net::Delivery delivery, const netwire::ChunkBatch& batch);

	// either: the other end is gone (client), and a game over was announced
	bool hostLost() const { return hostLost_; }
	bool gameOver() const { return gameOver_; }
	uint8_t winningTeam() const { return winningTeam_; }

	bool nextBatch(Received& out);

private:
	struct Client {
		gos_net::ConnectionId connection = gos_net::INVALID_CONNECTION;
		bool hello = false;
		bool ready = false;
		bool sentReady = false;		// and its hash, until the host has its own to compare
		uint32_t rosterHash = 0;
		uint8_t commander = 0;
	};

	void handleHost(const gos_net::Event& event);
	void handleClient(const gos_net::Event& event);
	void handleHello(Client& client, const gos_net::Event& event);
	void reject(gos_net::ConnectionId connection, netwire::RejectReason reason);
	void drop(gos_net::ConnectionId connection, const char* why);
	void judgeReady(gos_net::ConnectionId connection);
	Client* findClient(gos_net::ConnectionId connection);
	bool sendAll(gos_net::Delivery delivery, const std::vector<uint8_t>& message);

	gos_net::Transport& transport_;
	bool isHost_ = false;
	uint8_t commander_ = 0;
	uint8_t numPlayers_ = 0;
	uint32_t dataHash_ = 0;

	std::vector<Client> clients_;			// host
	bool commanderTaken_[netwire::MAX_PLAYERS] = {};
	uint32_t rosterHash_ = 0;
	bool haveRosterHash_ = false;
	bool startSent_ = false;

	gos_net::ConnectionId server_ = gos_net::INVALID_CONNECTION;	// client
	std::string name_;
	bool joined_ = false;
	bool started_ = false;
	netwire::RejectReason rejected_ = netwire::RejectReason::None;
	bool hostLost_ = false;

	bool gameOver_ = false;
	uint8_t winningTeam_ = netwire::NO_WINNER;
	std::deque<Received> inbox_;
	std::vector<uint8_t> scratch_;
};

}

#endif
