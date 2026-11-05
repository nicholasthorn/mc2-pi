// code/netlobby.cpp: see netlobby.h

#include "netlobby.h"

#include <algorithm>

using namespace netwire;

namespace netlobby {

Lobby::Lobby(uint8_t maxPlayers)
{
	state_.maxPlayers = maxPlayers;
}

LobbyPlayer* Lobby::findPlayer(uint8_t slot)
{
	for (LobbyPlayer& player : state_.players)
		if (player.slot == slot)
			return &player;
	return nullptr;
}

const LobbyPlayer* Lobby::player(uint8_t slot) const
{
	for (const LobbyPlayer& player : state_.players)
		if (player.slot == slot)
			return &player;
	return nullptr;
}

bool Lobby::addPlayer(uint8_t slot, const std::string& name)
{
	if (slot >= state_.maxPlayers || findPlayer(slot))
		return false;
	LobbyPlayer added;
	added.slot = slot;
	added.name = name;
	bool teamTaken[MAX_PLAYERS] = {};
	bool colorTaken[MAX_LOBBY_COLORS] = {};
	for (const LobbyPlayer& player : state_.players) {
		teamTaken[player.team] = true;
		colorTaken[player.baseColor] = true;
	}
	while (added.team < MAX_PLAYERS - 1 && teamTaken[added.team])
		added.team++;
	while (added.baseColor < MAX_LOBBY_COLORS - 1 && colorTaken[added.baseColor])
		added.baseColor++;
	added.stripeColor = added.baseColor;

	// a name the message could not carry would leave the whole state unsendable
	LobbyState trial;
	trial.maxPlayers = state_.maxPlayers;
	trial.players.push_back(added);
	std::vector<uint8_t> out;
	if (!encode(trial, out))
		return false;

	state_.players.push_back(added);
	std::sort(state_.players.begin(), state_.players.end(), [](const LobbyPlayer& a, const LobbyPlayer& b) { return a.slot < b.slot; });
	changed();
	return true;
}

void Lobby::removePlayer(uint8_t slot)
{
	const size_t before = state_.players.size();
	state_.players.erase(std::remove_if(state_.players.begin(), state_.players.end(),
		[slot](const LobbyPlayer& player) { return player.slot == slot; }), state_.players.end());
	if (state_.players.size() != before)
		changed();
}

bool Lobby::applyChoice(uint8_t slot, const LobbyChoice& choice)
{
	LobbyPlayer* player = findPlayer(slot);
	std::vector<uint8_t> out;
	if (!player || !encode(choice, out))
		return false;
	for (const LobbyPlayer& other : state_.players)
		if (other.slot != slot && other.baseColor == choice.baseColor)
			return false;
	const bool ready = choice.ready && !state_.map.empty() && (choice.mapHash == state_.mapHash) && (choice.revision >= readyFrom_);
	// the same again is no change: a client answers each new state, and must not start a loop
	const std::string& name = choice.name.empty() ? player->name : choice.name;
	if (player->team == choice.team && player->baseColor == choice.baseColor && player->stripeColor == choice.stripeColor && player->ready == ready && player->name == name)
		return true;
	player->name = name;
	player->team = choice.team;
	player->baseColor = choice.baseColor;
	player->stripeColor = choice.stripeColor;
	player->ready = ready;
	changed();
	return true;
}

void Lobby::clearReady()
{
	for (LobbyPlayer& player : state_.players)
		player.ready = false;
}

bool Lobby::setMap(const std::string& map, uint32_t mapHash)
{
	LobbyState trial = state_;
	trial.map = map;
	trial.mapHash = mapHash;
	std::vector<uint8_t> out;
	if (!encode(trial, out))
		return false;
	state_.map = map;
	state_.mapHash = mapHash;
	clearReady();
	changed();
	readyFrom_ = state_.revision;
	return true;
}

bool Lobby::setSettings(int32_t timeLimit, int32_t resourcePoints, uint16_t flags)
{
	LobbyState trial = state_;
	trial.timeLimit = timeLimit;
	trial.resourcePoints = resourcePoints;
	trial.flags = flags;
	std::vector<uint8_t> out;
	if (!encode(trial, out))
		return false;
	state_.timeLimit = timeLimit;
	state_.resourcePoints = resourcePoints;
	state_.flags = flags;
	clearReady();
	changed();
	readyFrom_ = state_.revision;
	return true;
}

bool Lobby::canStart(std::string* why) const
{
	const char* reason = nullptr;
	bool twoTeams = false;
	bool allReady = true;
	for (const LobbyPlayer& player : state_.players) {
		if (player.team != state_.players[0].team)
			twoTeams = true;
		if (player.slot != 0 && !player.ready)
			allReady = false;
	}
	if (state_.players.size() < 2)
		reason = "Waiting for another player";
	else if (state_.map.empty())
		reason = "No map chosen";
	else if (!twoTeams)
		reason = "Everyone is on one team";
	else if (!allReady)
		reason = "Not everyone is ready";
	if (why)
		*why = reason ? reason : "";
	return !reason;
}

uint8_t Lobby::commanders(uint8_t out[MAX_PLAYERS]) const
{
	std::fill(out, out + MAX_PLAYERS, NO_COMMANDER);
	uint8_t next = 0;
	for (const LobbyPlayer& player : state_.players)	// in slot order, so the host is 0
		out[player.slot] = next++;
	return next;
}

}
