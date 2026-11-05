// code/netlobby.h
//
// The rules of a multiplayer lobby: who is in it, on which team and in which colours, which
// map and settings, and whether the game can start. The host holds the one that counts;
// clients see its state (netwire::LobbyState) and ask for changes (netwire::LobbyChoice). No
// GL, SDL or game dependency.
//
// A slot here is a player's commander in netsession from when they joined. Players can leave
// and leave a gap, which the game cannot take, so a launch renumbers them: commanders().

#ifndef NETLOBBY_H
#define NETLOBBY_H

#include "netwire.h"

#include <cstdint>
#include <string>

namespace netlobby {

constexpr uint8_t MAX_COLORS = netwire::MAX_LOBBY_COLORS;

class Lobby {
public:
	explicit Lobby(uint8_t maxPlayers);

	// A new player is on a team and in a colour nobody has, and not ready. False if the slot
	// is taken, past the limit, or the name cannot be sent.
	bool addPlayer(uint8_t slot, const std::string& name);
	void removePlayer(uint8_t slot);

	// A player's own choice, from the network or from this machine. Refused whole if anything
	// in it is out of range, or the colour is someone else's. Ready only sticks with the map's
	// hash, so nobody can be ready on a map they lack.
	bool applyChoice(uint8_t slot, const netwire::LobbyChoice& choice);

	// The host's. Each takes everyone's ready back: they agreed to something else.
	void setMap(const std::string& map, uint32_t mapHash);
	void setSettings(int32_t timeLimit, int32_t resourcePoints, uint16_t flags);

	// The host's own place needs no ready: starting is its say. If not, why not.
	bool canStart(std::string* why = nullptr) const;

	// For a launch: by slot, each player's commander in the game, from 0 with no gaps, and
	// NO_COMMANDER for nobody. The number of players.
	uint8_t commanders(uint8_t out[netwire::MAX_PLAYERS]) const;

	const netwire::LobbyState& state() const { return state_; }
	const netwire::LobbyPlayer* player(uint8_t slot) const;

private:
	netwire::LobbyPlayer* findPlayer(uint8_t slot);
	void changed() { state_.revision++; }
	void clearReady();

	netwire::LobbyState state_;
	uint32_t mapHash_ = 0;
};

}

#endif
