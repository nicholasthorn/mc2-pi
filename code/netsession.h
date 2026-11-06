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

// What has happened in a lobby, for the host to put to its netlobby::Lobby.
enum class LobbyEventType { Joined, Left, Choice };

struct LobbyEvent {
	LobbyEventType type = LobbyEventType::Joined;
	uint8_t commander = 0;
	std::string name;				// Joined
	netwire::LobbyChoice choice;	// Choice
};

// Ranks for a results table, from each player's standing, higher is better: 1 for the
// best, players level share a rank, and the next rank skips past them.
void rankPlayers(const long* standings, int count, long* ranks);

// the most chat lines that wait to be shown (Session::nextChat)
constexpr size_t MAX_CHAT_INBOX = 64;

class Session {
public:
	explicit Session(gos_net::Transport& transport) : transport_(transport) {}

	// Host. Commander 0 is the host itself, and clients take 1 to numPlayers - 1 as they
	// join. False if the port cannot be had. Port 0 listens on none, for a host that dials.
	bool host(uint16_t port, uint8_t numPlayers, uint32_t dataHash);

	// Host, with a lobby: players come and go up to maxPlayers, with no number decided until
	// launch(). Then it goes on as above, from "load".
	bool hostLobby(uint16_t port, uint8_t maxPlayers, uint32_t dataHash);
	bool sendLobbyState(const netwire::LobbyState& state);
	bool nextLobbyEvent(LobbyEvent& out);
	// Ends the lobby and renumbers everyone. False if it does not match who is here, or if the
	// host is not commander 0, which the game takes for granted. Whoever is too far behind to
	// be told is dropped, as anyone can be; the game goes on with the rest.
	bool launch(const netwire::LobbyLaunch& launch);

	// Client. False if the connection could not even be started.
	bool join(const char* address, uint16_t port, uint32_t dataHash, const std::string& name);

	// The same two, the other way round, for a host that cannot be reached but can reach
	// out (behind address translation, as in WSL): the client listens for one connection,
	// and the host dials it. After connecting nothing differs: the client still says Hello.
	// Whoever connects first is taken as the host, so this is not for the open internet.
	// A dial that finds nobody listening fails after a while: call it again.
	bool joinWaiting(uint16_t port, uint32_t dataHash, const std::string& name);
	bool dialClient(const char* address, uint16_t port);

	// Handles whatever has arrived. Call often while waiting, and each frame after.
	void update();

	bool isHost() const { return isHost_; }
	uint8_t commander() const { return commander_; }
	uint8_t numPlayers() const { return numPlayers_; }

	// host
	bool allJoined() const;
	bool allReady() const;
	// A launched game with nobody left to play it: they all went while loading.
	bool allLeft() const { return launched_ && numPlayers_ > 1 && connectedClients() == 0; }
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
	bool hasLobbyState() const { return hasLobbyState_; }
	const netwire::LobbyState& lobbyState() const { return lobbyState_; }
	bool sendLobbyChoice(const netwire::LobbyChoice& choice);
	bool launched() const { return launched_; }
	const netwire::LobbyLaunch& lobbyLaunch() const { return lobbyLaunch_; }	// either end, once launched
	gos_net::ConnectionId serverConnection() const { return server_; }
	bool sendToHost(gos_net::Delivery delivery, const netwire::ChunkBatch& batch);

	// The units each player chose, between a launch and the start, in a game without quickstart.
	// A client sends its own once, for its commander in the game; the host takes each with
	// nextForce, judges it, and sends every player's, its own among them. A client that sends
	// another's, a second, or one outside that time is dropped.
	bool sendForce(const netwire::Force& force);
	bool nextForce(netwire::Force& out);	// host
	// The host's word on that commander's units, at either end: once said, it stands.
	bool hasForce(uint8_t commander) const;
	const netwire::Force& force(uint8_t commander) const;

	// Chat, at either end, from joining on. A host sends each line, its own and its clients',
	// to everyone with who it came from, and keeps it to show; a client shows only what comes
	// back. from is put in by the host, whatever a client says. The oldest lines go when more
	// than MAX_CHAT_INBOX wait.
	bool sendChat(netwire::Chat chat);
	bool nextChat(netwire::Chat& out);

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
		bool sentForce = false;
		uint32_t rosterHash = 0;
		uint8_t commander = 0;
	};

	void lobbyPlayerGone(const Client& client);
	void handleHost(const gos_net::Event& event);
	void handleClient(const gos_net::Event& event);
	void handleHello(Client& client, const gos_net::Event& event);
	void reject(gos_net::ConnectionId connection, netwire::RejectReason reason);
	void drop(gos_net::ConnectionId connection, const char* why);
	void judgeReady(gos_net::ConnectionId connection);
	Client* findClient(gos_net::ConnectionId connection);
	bool sendAll(gos_net::Delivery delivery, const std::vector<uint8_t>& message);
	void keepChat(netwire::Chat chat);

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
	bool lobbyOpen_ = false;
	std::deque<LobbyEvent> lobbyEvents_;
	netwire::LobbyState lobbyState_;		// client
	bool hasLobbyState_ = false;
	netwire::LobbyLaunch lobbyLaunch_;
	std::deque<netwire::Force> forceInbox_;	// host
	bool forceSent_ = false;				// client
	bool haveForce_[netwire::MAX_PLAYERS] = {};
	netwire::Force forces_[netwire::MAX_PLAYERS];

	gos_net::ConnectionId server_ = gos_net::INVALID_CONNECTION;	// client
	std::string name_;
	bool waitingForHost_ = false;
	bool joined_ = false;
	bool started_ = false;
	bool launched_ = false;					// either: the lobby is over
	netwire::RejectReason rejected_ = netwire::RejectReason::None;
	bool hostLost_ = false;

	bool gameOver_ = false;
	uint8_t winningTeam_ = netwire::NO_WINNER;
	std::deque<Received> inbox_;
	std::deque<netwire::Chat> chatInbox_;
	std::vector<uint8_t> scratch_;
};

}

#endif
