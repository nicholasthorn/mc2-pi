//***************************************************************************
//
//	multplyr.cpp - This file contains the MultiPlayer Class
//
//---------------------------------------------------------------------------//
// Copyright (C) Microsoft Corporation. All rights reserved.                 //
//===========================================================================//

#ifndef MCLIB_H
#include"mclib.h"
#endif

#ifndef LINUX_BUILD
#include"crtdbg.h"
#endif

#ifndef MULTPLYR_H
#include"multplyr.h"
#endif

#ifndef TEAM_H
#include"team.h"
#endif

#ifndef MOVER_H
#include"mover.h"
#endif

#ifndef TURRET_H
#include"turret.h"
#endif

#ifndef MISSION_H
#include"mission.h"
#endif

#ifndef MECH_H
#include"mech.h"
#endif

#ifndef GAMEOBJ_H
#include"gameobj.h"
#endif

#ifndef OBJMGR_H
#include"objmgr.h"
#endif

#ifndef LOGISTICSDATA_H
#include"logisticsdata.h"
#endif

#ifndef BLDNG_H
#include"bldng.h"
#endif

#ifndef TERROBJ_H
#include"terrobj.h"
#endif

#ifndef MOVE_H
#include"move.h"
#endif

#ifndef GROUP_H
#include"group.h"
#endif

#ifndef COMNDR_H
#include"comndr.h"
#endif

#ifndef CARNAGE_H
#include"carnage.h"
#endif

#ifndef ARTLRY_H
#include"artlry.h"
#endif

#ifndef CONTROLGUI_H
#include"controlgui.h"
#endif

#ifndef LOGISTICS_H
#include"logistics.h"
#endif

#include"missionbegin.h"

#ifndef PREFS_H
#include"prefs.h"
#endif

#ifndef MISSIONGUI_H
#include"missiongui.h"
#endif

#ifndef GAMELOG_H
#include"gamelog.h"
#endif

#ifndef LINUX_BUILD
#include"dplay8.h"
#endif

#include"mpparameterscreen.h"

//sebi: commented include
//#ifndef VERSION_H
//#include"version.h"
//#endif

#include "../resource.h"

#ifdef USE_MISSION_RESULTS_SCREEN
extern bool EventsToMissionResultsScreen;
#else
bool EventsToMissionResultsScreen = false;
#endif

#ifdef USE_LOGISTICS
extern bool whackTimer;
void CancelBool(long value);
#endif

#ifndef MPPREFS_H
#include"mpprefs.h"
#endif

#ifdef USE_STRING_RESOURCES
extern HINSTANCE thisInstance;
long cLoadString (HINSTANCE hInstance,  UINT uID, LPTSTR lpBuffer, int nBufferMax );
#endif

#include"gamesound.h"

#include"netsession.h"
#include"netrules.h"
#include"netlan.h"
#include"netlobby.h"

#include<algorithm>
#include<cctype>
#include<chrono>
#include<cmath>
#include<memory>
#include<random>
#include<string>
#include<thread>
#include<vector>

#define	MAX_MSG_SIZE		10240

extern CPrefs prefs;
extern bool quitGame;

extern float loadProgress;
extern bool aborted;

//***************************************************************************

DWORD ServerPlayerNum = 1; //commanderId (or checkInId) of server
bool MultiPlayer::launchedFromLobby = false;
bool MultiPlayer::registerZone = false;

long MultiPlayer::presetDropZones[MAX_MC_PLAYERS] = {-1, -1, -1, -1, -1, -1, -1, -1};
long MultiPlayer::colors[MAX_COLORS] = {0};

float ResourceBuildingRefreshRate = 60.0f;

// {35DC7890-C5EF-4171-B0CF-4D5C7AE7C2D7}
static const GUID MC2GUID = { 0x35dc7890, 0xc5ef, 0x4171, { 0xb0, 0xcf, 0x4d, 0x5c, 0x7a, 0xe7, 0xc2, 0xd7 } };
// {1F8251BB-1436-44b3-A5B0-0FCF6858C176}
static const GUID MC2DEMOGUID = { 0x1f8251bb, 0x1436, 0x44b3, { 0xa5, 0xb0, 0xf, 0xcf, 0x68, 0x58, 0xc1, 0x76 } };

void* MC2NetLib = NULL;
bool OnLAN = false;
NETMESSAGE ReceiveMsg;

//------------
// EXTERN vars
extern GameLog* CombatLog;
extern GameLog* NetLog;

extern bool LaunchedFromLobby;

extern UserHeapPtr systemHeap;

#ifdef _DEBUG
//extern DebugFileStream Debug;
#endif

extern char* startupPakFile;

// ConnectUsingDialog is a function defined in dpdialog.cpp.  
// It allows the user to choose the type of connection.
//extern HRESULT ConnectUsingDialog(HINSTANCE );

extern void SortMoverList (long numMovers, MoverPtr* moverList, Stuff::Vector3D dest);

extern void killTheGame(void);

#ifndef TERRAINEDIT
extern DebuggerPtr debugger;
#endif

//------------
// GLOBAL vars
MultiPlayer* MPlayer = NULL;
//extern long NumMissionScriptMessages;

///extern Scenario* scenario;
///extern Logistics* globalLogPtr;

void DEBUGWINS_print (const char* s, long window = 0);

//***************************************************************************
// MISC functions
//***************************************************************************

bool StartupNetworking (void) {

	return false;
}

//-----------------------------------------------------------------------------

void ResetNetworking (void) {

}

//-----------------------------------------------------------------------------

static void netClose (void);

void ShutdownNetworking (void) {

	// before the game's heap goes, which is where these came from
	netClose();
}

//***************************************************************************
// WORLD CHUNK class
//***************************************************************************

void* WorldChunk::operator new (size_t ourSize) {

	void* result = systemHeap->Malloc(ourSize);
	return(result);
}

//---------------------------------------------------------------------------

void WorldChunk::operator delete (void* us) {

	systemHeap->Free(us);
}	

//---------------------------------------------------------------------------

void WorldChunk::buildMine (long worldCellR,
							long worldCellC,
							long teamId,
							long mineState,
							long explosionType) {

}

//---------------------------------------------------------------------------

void WorldChunk::buildTerrainFire (GameObjectPtr object,
								   long seconds) {

}

//---------------------------------------------------------------------------

void WorldChunk::buildArtillery (long commanderId,
								 long artilleryType,
								 Stuff::Vector3D location,
								 long seconds) {

}

//---------------------------------------------------------------------------

void WorldChunk::buildMissionScriptMessage (long messageCode,
											long messageParam) {

}

//---------------------------------------------------------------------------

void WorldChunk::buildPilotKillStat (
									 long moverIndex,
									 long vehicleClass) {

}

//--------------------------------------------------------------------------

void WorldChunk::buildScore (long commanderID, long score) {

}

//--------------------------------------------------------------------------

void WorldChunk::buildKillLoss (long killerCID, long loserCID) {

}

//--------------------------------------------------------------------------

void WorldChunk::buildCaptureBuilding (BuildingPtr building, long newCommanderID) {

}

//---------------------------------------------------------------------------

void WorldChunk::buildEndMission (void) {

}

//--------------------------------------------------------------------------

void WorldChunk::pack (void) {

}

//---------------------------------------------------------------------------
		
void WorldChunk::unpack (void) {

}

//---------------------------------------------------------------------------

bool WorldChunk::equalTo (WorldChunkPtr chunk) {

	return false;
}

//***************************************************************************
// MECHCOMMANDER MESSAGE handlers
//***************************************************************************

//***************************************************************************
// MULTIPLAYER class
//***************************************************************************

void* MultiPlayer::operator new (size_t ourSize) {

	void* result = systemHeap->Malloc(ourSize);
	return(result);
}

//---------------------------------------------------------------------------

void MultiPlayer::operator delete (void* us) {

	systemHeap->Free(us);
}	

//---------------------------------------------------------------------------

void MultiPlayer::init (void) {

	// no support vehicle or recovery under way
	for (long i = 0; i < MAX_MC_PLAYERS; i++)
		reinforcements[i][0] = reinforcements[i][1] = -1;
}

//---------------------------------------------------------------------------

long MultiPlayer:: setup (void) {

	return(MPLAYER_NO_ERR);
}

//---------------------------------------------------------------------------

void MultiPlayer::startLocalHost (long numPlayers) {

	iAmHost = true;
	commanderID = 0;
	// Every commander is on a team of their own. Mission::init reads these, and
	// Building::setTeamId clears any team at or past numTeams.
	numTeams = numPlayers;
	for (long i = 0; i < MAX_MC_PLAYERS; i++) {
		const char id = (i < numPlayers) ? (char)i : (char)-1;
		playerInfo[i].commanderID = id;
		playerInfo[i].team = id;
	}
}

//---------------------------------------------------------------------------

void MultiPlayer::initUpdateFrequencies() {

	moverUpdateFrequency = 0.2f;
	worldUpdateFrequency = 5.0f;	// the convergence check
	warpFactor = 300.0f;			// about seven cells: enough for a late update, not for a teleport
}

//---------------------------------------------------------------------------
// The network game: what connects the MultiPlayer methods to the transport.
//---------------------------------------------------------------------------

// Null in a game with no network (-nethost alone, and everything else).
static std::unique_ptr<gos_net::Transport> netTransport;
static std::unique_ptr<netsession::Session> netSession;

static void addWorldEvent (MultiPlayer* player, netwire::WorldEventType type, long a, long b, unsigned long value);

static void netClose (void) {

	netSession.reset();
	netTransport.reset();
}

static const float NET_WAIT_SECONDS = 300.0f;
static long netOrdersApplied = 0;
static bool netGameEnded = false;
static long netRepairsApplied = 0;
static long netOrdersByCode[NUM_TACTICAL_ORDERS] = {0};	// on the host
static long netHoldsApplied = 0;
static long netWorldApplied[10] = {0};	// by event type: artillery, mine, fire, capture, kill or loss, script, score, resources, pilot kill, turret target
static const long NET_CAUSE_REFIT_POINTS = -5;	// a weapon hit chunk's cause: a repair spending its points
static const uint32_t NET_SWEEP_AWAKE = 0x10000u;	// on a unit's index in a sweep
static const long NET_TURRET_SUBJECT = MAX_MULTIPLAYER_MOVERS;	// a WeaponFire batch's subject from here is a turret

static double netNow (void) {

	return(std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count());
}

// Until done, or the time is up. GNS keeps the connections alive on its own thread.
template<class Done>
static bool netWait (Done done) {

	const double giveUp = netNow() + NET_WAIT_SECONDS;
	while (netNow() < giveUp) {
		netSession->update();
		if (done())
			return(true);
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	return(false);
}

// The file, if it can be read: both ends must have the same one, or neither.
static uint32_t netHashFile (uint32_t hash, const char* path, unsigned long& total) {

	File file;
	if (file.open(path) != NO_ERR)
		return(netwire::hashBytes(hash, "none", 4));
	const unsigned long size = file.getLength();
	total += size;
	std::vector<unsigned char> bytes(size);
	if (size > 0)
		file.read(bytes.data(), size);
	file.close();
	return((size > 0) ? netwire::hashBytes(hash, bytes.data(), size) : hash);
}

// What two machines must agree on to play a mission: the mission, the units and their weapons,
// and the rules. So a mod, or another version of the data, is refused in the lobby, not found in play.
static uint32_t netDataHash (const char* missionName) {

	uint32_t hash = netwire::HASH_START;
	unsigned long total = 0;
	FullPathFileName path;
	// a lobby has no mission yet: each map is checked when it is chosen (netMapHash)
	if (missionName) {
		path.init(missionPath, missionName, ".fit");
		hash = netHashFile(hash, path, total);
	}
	path.init(objectPath, "object2", ".pak");
	hash = netHashFile(hash, path, total);
	path.init(missionPath, "gamesys", ".fit");
	hash = netHashFile(hash, path, total);
	printf("[NET] game data: %lu bytes, hash %08x\n", total, (unsigned)hash);
	fflush(stdout);
	return(hash);
}

static uint32_t netMapHash (const char* map) {

	unsigned long total = 0;
	FullPathFileName path;
	path.init(missionPath, map, ".fit");
	return(netHashFile(netwire::HASH_START, path, total));
}

//---------------------------------------------------------------------------

static unsigned long netHash (unsigned long hash, const char* text) {

	for (; *text; text++) {
		hash ^= (unsigned char)tolower((unsigned char)*text);
		hash = (hash * 16777619u) & 0xFFFFFFFFu;
	}
	return(hash);
}

// No lobby to set these in yet, so they are the map's.
static void netReadMissionSettings (MultiPlayer* player, const char* missionName) {

	strncpy(player->missionSettings.map, missionName, MAXLEN_MAP_NAME - 1);
	player->missionSettings.map[MAXLEN_MAP_NAME - 1] = 0;
	strncpy(player->missionSettings.name, missionName, MAXLEN_MAP_DESC - 1);	// its display name is in the lobby's data
	player->missionSettings.name[MAXLEN_MAP_DESC - 1] = 0;
	player->missionSettings.airStrike = player->missionSettings.guardTower = true;
	player->missionSettings.repairVehicle = player->missionSettings.recoveryTeam = true;
	player->missionSettings.sensorProbe = player->missionSettings.scoutCopter = true;
	player->missionSettings.mineLayer = player->missionSettings.resourceBuilding = true;
	player->missionSettings.timeLimit = -1.0f;
	player->missionSettings.missionType = MISSION_TYPE_ELIMINATION;
	player->missionSettings.maxPlayers = MAX_MC_PLAYERS;
	FullPathFileName path;
	path.init(missionPath, missionName, ".fit");
	FitIniFile file;
	if (file.open(path) != NO_ERR)
		return;
	if (file.seekBlock("MissionSettings") == NO_ERR) {
		float limit = 0.0f;
		if (file.readIdFloat("TimeLimit", limit) == NO_ERR)
			player->missionSettings.timeLimit = limit;
		unsigned long type = 0;
		if ((file.readIdULong("MissionType", type) == NO_ERR) && (type <= MISSION_TYPE_OTHER))
			player->missionSettings.missionType = (char)type;
		unsigned long most = 0;
		if ((file.readIdULong("MaximumNumberOfPlayers", most) == NO_ERR) && (most >= 1) && (most <= MAX_MC_PLAYERS))
			player->missionSettings.maxPlayers = (char)most;
		long points = 0;
		if ((file.readIdLong("ResourcePoints", points) == NO_ERR) && (points >= 0))
			player->missionSettings.resourcePoints = points;
		bool enabled = true;
		if (file.readIdBoolean("AirStrikesEnabledDefault", enabled) == NO_ERR)
			player->missionSettings.airStrike = enabled;
		if (file.readIdBoolean("MineLayersEnabledDefault", enabled) == NO_ERR)
			player->missionSettings.mineLayer = enabled;
		if (file.readIdBoolean("ScoutCoptersEnabledDefault", enabled) == NO_ERR)
			player->missionSettings.scoutCopter = enabled;
		if (file.readIdBoolean("SensorProbesEnabledDefault", enabled) == NO_ERR)
			player->missionSettings.sensorProbe = enabled;
		if (file.readIdBoolean("RepairVehicleEnabledDefault", enabled) == NO_ERR)
			player->missionSettings.repairVehicle = enabled;
		if (file.readIdBoolean("ArtilleryPieceEnabledDefault", enabled) == NO_ERR)
			player->missionSettings.guardTower = enabled;
		if (file.readIdBoolean("ResourceBuildingsEnabledDefault", enabled) == NO_ERR)
			player->missionSettings.resourceBuilding = enabled;
	}
	file.close();
	printf("[NET] mode %d, time limit %.0f\n", (int)player->missionSettings.missionType, player->missionSettings.timeLimit);
}

// No lobby to choose one in yet: the results and the units' labels show these.
static void netNamePlayers (MultiPlayer* player) {

	for (long i = 0; i < MAX_MC_PLAYERS; i++)
		if (player->playerInfo[i].commanderID > -1) {
			sprintf(player->playerInfo[i].name, "Player %ld", i + 1);
			player->playerInfo[i].teamSelected = player->playerInfo[i].team;	// the results show it
		}
}

static const char* netRejectName (netwire::RejectReason reason) {

	switch (reason) {
		case netwire::RejectReason::WrongVersion: return("another version of the game");
		case netwire::RejectReason::WrongData: return("another mission, or other game data");
		case netwire::RejectReason::GameFull: return("the game is full");
		case netwire::RejectReason::WrongRoster: return("other units in the mission");
		default: return("unknown");
	}
}

//---------------------------------------------------------------------------

static netlan::Beacon netBeacon;
static netwire::LanAnnounce netAnnounce;

// How many have joined, for whoever asks: until the game starts, and after it as in progress.
static void netUpdateBeacon (void) {

	if (!netBeacon.isOpen())
		return;
	netAnnounce.numPlayers = (uint8_t)(1 + netSession->connectedClients());
	if (netAnnounce.numPlayers > netAnnounce.maxPlayers)
		netAnnounce.numPlayers = netAnnounce.maxPlayers;
	netAnnounce.inProgress = netSession->allJoined();
	netBeacon.update(netAnnounce);
}

static void netStartBeacon (unsigned short port, long maxPlayers, const char* missionName) {

	netAnnounce.gamePort = port;
	netAnnounce.instance = std::random_device{}();
	netAnnounce.maxPlayers = (uint8_t)maxPlayers;
	netAnnounce.map = missionName;
	netAnnounce.name = (std::string(missionName) + " game").substr(0, netwire::MAX_NAME_LENGTH);
	std::vector<uint8_t> trial;
	if (!netwire::encode(netAnnounce, trial))
		printf("[NET] not announcing on the local network: this mission's name cannot be sent\n");
	else if (netBeacon.open())
		printf("[NET] announcing on the local network\n");
	else
		printf("[NET] not announcing on the local network: its port is taken\n");
	fflush(stdout);
}

// The first game of this mission with room in it, on the version this build speaks. With no
// mission, the first with room in it of any: a lobby's client takes the map it finds.
static bool netFindOnLan (const char* missionName, std::string& address, unsigned short& port) {

	netlan::Browser browser;
	if (!browser.open())
		return(false);
	printf("[NET] looking for %s on the local network\n", missionName ? missionName : "a game");
	fflush(stdout);
	std::vector<uint32_t> reported;
	const double giveUp = netNow() + NET_WAIT_SECONDS;
	while (netNow() < giveUp) {
		browser.update(netNow());
		for (const netlan::Found& found : browser.found()) {
			const netwire::LanAnnounce& game = found.announce;
			const char* skip = NULL;
			if (game.protocol != netwire::PROTOCOL_VERSION)
				skip = "another version of the game";
			else if (missionName && (S_stricmp(game.map.c_str(), missionName) != 0))
				skip = "another mission";
			else if (game.inProgress || (game.numPlayers >= game.maxPlayers))
				skip = "full, or already playing";
			if (skip) {
				// once each, or the wait says nothing about why
				if (std::find(reported.begin(), reported.end(), game.instance) == reported.end()) {
					reported.push_back(game.instance);
					printf("[NET] passing over %s at %s: %s\n", game.name.c_str(), found.address.c_str(), skip);
					fflush(stdout);
				}
				continue;
			}
			address = found.address;
			port = game.gamePort;
			printf("[NET] found %s at %s port %d, %d of %d players\n", game.name.c_str(), address.c_str(), (int)port, (int)game.numPlayers, (int)game.maxPlayers);
			fflush(stdout);
			return(true);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	return(false);
}

//---------------------------------------------------------------------------

void MultiPlayer::hostNetGame (long numPlayers, unsigned short port, const char* missionName, const char* dialAddress, unsigned short dialPort) {

	std::string error;
	netTransport = gos_net::createGnsTransport(&error);
	if (!netTransport)
		Fatal(0, (std::string(" -netlisten: no network: ") + error).c_str());
	netSession.reset(new netsession::Session(*netTransport));
	if (!netSession->host(port, (uint8_t)numPlayers, netDataHash(missionName)))
		Fatal(port, " -netlisten: cannot listen on this port ");
	startLocalHost(numPlayers);
	inSession = true;
	netReadMissionSettings(this, missionName);
	if (numPlayers > missionSettings.maxPlayers)
		Fatal(missionSettings.maxPlayers, " -nethost: more players than this map takes ");
	netNamePlayers(this);
	initUpdateFrequencies();
	printf("[NET] hosting %s on port %d for %ld players\n", missionName, (int)port, numPlayers);
	fflush(stdout);
	// not one who dials: nobody could reach this end in any case
	if (port && !dialAddress)
		netStartBeacon(port, numPlayers, missionName);
	// a client may not be listening yet, and a dial with nobody there gives up in about 10 s
	double nextDial = 0.0;
	const bool everyone = netWait([&]() {
		if (dialAddress && (netNow() >= nextDial)) {
			nextDial = netNow() + 15.0;
			if (!netSession->dialClient(dialAddress, dialPort))
				Fatal(0, " -netcall: needs an IP address ");
		}
		netUpdateBeacon();
		return netSession->allJoined();
	});
	if (!everyone)
		Fatal(0, " -netlisten: not everyone joined in time ");
}

//---------------------------------------------------------------------------

void MultiPlayer::joinNetGame (const char* address, unsigned short port, const char* missionName) {

	std::string error;
	netTransport = gos_net::createGnsTransport(&error);
	if (!netTransport)
		Fatal(0, (std::string(" -netjoin: no network: ") + error).c_str());
	netSession.reset(new netsession::Session(*netTransport));
	const uint32_t hash = netDataHash(missionName);
	std::string lanAddress;
	if (address && (strcmp(address, "lan") == 0)) {
		if (!netFindOnLan(missionName, lanAddress, port))
			Fatal(0, " -netjoin lan: no game of this mission found on the local network ");
		address = lanAddress.c_str();
	}
	if (address ? !netSession->join(address, port, hash, "player") : !netSession->joinWaiting(port, hash, "player"))
		Fatal(0, address ? " -netjoin: needs an IP address " : " -netjoin wait: cannot listen on this port ");
	printf("[NET] joining %s at %s port %d\n", missionName, address ? address : "whoever calls", (int)port);
	fflush(stdout);
	const bool answered = netWait([]() {
		return netSession->joined() || netSession->hostLost() || (netSession->rejected() != netwire::RejectReason::None);
	});
	if (!answered || netSession->hostLost())
		Fatal(0, " -netjoin: no answer from the host ");
	if (netSession->rejected() != netwire::RejectReason::None)
		Fatal(0, (std::string(" -netjoin: refused: ") + netRejectName(netSession->rejected())).c_str());
	startLocalHost(netSession->numPlayers());
	inSession = true;
	netReadMissionSettings(this, missionName);
	netNamePlayers(this);
	iAmHost = false;
	commanderID = netSession->commander();
	initUpdateFrequencies();
	printf("[NET] joined as commander %ld of %d\n", commanderID, (int)netSession->numPlayers());
	fflush(stdout);
}

//---------------------------------------------------------------------------

// What the wire checks is what these tables hold.
static_assert(netwire::MAX_LOBBY_COLORS == MAX_COLORS, "a lobby's colour indexes MultiPlayer::colors");
static_assert(netwire::MAX_PLAYERS == MAX_MC_PLAYERS, "a lobby's slot and commander index playerInfo");

// The screens fill MultiPlayer::colors from their layout (MPPrefs::initColors). A lobby with no
// screens has these, or every unit would be black whatever colour its player has.
static void netFillColors (void) {

	static const unsigned long colors[] = {0xFFC02020, 0xFF2040C0, 0xFF20A040, 0xFFC0A020, 0xFF8030A0, 0xFF20A0A0, 0xFFC06020, 0xFFA0A0A0};
	const long count = sizeof(colors) / sizeof(colors[0]);
	for (long i = 0; i < MAX_COLORS; i++)
		MultiPlayer::colors[i] = (long)colors[i % count];
}

// What a player is called, as a message can carry it and the game can hold it.
static std::string netPlayerName (void) {

	std::string name;
	for (const char* c = prefs.playerName[0]; *c && (name.size() < MAXLEN_PLAYER_NAME - 1); c++)
		if ((*c >= 0x20) && (*c <= 0x7E) && (*c != '%'))
			name += *c;
	return(name.empty() ? std::string("Player") : name);
}

static uint16_t netLobbyFlags (const MissionSettings& settings) {

	uint16_t flags = 0;
	if (settings.airStrike) flags |= netwire::LOBBY_AIRSTRIKE;
	if (settings.guardTower) flags |= netwire::LOBBY_GUARD_TOWER;
	if (settings.repairVehicle) flags |= netwire::LOBBY_REPAIR_VEHICLE;
	if (settings.recoveryTeam) flags |= netwire::LOBBY_RECOVERY_TEAM;
	if (settings.sensorProbe) flags |= netwire::LOBBY_SENSOR_PROBE;
	if (settings.scoutCopter) flags |= netwire::LOBBY_SCOUT_COPTER;
	if (settings.mineLayer) flags |= netwire::LOBBY_MINELAYER;
	if (settings.resourceBuilding) flags |= netwire::LOBBY_RESOURCE_BUILDINGS;
	if (settings.resourceForMechs) flags |= netwire::LOBBY_RESOURCE_FOR_MECHS;
	if (settings.unlimitedAmmo) flags |= netwire::LOBBY_UNLIMITED_AMMO;
	return(flags);
}

// What the lobby agreed, into what the game reads. Both ends get the same from the same state.
static void netApplyLobby (MultiPlayer* player, const netwire::LobbyState& state, const netwire::LobbyLaunch& launch) {

	MissionSettings& settings = player->missionSettings;
	settings.timeLimit = (float)state.timeLimit;
	settings.resourcePoints = state.resourcePoints;
	settings.airStrike = (state.flags & netwire::LOBBY_AIRSTRIKE) != 0;
	settings.guardTower = (state.flags & netwire::LOBBY_GUARD_TOWER) != 0;
	settings.repairVehicle = (state.flags & netwire::LOBBY_REPAIR_VEHICLE) != 0;
	settings.recoveryTeam = (state.flags & netwire::LOBBY_RECOVERY_TEAM) != 0;
	settings.sensorProbe = (state.flags & netwire::LOBBY_SENSOR_PROBE) != 0;
	settings.scoutCopter = (state.flags & netwire::LOBBY_SCOUT_COPTER) != 0;
	settings.mineLayer = (state.flags & netwire::LOBBY_MINELAYER) != 0;
	settings.resourceBuilding = (state.flags & netwire::LOBBY_RESOURCE_BUILDINGS) != 0;
	settings.resourceForMechs = (state.flags & netwire::LOBBY_RESOURCE_FOR_MECHS) != 0;
	settings.unlimitedAmmo = (state.flags & netwire::LOBBY_UNLIMITED_AMMO) != 0;

	// Teams from 0 with no gaps, as for commanders: Building::setTeamId clears any at or past numTeams.
	long teamIndex[netwire::MAX_PLAYERS];
	std::fill(teamIndex, teamIndex + netwire::MAX_PLAYERS, -1L);
	for (const netwire::LobbyPlayer& lobbyPlayer : state.players)
		if (launch.commanders[lobbyPlayer.slot] != netwire::NO_COMMANDER)
			teamIndex[lobbyPlayer.team] = 0;
	long numTeams = 0;
	for (long team = 0; team < netwire::MAX_PLAYERS; team++)
		if (teamIndex[team] == 0)
			teamIndex[team] = numTeams++;
	player->numTeams = numTeams;

	for (const netwire::LobbyPlayer& lobbyPlayer : state.players) {
		const uint8_t commander = launch.commanders[lobbyPlayer.slot];
		if (commander == netwire::NO_COMMANDER)
			continue;
		MC2Player& info = player->playerInfo[commander];
		strncpy(info.name, lobbyPlayer.name.c_str(), MAXLEN_PLAYER_NAME - 1);
		info.name[MAXLEN_PLAYER_NAME - 1] = 0;
		info.team = (char)teamIndex[lobbyPlayer.team];
		info.teamSelected = info.team;
		for (long i = 0; i < 3; i++)
			info.baseColor[i] = (char)lobbyPlayer.baseColor;
		info.stripeColor = (char)lobbyPlayer.stripeColor;
		printf("[NETLOBBY] commander %d: %s, team %d, colours %d and %d\n", (int)commander, info.name, (int)info.team, (int)lobbyPlayer.baseColor, (int)lobbyPlayer.stripeColor);
	}
	printf("[NETLOBBY] map %s, %ld teams, time limit %d, resource points %d, flags %x\n", state.map.c_str(), numTeams, (int)state.timeLimit, (int)state.resourcePoints, (unsigned)state.flags);
	fflush(stdout);
}

//---------------------------------------------------------------------------

void MultiPlayer::hostNetLobby (long maxPlayers, unsigned short port, const char* missionName, const char* dialAddress, unsigned short dialPort) {

	if (!missionName || !missionName[0])
		Fatal(0, " -netlobby: a host needs -mission ");
	std::string error;
	netTransport = gos_net::createGnsTransport(&error);
	if (!netTransport)
		Fatal(0, (std::string(" -netlobby: no network: ") + error).c_str());
	netSession.reset(new netsession::Session(*netTransport));
	// one who dials need not listen too, as for a game with no lobby
	if (!port && !dialAddress)
		port = netsession::DEFAULT_PORT;
	if (!netSession->hostLobby(port, (uint8_t)maxPlayers, netDataHash(NULL)))
		Fatal(port, " -netlobby: cannot listen on this port ");
	netReadMissionSettings(this, missionName);
	if (maxPlayers > missionSettings.maxPlayers)
		Fatal(missionSettings.maxPlayers, " -nethost: more players than this map takes ");

	netlobby::Lobby lobby((uint8_t)maxPlayers);
	if (!lobby.addPlayer(0, netPlayerName()))
		Fatal(0, " -netlobby: cannot add the host ");
	if (!lobby.setMap(missionName, netMapHash(missionName)))
		Fatal(0, " -netlobby: this map's name cannot be sent ");
	// anything under nothing is no time limit, in a map's own data too
	const int32_t timeLimit = (missionSettings.timeLimit < 0.0f) ? netwire::NO_TIME_LIMIT : (int32_t)missionSettings.timeLimit;
	if (!lobby.setSettings(timeLimit, (int32_t)missionSettings.resourcePoints, netLobbyFlags(missionSettings)))
		Fatal(0, " -netlobby: this map's settings cannot be sent ");
	netFillColors();
	printf("[NET] lobby for %s on port %d, for %ld players\n", missionName, (int)port, maxPlayers);
	fflush(stdout);
	// not one who dials: nobody could reach this end in any case
	if (port && !dialAddress)
		netStartBeacon(port, maxPlayers, missionName);

	uint32_t sentRevision = lobby.state().revision - 1;
	double nextDial = 0.0;
	const bool everyone = netWait([&]() {
		// a client may not be listening yet, and a dial with nobody there gives up in about 10 s
		if (dialAddress && (netNow() >= nextDial)) {
			nextDial = netNow() + 15.0;
			if (!netSession->dialClient(dialAddress, dialPort))
				Fatal(0, " -netcall: needs an IP address ");
		}
		netsession::LobbyEvent event;
		while (netSession->nextLobbyEvent(event)) {
			switch (event.type) {
				case netsession::LobbyEventType::Joined:
					lobby.addPlayer(event.commander, event.name);
					break;
				case netsession::LobbyEventType::Left:
					lobby.removePlayer(event.commander);
					break;
				case netsession::LobbyEventType::Choice:
					// a choice the rules refuse changes nothing, and the next state says so
					lobby.applyChoice(event.commander, event.choice);
					break;
			}
		}
		if (lobby.state().revision != sentRevision) {
			sentRevision = lobby.state().revision;
			if (!netSession->sendLobbyState(lobby.state())) {
				printf("[NET] the lobby's state did not reach everyone\n");
				fflush(stdout);
			}
		}
		netUpdateBeacon();
		return (lobby.state().players.size() == (size_t)maxPlayers) && lobby.canStart();
	});
	if (!everyone)
		Fatal(0, " -netlobby: not everyone joined and was ready in time ");

	netwire::LobbyLaunch launch;
	launch.numPlayers = lobby.commanders(launch.commanders);
	if (!netSession->launch(launch))
		Fatal(0, " -netlobby: could not launch ");
	startLocalHost(launch.numPlayers);
	inSession = true;
	netApplyLobby(this, lobby.state(), launch);
	initUpdateFrequencies();
}

//---------------------------------------------------------------------------

void MultiPlayer::joinNetLobby (const char* address, unsigned short port, char* missionName, size_t missionNameSize) {

	std::string error;
	netTransport = gos_net::createGnsTransport(&error);
	if (!netTransport)
		Fatal(0, (std::string(" -netjoin: no network: ") + error).c_str());
	netSession.reset(new netsession::Session(*netTransport));
	std::string lanAddress;
	if (address && (strcmp(address, "lan") == 0)) {
		if (!netFindOnLan(NULL, lanAddress, port))
			Fatal(0, " -netjoin lan: no game found on the local network ");
		address = lanAddress.c_str();
	}
	const uint32_t hash = netDataHash(NULL);
	const std::string name = netPlayerName();
	if (address ? !netSession->join(address, port, hash, name) : !netSession->joinWaiting(port, hash, name))
		Fatal(0, address ? " -netjoin: needs an IP address " : " -netjoin wait: cannot listen on this port ");
	printf("[NET] joining a lobby at %s port %d\n", address ? address : "whoever calls", (int)port);
	fflush(stdout);

	uint32_t answeredRevision = 0;
	bool answeredAny = false;
	std::string hashedMap;
	uint32_t mapHash = 0;
	const bool over = netWait([&]() {
		if (netSession->launched() || netSession->hostLost() || (netSession->rejected() != netwire::RejectReason::None))
			return true;
		if (!netSession->hasLobbyState())
			return false;
		const netwire::LobbyState& state = netSession->lobbyState();
		if (answeredAny && (state.revision == answeredRevision))
			return false;
		answeredAny = true;
		answeredRevision = state.revision;
		if (state.map != hashedMap) {
			hashedMap = state.map;
			mapHash = state.map.empty() ? 0 : netMapHash(state.map.c_str());
			// the host would never take this one's ready, and neither would know why
			if (!state.map.empty() && (mapHash != state.mapHash))
				Fatal(0, (std::string(" -netjoin: this machine has no copy of ") + state.map + ", or another version of it ").c_str());
		}
		// as soon as it can be: this is the lobby with nobody at the screen
		for (const netwire::LobbyPlayer& me : state.players)
			if ((me.slot == netSession->commander()) && !me.ready) {
				netwire::LobbyChoice choice;
				choice.revision = state.revision;
				choice.team = me.team;
				choice.baseColor = me.baseColor;
				choice.stripeColor = me.stripeColor;
				choice.ready = true;
				choice.mapHash = mapHash;
				netSession->sendLobbyChoice(choice);
			}
		return false;
	});
	if (!over || netSession->hostLost())
		Fatal(0, " -netjoin: the host went away, or never launched ");
	if (netSession->rejected() != netwire::RejectReason::None)
		Fatal(0, (std::string(" -netjoin: refused: ") + netRejectName(netSession->rejected())).c_str());

	const netwire::LobbyState& state = netSession->lobbyState();
	if (state.map.size() >= missionNameSize)
		Fatal(0, " -netjoin: the map's name is too long ");
	strcpy(missionName, state.map.c_str());
	netFillColors();
	startLocalHost(netSession->numPlayers());
	inSession = true;
	netReadMissionSettings(this, missionName);
	netApplyLobby(this, state, netSession->lobbyLaunch());
	iAmHost = false;
	commanderID = netSession->commander();
	initUpdateFrequencies();
	printf("[NET] launched as commander %ld of %d, on %s\n", commanderID, (int)netSession->numPlayers(), missionName);
	fflush(stdout);
}

//---------------------------------------------------------------------------

void MultiPlayer::waitForNetStart (void) {

	if (!netSession)
		return;

	// the units must be the same on every machine, in the same order: the roster
	// index is what names a unit on the wire
	unsigned long hash = 2166136261u;
	for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++)
		if (moverRoster[i]) {
			char line[64];
			sprintf(line, "%ld %ld ", i, moverRoster[i]->getCommanderId());
			hash = netHash(hash, line);
			hash = netHash(hash, moverRoster[i]->getName() ? moverRoster[i]->getName() : "");
		}

	if (iAmHost) {
		netSession->setRosterHash((uint32_t)hash);
		if (!netWait([]() { return netSession->allReady(); }) || !netSession->start())
			Fatal(0, " -netlisten: not everyone loaded the mission ");
		}
	else {
		netSession->sendReady((uint32_t)hash);
		const bool answered = netWait([]() {
			return netSession->started() || netSession->hostLost() || (netSession->rejected() != netwire::RejectReason::None);
		});
		if (!answered || netSession->hostLost())
			Fatal(0, " -netjoin: the host did not start the mission ");
		if (netSession->rejected() != netwire::RejectReason::None)
			Fatal(0, (std::string(" -netjoin: refused: ") + netRejectName(netSession->rejected())).c_str());
	}
	printf("[NET] mission start\n");
	printf("[NET] %ld turrets\n", ObjectManager ? ObjectManager->getNumTurrets() : 0L);
	for (long i = 0; ObjectManager && (i < ObjectManager->getNumTurrets()) && (i < 4); i++) {
		const Stuff::Vector3D at = ObjectManager->getTurret(i)->getPosition();
		printf("[NET] turret %ld at %.0f, %.0f, team %ld\n", i, at.x, at.y, ObjectManager->getTurret(i)->getTeamId());
	}
	fflush(stdout);
}

//---------------------------------------------------------------------------

bool MultiPlayer::isNetworked (void) {

	return(netSession != nullptr);
}

//---------------------------------------------------------------------------

long MultiPlayer::update (void) {

	processMessages();
	if (iAmHost) {
		updateClients();
		if (netSession)
			netUpdateBeacon();
	}
	return(MPLAYER_NO_ERR);
}

//---------------------------------------------------------------------------

long MultiPlayer::beginSessionScan (char* ipAddress, bool persistent) {

	return(MPLAYER_NO_ERR);
}

//---------------------------------------------------------------------------

long MultiPlayer::endSessionScan (void) {

	return(MPLAYER_NO_ERR);
}

//---------------------------------------------------------------------------

MC2Session* MultiPlayer::getSessions (long& sessionCount) {

	return(NULL);
}

//---------------------------------------------------------------------------

bool MultiPlayer::hostSession (char* sessionName, char* playerName, long mxPlayers) {

	return(true);
}

//---------------------------------------------------------------------------

void MultiPlayer::updateSessionData (MC2SessionData* sessionData) {

}

//---------------------------------------------------------------------------

void MultiPlayer::setLocked (bool set) {

}

//---------------------------------------------------------------------------

void MultiPlayer::setInProgress (bool set) {

}
//---------------------------------------------------------------------------

void MultiPlayer::setCancelled (bool set) {

}

//---------------------------------------------------------------------------

void MultiPlayer::addTeamScore (int teamID, int score) {

	if (!netSession || (teamID < 0) || (teamID >= MAX_TEAMS))
		return;
	teamScore[teamID] += score;
	addWorldEvent(this, netwire::WorldEventType::Score, teamID, 0, (unsigned long)(uint32_t)score);
}

//---------------------------------------------------------------------------

long MultiPlayer::joinSession (MC2Session* session, char* playerName) {

	return(MPLAYER_ERR_SESSION_NOT_FOUND);
}

//-----------------------------------------------------------------------------

long MultiPlayer::closeSession (void) {

	return 0;
}

//-----------------------------------------------------------------------------

void MultiPlayer::leaveSession (void) {

}

//-----------------------------------------------------------------------------

  long MultiPlayer::bootPlayer (NETPLAYER bootedPlayer) {

	return(MPLAYER_NO_ERR);
}

//-----------------------------------------------------------------------------

bool MultiPlayer::waitTillStartLoading (void) {

	return(false);
}

//-----------------------------------------------------------------------------

bool MultiPlayer::waitTillMechDataReceived (void) {

	return(false);
}

//-----------------------------------------------------------------------------

bool MultiPlayer::waitTillMissionLoaded (void) {

	return(false);
}

//-----------------------------------------------------------------------------

bool MultiPlayer::waitTillMissionSetup (void) {

	return(false);
}

//-----------------------------------------------------------------------------

bool MultiPlayer::waitForSessionEntry (void) {

	sessionEntry = -1;
	return(sessionEntry == 1);
}

//-----------------------------------------------------------------------------

bool MultiPlayer::playersReadyToLoad (void) {

	return(true);
}

//-----------------------------------------------------------------------------

bool MultiPlayer::launchBrowser (const char* link) {

	return(false);
}

//-----------------------------------------------------------------------------

void MultiPlayer::initParametersScreen (void) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::setDefaultPlayerInfo (void) {

}

//-----------------------------------------------------------------------------

long MultiPlayer::setClosestColor (long colorIndex, long commanderID) {

	long curIndex = colorIndex;
	return(curIndex);
}

//-----------------------------------------------------------------------------

long MultiPlayer::setNextFreeColor (long commanderID) {

	long i = 0;
	return(i);
}

//-----------------------------------------------------------------------------

void MultiPlayer::setPlayerBaseColor (long commanderID, long colorIndex) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::setPlayerTeam (long commanderID, long teamID) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::setColor (long colorIndex, long commanderID) {

}

//-----------------------------------------------------------------------------

MC2Player GetPlayersList[MAX_MC_PLAYERS];

const MC2Player* MultiPlayer::getPlayers (long& playerCount) {

	// with no network: no players (callers loop to playerCount)
	playerCount = 0;
	if (!netSession)
		return(NULL);
	for (long i = 0; i < MAX_MC_PLAYERS; i++)
		if (playerInfo[i].commanderID > -1)
			GetPlayersList[playerCount++] = playerInfo[i];
	return(GetPlayersList);
}

//-----------------------------------------------------------------------------

void MultiPlayer::logMessage (NETMESSAGE* message, bool sent) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::sendMessage (NETPLAYER player,
							   void* data,
							   int dataSize,
							   bool guaranteed,
							   bool toSelf) {

}

//-----------------------------------------------------------------------------

bool MultiPlayer::hostGame (char* sessionName, char* playerName, long nPlayers) {

	return(true);
}

//---------------------------------------------------------------------------

long MultiPlayer::joinGame (char* ipAddress, char* sessionName, char* playerName) {

	return(MPLAYER_NO_ERR);
}

//-----------------------------------------------------------------------------

void MultiPlayer::addToLocalMovers (MoverPtr mover) {

}

//---------------------------------------------------------------------------

void MultiPlayer::removeFromLocalMovers (MoverPtr mover) {

}

//---------------------------------------------------------------------------

void MultiPlayer::addToMoverRoster (MoverPtr mover) {

	for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++)
		if (!moverRoster[i]) {
			moverRoster[i] = mover;
			mover->setNetRosterIndex(i);
			return;
		}
	STOP(("MultiPlayer.addToMoverRoster: too many movers"));
}

//---------------------------------------------------------------------------

void MultiPlayer::removeFromMoverRoster (MoverPtr mover) {

}

//---------------------------------------------------------------------------

void MultiPlayer::addToPlayerMoverRoster (long playerCommanderID, MoverPtr mover) {

}

//---------------------------------------------------------------------------

void MultiPlayer::removeFromPlayerMoverRoster (MoverPtr mover) {

}

//---------------------------------------------------------------------------

void MultiPlayer::addToTurretRoster (TurretPtr turret) {

}

//---------------------------------------------------------------------------

// Generic HQs: a base each on a capture base map, the hill or the territories otherwise.
static std::vector<BuildingPtr> netHqs;

static float netDistanceSquared (const Stuff::Vector3D& a, float x, float y) {

	return((a.x - x) * (a.x - x) + (a.y - y) * (a.y - y));
}

void MultiPlayer::initSpecialBuildings (char commandersToLoad[8][3]) {

	if (!netSession)
		return;

	netHqs.clear();
	memset(mainHqBuildings, 0, sizeof(mainHqBuildings));
	for (long i = 0; i < ObjectManager->getNumBuildings(); i++) {
		BuildingPtr building = ObjectManager->getBuilding(i);
		if (building && (building->getObjectType()->getObjTypeNum() == GENERIC_HQ_BUILDING_OBJNUM))
			netHqs.push_back(building);
	}

	if (missionSettings.missionType == MISSION_TYPE_CAPTURE_BASE) {
		// Each team takes the free HQ nearest where its units start, in team order, so
		// that both ends come to the same answer. HQs nobody took are out of the game.
		std::vector<BuildingPtr> freeHqs = netHqs;
		for (long team = 0; team < numTeams; team++) {
			float x = 0.0f, y = 0.0f;
			long units = 0;
			for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++)
				if (moverRoster[i] && (moverRoster[i]->getTeamId() == team)) {
					x += moverRoster[i]->getPosition().x;
					y += moverRoster[i]->getPosition().y;
					units++;
				}
			if ((units == 0) || freeHqs.empty())
				continue;
			x /= units;
			y /= units;
			size_t best = 0;
			for (size_t i = 1; i < freeHqs.size(); i++)
				if (netDistanceSquared(freeHqs[i]->getPosition(), x, y) < netDistanceSquared(freeHqs[best]->getPosition(), x, y))
					best = i;
			BuildingPtr hq = freeHqs[best];
			freeHqs.erase(freeHqs.begin() + best);
			mainHqBuildings[team] = hq;
			hq->setCommanderId(team);
			hq->setTeamId(team, true);
			printf("[NET] team %ld's base is at %.0f, %.0f\n", team, hq->getPosition().x, hq->getPosition().y);
		}
		for (size_t i = 0; i < freeHqs.size(); i++)
			freeHqs[i]->setFlag(OBJECT_FLAG_CAPTURABLE, false);
	}
}

//---------------------------------------------------------------------------

BuildingPtr MultiPlayer::hqToTake (void) {

	if (missionSettings.missionType == MISSION_TYPE_CAPTURE_BASE) {
		for (long team = 0; team < numTeams; team++)
			if ((team != playerInfo[commanderID].team) && mainHqBuildings[team])
				return(mainHqBuildings[team]);
		return(NULL);
	}
	return(netHqs.empty() ? NULL : netHqs[0]);
}

//---------------------------------------------------------------------------

void MultiPlayer::stageHqCapture (void) {

	if (!netSession || !iAmHost)
		return;
	BuildingPtr target = hqToTake();
	if (!target) {
		printf("[NET] no HQ to take on this map\n");
		return;
	}
	addCaptureBuildingChunk(target, target->getCommanderId(), commanderID);
	if (missionSettings.missionType == MISSION_TYPE_CAPTURE_BASE)
		target->setFlag(OBJECT_FLAG_CAPTURABLE, false);
	target->setCommanderId(commanderID);
	target->setTeamId(playerInfo[commanderID].team, false);
	printf("[NET] the host's team takes the HQ at %.0f, %.0f\n", target->getPosition().x, target->getPosition().y);
}

//***************************************************************************
// WORLD CHUNK maintenance functions
//***************************************************************************

long MultiPlayer::addWorldChunk (WorldChunkPtr chunk) {

	return(0);
}

//---------------------------------------------------------------------------

static void addWorldEvent (MultiPlayer* player, netwire::WorldEventType type, long a, long b, unsigned long value) {

	if (!netSession || !player->isServer() || (a < 0) || (b < 0))
		return;
	netwire::WorldEvent event;
	event.type = type;
	event.a = (uint16_t)a;
	event.b = (uint16_t)b;
	event.value = (uint32_t)value;
	uint32_t words[netwire::WORLD_EVENT_WORDS];
	if ((a > netwire::MAX_WORLD_FIELD) || (b > netwire::MAX_WORLD_FIELD) || !netwire::encodeWorldEvent(event, words))
		return;
	if ((player->numWorldChunks + (long)netwire::WORLD_EVENT_WORDS) > MAX_WORLD_CHUNKS)
		player->flushWorldEvents();
	player->worldChunks[player->numWorldChunks++] = words[0];
	player->worldChunks[player->numWorldChunks++] = words[1];
	player->worldStateLoad++;
}

//---------------------------------------------------------------------------

long MultiPlayer::addMissionScriptMessageChunk (long code, long param) {

	if ((param < 0) || (param > 0xFFFFFF))
		printf("[NET] mission script message %ld: parameter %ld does not fit, not sent\n", code, param);
	else
		addWorldEvent(this, netwire::WorldEventType::ScriptMessage, param & netwire::MAX_WORLD_FIELD, param >> 12, (unsigned long)code);
	return(0);
}

//---------------------------------------------------------------------------

long MultiPlayer::addArtilleryChunk (long commanderId, long artilleryType, Stuff::Vector3D location, long seconds)
{
	int row = 0, col = 0;
	land->worldToCell(location, row, col);
	addWorldEvent(this, netwire::WorldEventType::Artillery, row, col,
		((unsigned long)commanderId & 0xFF) | (((unsigned long)artilleryType & 0xFF) << 8) | (((unsigned long)seconds & 0xFFFF) << 16));
	return(0);
}

//---------------------------------------------------------------------------

long MultiPlayer::addMineChunk (long tileR, long tileC, long teamId, long mineState, long explosionType) {

	addWorldEvent(this, netwire::WorldEventType::Mine, tileR, tileC,
		((unsigned long)mineState & 0xFF) | (((unsigned long)explosionType & 0xFF) << 8));
	return(0);
}

//---------------------------------------------------------------------------

long MultiPlayer::addLightOnFireChunk (GameObjectPtr object, long seconds) {

	if (object)
		addWorldEvent(this, netwire::WorldEventType::TerrainFire, seconds, 0, (unsigned long)object->getPartId());
	return(0);
}

//---------------------------------------------------------------------------

long MultiPlayer::addPilotKillStat (MoverPtr mover, long vehicleClass) {

	if (mover && (vehicleClass >= 0))
		addWorldEvent(this, netwire::WorldEventType::PilotKill, mover->getNetRosterIndex(), vehicleClass, 0);
	return(0);
}

//---------------------------------------------------------------------------

long MultiPlayer::addScoreChunk (long commanderID, long score) {

	return(0);
}

//---------------------------------------------------------------------------

long MultiPlayer::addKillLossChunk (long killerCID, long loserCID) {

	if (netSession) {
		if ((killerCID >= 0) && (killerCID < MAX_MC_PLAYERS))
			playerInfo[killerCID].kills++;
		if ((loserCID >= 0) && (loserCID < MAX_MC_PLAYERS))
			playerInfo[loserCID].losses++;
		addWorldEvent(this, netwire::WorldEventType::KillLoss, killerCID, loserCID, 0);
	}
	return(0);
}

//---------------------------------------------------------------------------

long MultiPlayer::addEndMissionChunk (void) {

	return(0);
}

//---------------------------------------------------------------------------

long MultiPlayer::addCaptureBuildingChunk (BuildingPtr building, long prevCommanderID, long newCommanderID) {

	if (building && (newCommanderID >= 0) && (newCommanderID < MAX_MC_PLAYERS))
		addWorldEvent(this, netwire::WorldEventType::Capture, newCommanderID, playerInfo[newCommanderID].team, (unsigned long)building->getPartId());
	return(0);
}

//---------------------------------------------------------------------------

long MultiPlayer::grabWorldChunks (unsigned long* packedChunkBuffer) {

	return(0);
}

//---------------------------------------------------------------------------
// WEAPON HIT CHUNK maintenance functions
//---------------------------------------------------------------------------

long MultiPlayer::addWeaponHitChunk (WeaponHitChunkPtr chunk) {

	weaponHitLoad++;
	if (!netSession)
		return(0);
	if (numWeaponHitChunks == MAX_WEAPONHIT_CHUNKS)
		sendWeaponHitUpdate();
	chunk->pack();
	weaponHitChunks[numWeaponHitChunks++] = chunk->data;
	return(numWeaponHitChunks);
}

//---------------------------------------------------------------------------

static void netSendArmour (MultiPlayer* player, MoverPtr mover);

long MultiPlayer::addWeaponHitChunk (GameObjectPtr target, WeaponShotInfoPtr shotInfo, bool isRefit) {

	if (!netSession) {
		weaponHitLoad++;
		return(0);
	}
	WeaponHitChunk chunk;
	chunk.init();
	chunk.build(target, shotInfo, isRefit);
	const long result = addWeaponHitChunk(&chunk);
	// A repair shares its points with the ammo that needs refilling, and a client's ammo can be
	// a moment behind the host's, so running it there can come out differently. Armour first
	// differed on a client that way, in a battle in CI. The host has repaired by now: say what came of it.
	if (isRefit && target && target->isMover()) {
		sendWeaponHitUpdate();
		netSendArmour(this, (MoverPtr)target);
	}
	return(result);
}

//---------------------------------------------------------------------------

// A word in the weapon hit stream with the target type a chunk never uses: a critical hit.
static const unsigned long NET_STREAM_CRITICAL = 3;

void MultiPlayer::addCriticalHitToStream (MoverPtr mover, unsigned char chunkData) {

	moverCriticalLoad++;
	if (numWeaponHitChunks == MAX_WEAPONHIT_CHUNKS)
		sendWeaponHitUpdate();
	weaponHitChunks[numWeaponHitChunks++] = NET_STREAM_CRITICAL | (((unsigned long)mover->getNetRosterIndex() & WEAPONHITCHUNK_MOVERINDEX_MASK) << WEAPONHITCHUNK_TARGETTYPE_BITS) | ((unsigned long)chunkData << (WEAPONHITCHUNK_TARGETTYPE_BITS + WEAPONHITCHUNK_MOVERINDEX_BITS));
}

//---------------------------------------------------------------------------

void MultiPlayer::grabWeaponHitChunks (unsigned long* packedChunkBuffer, long numChunks) {

}

//---------------------------------------------------------------------------
// MESSAGE SENDERS
//---------------------------------------------------------------------------

void MultiPlayer::sendPlayerInfo (NETPLAYER receiver) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendPlayerCID (NETPLAYER receiver, unsigned char subType, char CID) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendPlayerUpdate (NETPLAYER receiver, long stage, long newCommanderID) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendMissionSettingsUpdate (NETPLAYER receiver) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendChat (NETPLAYER receiver, char team, char* chatMessage) {

}

void MultiPlayer::sendPlayerActionChat(NETPLAYER receiver, const char* playerName, unsigned long resID )
{
}


//---------------------------------------------------------------------------

void MultiPlayer::sendPlayerCheckIn (void) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendPlayerSetup (void) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendPlayerInsignia (char* insigniaFileName, unsigned char* insigniaData, long dataSize) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendMissionSetup (NETPLAYER receiver, long subType, CompressedMech* mechData) {

}

//---------------------------------------------------------------------------

void MultiPlayer::setServer (NETPLAYER player, char playerIPAddress[16]) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendStartMission (void) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendEndMission (long result) {

}

//---------------------------------------------------------------------------
extern MoverPtr BringInReinforcement (long vehicleID, long rosterIndex, long commanderID, Stuff::Vector3D pos, bool exists);

// Support vehicles and recoveries. The host decides everything: it holds every commander's
// resource points, and takes the money when a request reaches it. A commander's own machine
// has taken it already, at the button (ControlGui::handleVehicleClick), so that the button
// and the number on screen need no round trip, and gets it back if the host says no.
static bool netSupportUnit[MAX_MULTIPLAYER_MOVERS] = {false};	// on the host: does not keep a team in the game
static bool netSupportAsked = false;	// on a client: one request at a time, as there is one button
static long netSupportCounts[5] = {0};	// vehicles begun, dropped, recoveries begun, mechs recovered, refusals

static netwire::SupportEvent netSupportEvent (netwire::SupportStage stage, long commander, long rosterIndex, long vehicleId, long cost, const Stuff::Vector3D& pos) {

	netwire::SupportEvent event;
	event.stage = stage;
	event.commander = (uint8_t)commander;
	event.rosterIndex = (uint16_t)rosterIndex;
	event.vehicleId = (uint16_t)vehicleId;
	event.cost = (int32_t)cost;
	event.x = pos.x;
	event.y = pos.y;
	event.z = pos.z;
	return(event);
}

static void netSendSupport (MultiPlayer* player, const netwire::SupportEvent& event) {

	uint32_t words[netwire::SUPPORT_EVENT_WORDS];
	if (!netSession || !netwire::encodeSupportEvent(event, words))
		return;
	// what came before this must arrive before it: a hit landed before a recovery does not count after it
	player->sendWeaponHitUpdate();
	player->flushWorldEvents();
	netwire::ChunkBatch batch;
	batch.kind = netwire::ChunkKind::Support;
	batch.words.assign(words, words + netwire::SUPPORT_EVENT_WORDS);
	netSession->sendToClients(gos_net::Delivery::ReliableOrdered, batch);
}

// A shot or a blast set off a mine. Every machine sees the shot land, but each would see it
// land somewhere a little different, so the host decides and tells the others.
void SetOffMine (long row, long col) {

	if (MPlayer && !MPlayer->isServer())
		return;
	GameMap->setMine(row, col, 2);
	if (MPlayer)
		MPlayer->addMineChunk(row, col, 1, 2, 0);
}

//---------------------------------------------------------------------------

// What a hit reads on a unit, exact: armour, structure, and each location's damage and critical
// hits, which decide where the next hit goes. A wreck's are allowed to differ between machines,
// since nothing reads them (applyNetCheck), until a recovery brings it back. Recovery itself
// makes the components whole everywhere (Mover::recover).
static void netSendArmour (MultiPlayer* player, MoverPtr mover) {

	if (!netSession)
		return;
	netwire::ChunkBatch batch;
	batch.kind = netwire::ChunkKind::Support;
	batch.subject = (uint16_t)mover->getNetRosterIndex();
	const auto add = [&batch](float value) {
		uint32_t word = 0;
		memcpy(&word, &value, 4);
		batch.words.push_back(word);
	};
	for (long i = 0; i < mover->numArmorLocations; i++)
		add(mover->armor[i].curArmor);
	for (long i = 0; i < mover->numBodyLocations; i++) {
		add(mover->body[i].curInternalStructure);
		uint32_t state = mover->body[i].damageState;
		// a slot past a location's parts is never set, so not read as a bool
		for (long j = 0; (j < mover->body[i].totalSpaces) && (j < MAX_CRITSPACES_PER_BODYLOCATION); j++) {
			unsigned char hit = 0;
			memcpy(&hit, &mover->body[i].criticalSpaces[j].hit, 1);
			if (hit == 1)
				state |= 0x100u << j;
		}
		batch.words.push_back(state);
	}
	netSession->sendToClients(gos_net::Delivery::ReliableOrdered, batch);
}

static void netApplyArmour (MultiPlayer* player, const netwire::ChunkBatch& batch) {

	MoverPtr mover = (batch.subject < MAX_MULTIPLAYER_MOVERS) ? player->moverRoster[batch.subject] : NULL;
	if (!mover || (batch.words.size() != (size_t)(mover->numArmorLocations + 2 * mover->numBodyLocations)))
		return;
	const auto get = [&batch](size_t i) {
		float value = 0.0f;
		memcpy(&value, &batch.words[i], 4);
		return(value);
	};
	for (long i = 0; i < mover->numArmorLocations; i++) {
		const float value = get(i);
		if ((value >= 0.0f) && (value <= mover->armor[i].maxArmor))
			mover->armor[i].curArmor = value;
	}
	for (long i = 0; i < mover->numBodyLocations; i++) {
		const size_t at = mover->numArmorLocations + 2 * i;
		const float value = get(at);
		if ((value >= 0.0f) && (value <= mover->body[i].maxInternalStructure))
			mover->body[i].curInternalStructure = value;
		const uint32_t state = batch.words[at + 1];
		if ((state & 0xFFu) <= IS_DAMAGE_DESTROYED)
			mover->body[i].damageState = (unsigned char)(state & 0xFFu);
		for (long j = 0; (j < mover->body[i].totalSpaces) && (j < MAX_CRITSPACES_PER_BODYLOCATION); j++)
			mover->body[i].criticalSpaces[j].hit = ((state >> (8 + j)) & 1u) != 0;
	}
	if (mover->getObjectClass() == BATTLEMECH) {
		((BattleMechPtr)mover)->calcLegStatus();
		((BattleMechPtr)mover)->calcTorsoStatus();
	}
}

//---------------------------------------------------------------------------

// The host's own count of a commander's points, for all to see. What a machine's commander
// spent is not taken from its own screen twice (applyWorldEvent).
static void netHostResources (MultiPlayer* player, long commander, long points) {

	player->playerInfo[commander].resourcePoints += points;
	if (points > 0)
		player->playerInfo[commander].resourcePointsGained += points;
	addWorldEvent(player, netwire::WorldEventType::Resources, commander, 0, (unsigned long)(uint32_t)(int32_t)points);
}

static void netApplyRefused (MultiPlayer* player, const netwire::SupportEvent& event) {

	netSupportCounts[4]++;
	if (event.commander != player->commanderID)
		return;
	netSupportAsked = false;
	LogisticsData::instance->addResourcePoints(event.cost);
	player->refundResources(event.cost);
	if (ControlGui::instance)
		ControlGui::instance->unPressAllVehicleButtons();
}

static void netHostRefuse (MultiPlayer* player, long commander, long vehicleId, long cost) {

	const Stuff::Vector3D nowhere(0.0f, 0.0f, 0.0f);
	const netwire::SupportEvent event = netSupportEvent(netwire::SupportStage::Refused, commander, 0, vehicleId, (cost > 0) ? cost : 0, nowhere);
	netApplyRefused(player, event);
	netSendSupport(player, event);
}

// Every machine: the unit is there, not yet existing, while the VTOL flies it in.
static bool netBeginVehicle (MultiPlayer* player, const netwire::SupportEvent& event) {

	const Stuff::Vector3D where(event.x, event.y, event.z);
	Stuff::Vector3D pos = where;
	MoverPtr mover = BringInReinforcement(event.vehicleId, 255, event.commander, where, false);
	if (!mover)
		return(false);
	player->reinforcements[event.commander][0] = mover->getNetRosterIndex();
	MissionInterfaceManager::instance()->endVtol(event.commander);
	MissionInterfaceManager::instance()->beginVtol(event.vehicleId, event.commander, &pos);
	netSupportCounts[0]++;
	return(true);
}

static void netBeginRecovery (MultiPlayer* player, const netwire::SupportEvent& event) {

	Stuff::Vector3D pos(event.x, event.y, event.z);
	player->reinforcements[event.commander][1] = event.rosterIndex;
	strncpy(player->reinforcementPilot[event.commander], LogisticsData::instance->getBestPilot(player->moverRoster[event.rosterIndex]->tonnage), 31);
	player->reinforcementPilot[event.commander][31] = 0;
	MissionInterfaceManager::instance()->endVtol(event.commander);
	MissionInterfaceManager::instance()->beginVtol(event.vehicleId, event.commander, &pos);
	netSupportCounts[2]++;
}

static bool netSeenPlace (long commander, const Stuff::Vector3D& pos) {

	int tileRow = -1, tileCol = -1;
	land->worldToTile(pos, tileRow, tileCol);
	if ((tileRow < 0) || (tileRow >= land->realVerticesMapSide) || (tileCol < 0) || (tileCol >= land->realVerticesMapSide))
		return(false);
	return(Commander::commanders[commander]->getTeam()->teamLineOfSight(pos, 0.0f));
}

static bool netSupportEnabled (const MultiPlayer* player, long vehicleId) {

	switch (vehicleId) {
		case 182: return(player->missionSettings.repairVehicle);
		case 393: return(player->missionSettings.scoutCopter);
		case 120: return(player->missionSettings.mineLayer);
		case 147: return(player->missionSettings.recoveryTeam);
		case 415: return(player->missionSettings.guardTower);
	}
	return(false);
}

// The host: a commander's request, from the network or from the host's own interface,
// checked as the interface checks it there (canAddVehicle, canRecover).
static void netHostRequest (MultiPlayer* player, long commander, long vehicleId, long rosterIndex, Stuff::Vector3D pos, bool recovery) {

	const long cost = ControlGui::supportCost(vehicleId);
	const bool own = (commander == player->commanderID);
	bool allowed = (cost >= 0) && netSupportEnabled(player, vehicleId) && (recovery == (vehicleId == 147))
		&& Commander::commanders[commander] && Commander::commanders[commander]->getTeam()
		&& !MissionInterfaceManager::instance()->isVtolBusy(commander)
		&& (own || (player->playerInfo[commander].resourcePoints >= cost));
	if (allowed)
		allowed = netSeenPlace(commander, pos);
	if (allowed && recovery) {
		MoverPtr mech = ((rosterIndex >= 0) && (rosterIndex < MAX_MULTIPLAYER_MOVERS)) ? player->moverRoster[rosterIndex] : NULL;
		// a unit with no pilot has been released, and is not a wreck to recover
		allowed = mech && mech->getPilot() && (mech->getObjectClass() == BATTLEMECH) && mech->isDisabled() && !mech->isDestroyed();
		// one wreck, one Karnov: a second would take it from whoever recovered it first
		for (long i = 0; i < MAX_MC_PLAYERS; i++)
			if (player->reinforcements[i][1] == rosterIndex)
				allowed = false;
	}
	else if (allowed) {
		allowed = GameMap->getPassable(pos) && !land->getWater(pos);
		long free = 0;
		for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++)
			if (!player->moverRoster[i])
				free++;
		allowed = allowed && (free > 0);
	}
	if (!allowed) {
		printf("[NET] commander %ld is refused vehicle %ld at %.0f, %.0f\n", commander, vehicleId, pos.x, pos.y);
		netHostRefuse(player, commander, vehicleId, cost);
		return;
	}
	pos.z = land->getTerrainElevation(pos);
	netwire::SupportEvent event = netSupportEvent(recovery ? netwire::SupportStage::BeginRecovery : netwire::SupportStage::BeginVehicle, commander, rosterIndex, vehicleId, cost, pos);
	if (recovery)
		netBeginRecovery(player, event);
	else {
		if (!netBeginVehicle(player, event)) {
			netHostRefuse(player, commander, vehicleId, cost);	// no vehicle left in the pool
			return;
		}
		event.rosterIndex = (uint16_t)player->reinforcements[commander][0];
		netSupportUnit[event.rosterIndex] = true;
	}
	if (!own)
		netHostResources(player, commander, -cost);
	netSendSupport(player, event);
}

bool MultiPlayer::supportRequestPending (void) {

	return(netSupportAsked);
}

//---------------------------------------------------------------------------

void MultiPlayer::refundResources (long cost) {

	if (iAmHost)
		netHostResources(this, commanderID, cost);
	else
		playerInfo[commanderID].resourcePoints += cost;
}

void MultiPlayer::sendReinforcement (long vehicleID, long rosterIndex, const char pilotName[16], long commanderID, Stuff::Vector3D pos, unsigned char stage) {

	if ((commanderID < 0) || (commanderID >= MAX_MC_PLAYERS))
		return;
	switch (stage) {
		case (unsigned char)netwire::SupportStage::RequestVehicle:
		case (unsigned char)netwire::SupportStage::RequestRecovery: {
			const bool recovery = (stage == (unsigned char)netwire::SupportStage::RequestRecovery);
			if (iAmHost) {
				netHostRequest(this, commanderID, vehicleID, rosterIndex, pos, recovery);
				break;
			}
			uint32_t words[netwire::SUPPORT_EVENT_WORDS];
			const netwire::SupportEvent event = netSupportEvent((netwire::SupportStage)stage, commanderID, (rosterIndex == 255) ? 0 : rosterIndex, vehicleID, 0, pos);
			if (!netSession || netSupportAsked || !netwire::encodeSupportEvent(event, words))
				break;
			netSupportAsked = true;
			// an order with no unit: it is the commander's own
			netwire::ChunkBatch batch;
			batch.kind = netwire::ChunkKind::Order;
			batch.words.assign(words, words + netwire::SUPPORT_EVENT_WORDS);
			netSession->sendToHost(gos_net::Delivery::ReliableOrdered, batch);
			break;
		}
		case (unsigned char)netwire::SupportStage::DropVehicle: {
			if (!iAmHost || (rosterIndex < 0) || (rosterIndex >= MAX_MULTIPLAYER_MOVERS) || !moverRoster[rosterIndex] || !moverRoster[rosterIndex]->getPilot())
				break;
			moverRoster[rosterIndex]->setExists(true);
			netSupportCounts[1]++;
			netSendSupport(this, netSupportEvent(netwire::SupportStage::DropVehicle, commanderID, rosterIndex, vehicleID, 0, pos));
			break;
		}
		case (unsigned char)netwire::SupportStage::Recovered: {
			if (!iAmHost || (rosterIndex < 0) || (rosterIndex >= MAX_MULTIPLAYER_MOVERS) || !moverRoster[rosterIndex])
				break;
			netSendSupport(this, netSupportEvent(netwire::SupportStage::Recovered, commanderID, rosterIndex, vehicleID, 0, pos));
			netSupportCounts[3]++;
			MissionInterfaceManager::instance()->recoverMech(commanderID, moverRoster[rosterIndex]);
			netSendArmour(this, moverRoster[rosterIndex]);
			break;
		}
		case 6: // resource points, in vehicleID
			if (iAmHost)
				netHostResources(this, commanderID, vehicleID);
			else if (vehicleID < 0)
				playerInfo[commanderID].resourcePoints += vehicleID;	// the button: the host takes them on the request
			if (iAmHost && (commanderID == this->commanderID) && (vehicleID > 0))
				LogisticsData::instance->addResourcePoints(vehicleID);
			break;
		default:
			// Stage 7 dropped a dead unit from its team, on the icon's death animation. A wreck
			// stays on the roster in single player too, and can still be recovered.
			break;
	}
}

//-----------------------------------------------------------------------------

void MultiPlayer::sendNewServer (void) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::sendLeaveSession (char subType, char commanderID) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::sendPlayerOrder (TacticalOrderPtr tacOrder,
								   bool needsSelection,
								   long numMovers,
								   MoverPtr* moverList,
								   long numGroups,
								   MoverGroupPtr* groupList,
   								   bool queuedOrder) {

	if (!netSession || iAmHost)
		return;
	for (long i = 0; i < numMovers; i++) {
		MoverPtr mover = moverList[i];
		if (!mover || (mover->getNetRosterIndex() < 0))
			continue;
		TacticalOrder order = *tacOrder;
		if (order.targetIsGone())
			continue;
		order.attackParams.range = (FireRangeType)mover->attackRange;
		order.pack(NULL, NULL);
		netwire::ChunkBatch batch;
		batch.kind = netwire::ChunkKind::Order;
		batch.subject = (uint16_t)mover->getNetRosterIndex();
		batch.words.push_back((uint32_t)order.data[0]);
		batch.words.push_back((uint32_t)order.data[1]);
		batch.words.push_back(queuedOrder ? 1u : 0u);
		netSession->sendToHost(gos_net::Delivery::ReliableOrdered, batch);
	}
}

//---------------------------------------------------------------------------

void MultiPlayer::sendHoldPosition (void) {

	if (!netSession || iAmHost || !Team::home || !Commander::home)
		return;
	const bool fromCurrentPos = ControlGui::instance && ControlGui::instance->getFireFromCurrentPos();
	for (long i = 0; i < Team::home->getRosterSize(); i++) {
		MoverPtr mover = Team::home->getMover(i);
		if (!mover || !mover->isSelected() || (mover->getCommander() != Commander::home) || (mover->getNetRosterIndex() < 0))
			continue;
		// one word: an order with only a range and whether to pursue
		netwire::ChunkBatch batch;
		batch.kind = netwire::ChunkKind::Order;
		batch.subject = (uint16_t)mover->getNetRosterIndex();
		const bool pursue = (mover->attackRange != FIRERANGE_CURRENT) && !fromCurrentPos;
		batch.words.push_back((uint32_t)(mover->attackRange - FIRERANGE_DEFAULT) | (pursue ? 0x100u : 0u));
		netSession->sendToHost(gos_net::Delivery::ReliableOrdered, batch);
	}
}

//---------------------------------------------------------------------------

void MultiPlayer::sendPlayerMoverGroup (long groupId,
										long numMovers,
										MoverPtr* moverList,
										long point) {

}

//---------------------------------------------------------------------------

void MultiPlayer::sendPlayerArtillery (long strikeType, Stuff::Vector3D location, long seconds) {

	if (!netSession || iAmHost)
		return;
	int row = 0, col = 0;
	land->worldToCell(location, row, col);
	// an order with no unit: it is the commander's own
	netwire::ChunkBatch batch;
	batch.kind = netwire::ChunkKind::Order;
	batch.words.push_back((uint32_t)strikeType);
	batch.words.push_back((uint32_t)row);
	batch.words.push_back((uint32_t)col);
	batch.words.push_back((uint32_t)seconds);
	netSession->sendToHost(gos_net::Delivery::ReliableOrdered, batch);
}


//---------------------------------------------------------------------------

void MultiPlayer::sendMoverUpdate (void) {

	if (!netSession)
		return;
	// newest wins, so a sweep carries its number, and all of it fits in one packet
	netwire::ChunkBatch batch;
	batch.kind = netwire::ChunkKind::Move;
	static uint32_t sweep = 0;	// moverUpdateId is 16 bits: it would wrap in hours, and look old
	batch.words.push_back(++sweep);
	for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++) {
		MoverPtr mover = moverRoster[i];
		if (!mover || !mover->getExists() || !mover->getPilot())
			continue;
		mover->buildMoveChunk();
		mover->buildStatusChunk();
		// a unit that sleeps on a client neither moves nor fires there
		batch.words.push_back((uint32_t)i | (mover->getAwake() ? NET_SWEEP_AWAKE : 0u));
		batch.words.push_back((uint32_t)mover->moveChunk.data);
		batch.words.push_back((uint32_t)mover->statusChunk.data);
	}
	netSession->sendToClients(gos_net::Delivery::Unreliable, batch);
}

//---------------------------------------------------------------------------

// A turret's target, when it changes. The host fires it, but a client turns it to face.
void MultiPlayer::sendTurretUpdate (void) {

	static std::vector<GameObjectWatchID> last;
	const long count = ObjectManager->getNumTurrets();
	if ((long)last.size() != count)
		last.assign(count, 0);
	for (long i = 0; i < count; i++) {
		TurretPtr turret = ObjectManager->getTurret(i);
		if (turret->targetWID == last[i])
			continue;
		last[i] = turret->targetWID;
		GameObjectPtr target = turret->targetWID ? ObjectManager->getByWatchID(turret->targetWID) : NULL;
		// "Turrets may only target movers and camera drones": a drone a client need not face
		const long index = (target && target->isMover()) ? ((MoverPtr)target)->getNetRosterIndex() : -1;
		addWorldEvent(this, netwire::WorldEventType::TurretTarget, i, index + 1, 0);
	}
}

//---------------------------------------------------------------------------

void MultiPlayer::sendMoverWeaponFireUpdate (void) {

	if (!netSession)
		return;
	unsigned long buffer[MAX_WEAPONFIRE_CHUNKS];
	for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++) {
		MoverPtr mover = moverRoster[i];
		if (!mover)
			continue;
		const long count = mover->grabWeaponFireChunks(CHUNK_SEND, buffer, MAX_WEAPONFIRE_CHUNKS);
		if (count < 1)
			continue;
		moverWeaponFireLoad += count;
		netwire::ChunkBatch batch;
		batch.kind = netwire::ChunkKind::WeaponFire;
		batch.subject = (uint16_t)i;
		batch.words.assign(buffer, buffer + count);
		netSession->sendToClients(gos_net::Delivery::ReliableUnordered, batch);
	}
}

//---------------------------------------------------------------------------

void MultiPlayer::sendTurretWeaponFireUpdate (void) {

	if (!netSession)
		return;
	unsigned long buffer[MAX_WEAPONFIRE_CHUNKS];
	for (long i = 0; i < ObjectManager->getNumTurrets(); i++) {
		TurretPtr turret = ObjectManager->getTurret(i);
		const long count = turret->grabWeaponFireChunks(CHUNK_SEND, buffer);
		if (count < 1)
			continue;
		turret->clearWeaponFireChunks(CHUNK_SEND);
		turretWeaponFireLoad += count;
		netwire::ChunkBatch batch;
		batch.kind = netwire::ChunkKind::WeaponFire;
		batch.subject = (uint16_t)(NET_TURRET_SUBJECT + i);
		batch.words.assign(buffer, buffer + count);
		netSession->sendToClients(gos_net::Delivery::ReliableUnordered, batch);
	}
}

//---------------------------------------------------------------------------

// Radio only: critical hits go with the weapon hits (addCriticalHitToStream).
void MultiPlayer::sendMoverCriticalUpdate (void) {

	if (!netSession)
		return;
	unsigned char radio[MAX_RADIO_CHUNKS];
	for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++) {
		MoverPtr mover = moverRoster[i];
		if (!mover)
			continue;
		const long radioCount = mover->grabRadioChunks(CHUNK_SEND, radio);
		if (radioCount > 0) {
			mover->clearRadioChunks(CHUNK_SEND);
			netwire::ChunkBatch batch;
			batch.kind = netwire::ChunkKind::Radio;
			batch.subject = (uint16_t)i;
			batch.words.assign(radio, radio + radioCount);
			netSession->sendToClients(gos_net::Delivery::ReliableUnordered, batch);
		}
	}
}

//---------------------------------------------------------------------------

void MultiPlayer::sendWeaponHitUpdate (void) {

	if (!netSession || (numWeaponHitChunks < 1))
		return;
	netwire::ChunkBatch batch;
	batch.kind = netwire::ChunkKind::WeaponHit;
	batch.words.assign(weaponHitChunks, weaponHitChunks + numWeaponHitChunks);
	numWeaponHitChunks = 0;
	netSession->sendToClients(gos_net::Delivery::ReliableOrdered, batch);
}

//---------------------------------------------------------------------------

void MultiPlayer::flushWorldEvents (void) {

	if (!netSession || (numWorldChunks < 1))
		return;
	netwire::ChunkBatch batch;
	batch.kind = netwire::ChunkKind::World;
	batch.words.assign(worldChunks, worldChunks + numWorldChunks);
	numWorldChunks = 0;
	netSession->sendToClients(gos_net::Delivery::ReliableOrdered, batch);
}

//---------------------------------------------------------------------------

void MultiPlayer::sendWorldUpdate (void) {

	flushWorldEvents();
}

//---------------------------------------------------------------------------
// The convergence check: what each unit should look like, for a client to compare.
//---------------------------------------------------------------------------

static uint32_t netArmorTotal (MoverPtr mover) {

	float total = 0.0f;
	for (long i = 0; i < mover->numArmorLocations; i++)
		total += mover->armor[i].curArmor;
	return((uint32_t)(total * 4.0f));
}

static void sendNetCheck (MultiPlayer* player) {

	netwire::ChunkBatch batch;
	batch.kind = netwire::ChunkKind::Check;
	for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++) {
		MoverPtr mover = player->moverRoster[i];
		if (!mover)
			continue;
		const Stuff::Vector3D position = mover->getPosition();
		batch.words.push_back((uint32_t)i);
		batch.words.push_back((uint32_t)mover->getStatus());
		batch.words.push_back(netArmorTotal(mover));
		batch.words.push_back((uint32_t)(int32_t)position.x);
		batch.words.push_back((uint32_t)(int32_t)position.y);
	}
	netSession->sendToClients(gos_net::Delivery::ReliableOrdered, batch);
}

// [NETCHECK] says how far this client is from what the host just sent. Armour is
// exact, since hits come in order on the same lane as the check. Status and position
// come by unreliable updates, so they may lag by one.
static void applyNetCheck (MultiPlayer* player, const netwire::ChunkBatch& batch) {

	long units = 0, statusDiffers = 0, armorDiffers = 0;
	float farthest = 0.0f;
	for (size_t i = 0; (i + 5) <= batch.words.size(); i += 5) {
		const uint32_t index = batch.words[i];
		MoverPtr mover = (index < MAX_MULTIPLAYER_MOVERS) ? player->moverRoster[index] : NULL;
		if (!mover)
			continue;
		units++;
		if ((uint32_t)mover->getStatus() != batch.words[i + 1]) {
			statusDiffers++;
			if (statusDiffers <= 3)
				printf("[NETCHECK] unit %u %s: status here %d, host %u\n", index, mover->getName(), (int)mover->getStatus(), batch.words[i + 1]);
		}
		// A wreck's armour no longer counts. A hit on a unit that is down takes another
		// path once it has finished falling, which an animation decides on each machine.
		const bool down = mover->isDisabled() || mover->isDestroyed() || (batch.words[i + 1] != OBJECT_STATUS_NORMAL);
		if (!down && (netArmorTotal(mover) != batch.words[i + 2])) {
			armorDiffers++;
			if (armorDiffers <= 3)
				printf("[NETCHECK] unit %u %s: armour here %.2f (status %d), host %.2f (status %u)\n", index, mover->getName(), netArmorTotal(mover) * 0.25f, (int)mover->getStatus(), batch.words[i + 2] * 0.25f, batch.words[i + 1]);
		}
		if (down)
			continue;	// nor does where it fell
		const Stuff::Vector3D position = mover->getPosition();
		const float dx = position.x - (float)(int32_t)batch.words[i + 3];
		const float dy = position.y - (float)(int32_t)batch.words[i + 4];
		const float distance = sqrt(dx * dx + dy * dy);
		if (distance > farthest)
			farthest = distance;
	}
	printf("[NETCHECK] %ld units: status differs %ld, armour differs %ld, farthest %.0f\n", units, statusDiffers, armorDiffers, farthest);
	printf("[NETWORLD] artillery %ld, mines %ld, fires %ld, captures %ld, kills and losses %ld, repairs %ld, script messages %ld, scores %ld, resource changes %ld, pilot kills %ld, turret targets %ld\n",
		netWorldApplied[0], netWorldApplied[1], netWorldApplied[2], netWorldApplied[3], netWorldApplied[4], netRepairsApplied, netWorldApplied[5], netWorldApplied[6], netWorldApplied[7], netWorldApplied[8], netWorldApplied[9]);
	printf("[NETSUPPORT] vehicles begun %ld, dropped %ld, recoveries begun %ld, recovered %ld, refused %ld\n",
		netSupportCounts[0], netSupportCounts[1], netSupportCounts[2], netSupportCounts[3], netSupportCounts[4]);
	fflush(stdout);
}

//---------------------------------------------------------------------------
// MESSAGE HANDLERS
//---------------------------------------------------------------------------

void MultiPlayer::handleChat (NETPLAYER sender, MCMSG_Chat* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleMissionSettingsUpdate (NETPLAYER sender, MCMSG_MissionSettingsUpdate* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handlePlayerCID (NETPLAYER sender, MCMSG_PlayerCID* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handlePlayerUpdate (NETPLAYER sender, MCMSG_PlayerUpdate* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleMissionSetup (NETPLAYER sender, MCMSG_MissionSetup* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handlePlayerInfo (NETPLAYER sender, MCMSG_PlayerInfo* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handlePlayerCheckIn (NETPLAYER sender, MCMSG_PlayerCheckIn* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handlePlayerSetup (NETPLAYER sender, MCMSG_PlayerSetup* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handlePlayerInsignia (NETPLAYER sender, MCMSG_PlayerInsignia* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleStartMission (NETPLAYER sender) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleEndMission (NETPLAYER sender, MCMSG_EndMission* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleReinforcement (NETPLAYER sender, MCMSG_Reinforcement* msg) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::handleNewServer (NETPLAYER sender, MCMSG_NewServer* msg) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::handleLeaveSession (NETPLAYER sender, MCMSG_LeaveSession* msg) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::handleHoldPosition (NETPLAYER sender, MCMSG_HoldPosition* msg) {

}

//-----------------------------------------------------------------------------

void MultiPlayer::handlePlayerOrder (NETPLAYER sender, MCMSG_PlayerOrder* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handlePlayerMoverGroup (NETPLAYER sender, MCMSG_PlayerMoverGroup* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handlePlayerArtillery (NETPLAYER sender, MCMSG_PlayerArtillery* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleMoverUpdate (NETPLAYER sender, MCMSG_MoverUpdate* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleTurretUpdate (NETPLAYER sender, MCMSG_TurretUpdate* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleMoverWeaponFireUpdate (NETPLAYER sender, MCMSG_MoverWeaponFireUpdate* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleTurretWeaponFireUpdate (NETPLAYER sender, MCMSG_TurretWeaponFireUpdate* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleMoverCriticalUpdate (NETPLAYER sender, MCMSG_MoverCriticalUpdate* msg) {

}

//---------------------------------------------------------------------------

extern bool FromMP;

void MultiPlayer::handleWeaponHitUpdate (NETPLAYER sender, MCMSG_WeaponHitUpdate* msg) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleWorldUpdate (NETPLAYER sender, MCMSG_WorldUpdate* msg) {

}

//---------------------------------------------------------------------------

// A weapon hit chunk from the network: unpack and the code after it end the game on
// a value the host would never send.
static bool validWeaponHitData (unsigned long data) {

	unsigned long fields = data;
	const unsigned long targetType = (fields & WEAPONHITCHUNK_TARGETTYPE_MASK);
	fields >>= WEAPONHITCHUNK_TARGETTYPE_BITS;
	if ((fields & WEAPONHITCHUNK_DAMAGE_MASK) > (255 * 4))
		return(false);
	fields >>= WEAPONHITCHUNK_DAMAGE_BITS;
	switch (targetType) {
		case WEAPONHITCHUNK_TARGET_MOVER: {
			if ((fields & WEAPONHITCHUNK_MOVERINDEX_MASK) >= MAX_MULTIPLAYER_MOVERS)
				return(false);
			fields >>= WEAPONHITCHUNK_MOVERINDEX_BITS + WEAPONHITCHUNK_CAUSE_BITS;
			const long hitLocation = (long)(fields & WEAPONHITCHUNK_HITLOCATION_MASK) - 2;
			return((hitLocation > -2) && (hitLocation < 12));
		}
		case WEAPONHITCHUNK_TARGET_TERRAIN:
			return(true);
		case WEAPONHITCHUNK_TARGET_SPECIAL:
			fields >>= WEAPONHITCHUNK_SPECIALID_BITS;
			return((fields & WEAPONHITCHUNK_SPECIALTYPE_MASK) == WEAPONHITCHUNK_SPECIAL_CAMERADRONE);
	}
	return(false);
}

//---------------------------------------------------------------------------

static void applyWeaponHit (MultiPlayer* player, unsigned long data) {

	static const float entryQuadTable[4] = {0.0f, 180.0f, -90.0f, 90.0f};

	if ((data & WEAPONHITCHUNK_TARGETTYPE_MASK) == NET_STREAM_CRITICAL) {
		const unsigned long index = (data >> WEAPONHITCHUNK_TARGETTYPE_BITS) & WEAPONHITCHUNK_MOVERINDEX_MASK;
		const unsigned long critical = (data >> (WEAPONHITCHUNK_TARGETTYPE_BITS + WEAPONHITCHUNK_MOVERINDEX_BITS)) & 0xFF;
		MoverPtr mover = (index < MAX_MULTIPLAYER_MOVERS) ? player->moverRoster[index] : NULL;
		const unsigned long space = critical & 0x0F;	// 15 is the whole location
		if (mover && (mover->getObjectClass() == BATTLEMECH) && ((critical >> 4) < (unsigned long)mover->numBodyLocations)
			&& ((space < MAX_CRITSPACES_PER_BODYLOCATION) || (space == 15)))
			((BattleMechPtr)mover)->handleCriticalHit(critical >> 4, space);
		return;
	}
	if (!validWeaponHitData(data))
		return;
	WeaponHitChunk chunk;
	chunk.init();
	chunk.data = data;
	chunk.unpack();
	GameObjectPtr target = NULL;
	if (chunk.targetType == WEAPONHITCHUNK_TARGET_MOVER)
		target = (GameObjectPtr)player->moverRoster[chunk.targetId];
	else
		target = ObjectManager->findByPartId(chunk.targetId);
	if (!target)
		return;
	// Not hits, though they travel as hits (tacordr.cpp). A repair runs here too, on the
	// same state, and comes to the same result.
	if (chunk.targetType == WEAPONHITCHUNK_TARGET_MOVER) {
		if (chunk.refit) {
			float pointsUsed = 0.0f;
			((MoverPtr)target)->refit(chunk.damage, pointsUsed);
			netRepairsApplied++;
			return;
		}
		if (chunk.cause == NET_CAUSE_REFIT_POINTS) {
			target->burnRefitPoints(chunk.damage);
			// as handleWeaponHit does on the host: out of points is the end of a truck
			if (!target->getRefitPoints())
				((MoverPtr)target)->disable(POWER_USED_UP);
			return;
		}
	}
	WeaponShotInfo shot;
	shot.attackerWID = 0;
	shot.masterId = chunk.cause;
	shot.damage = chunk.damage;
	shot.hitLocation = chunk.hitLocation;
	shot.entryAngle = entryQuadTable[chunk.entryAngle];
	target->handleWeaponHit(&shot, false);
}

//---------------------------------------------------------------------------

// Whether a strike from the network can be, and where on the ground.
static bool netStrikeLocation (long strike, long row, long col, Stuff::Vector3D& location) {

	if (((strike != ARTILLERY_SMALL) && (strike != ARTILLERY_LARGE) && (strike != ARTILLERY_SENSOR)) || !GameMap->inBounds(row, col))
		return(false);
	land->cellToWorld(row, col, location);
	location.z = land->getTerrainElevation(location);
	return(true);
}

//---------------------------------------------------------------------------

static void applyWorldEvent (MultiPlayer* player, const netwire::WorldEvent& event) {

	switch (event.type) {
		case netwire::WorldEventType::Artillery: {
			const long commander = event.value & 0xFF;
			const long strike = (event.value >> 8) & 0xFF;
			const long seconds = (short)(event.value >> 16);
			Stuff::Vector3D location;
			if ((commander >= MAX_MC_PLAYERS) || !Commander::commanders[commander] || !Commander::commanders[commander]->getTeam()
				|| !netStrikeLocation(strike, event.a, event.b, location))
				return;
			CallArtillery(commander, strike, location, seconds, false);
			netWorldApplied[0]++;
			break;
		}
		case netwire::WorldEventType::Mine: {
			const long state = event.value & 0xFF;
			const long explosion = (event.value >> 8) & 0xFF;
			if (!GameMap->inBounds(event.a, event.b) || (state > 2))
				return;
			GameMap->setMine(event.a, event.b, state);
			if (explosion == 2) {
				Stuff::Vector3D location;
				land->cellToWorld(event.a, event.b, location);
				ObjectManager->createExplosion(MINE_EXPLOSION_ID, NULL, location, MineSplashDamage, MineSplashRange * worldUnitsPerMeter);
			}
			netWorldApplied[1]++;
			break;
		}
		case netwire::WorldEventType::TerrainFire: {
			GameObjectPtr object = ObjectManager->findByPartId((long)event.value);
			if (!object || (object->getObjectClass() != TERRAINOBJECT) || (((TerrainObjectPtr)object)->getSubType() != TERROBJ_TREE))
				return;
			((TerrainObjectPtr)object)->lightOnFire((float)event.a);
			netWorldApplied[2]++;
			break;
		}
		case netwire::WorldEventType::Capture: {
			GameObjectPtr object = ObjectManager->findByPartId((long)event.value);
			if (!object || (object->getObjectClass() != BUILDING) || (event.a >= MAX_MC_PLAYERS) || (event.b >= MAX_TEAMS) || !Team::teams[event.b])
				return;
			// as the order that took it did on the host (tacordr.cpp)
			if ((object->getObjectType()->getObjTypeNum() == GENERIC_HQ_BUILDING_OBJNUM) && (player->missionSettings.missionType == MISSION_TYPE_CAPTURE_BASE))
				object->setFlag(OBJECT_FLAG_CAPTURABLE, false);
			object->setCommanderId(event.a);
			object->setTeamId(event.b, false);
			netWorldApplied[3]++;
			break;
		}
		case netwire::WorldEventType::KillLoss:
			if (event.a < MAX_MC_PLAYERS)
				player->playerInfo[event.a].kills++;
			if (event.b < MAX_MC_PLAYERS)
				player->playerInfo[event.b].losses++;
			netWorldApplied[4]++;
			break;
		case netwire::WorldEventType::ScriptMessage:
			if (mission)
				mission->handleMultiplayMessage((long)(int32_t)event.value, (long)event.a | ((long)event.b << 12));
			netWorldApplied[5]++;
			break;
		case netwire::WorldEventType::Score:
			if (event.a < MAX_TEAMS)
				player->teamScore[event.a] += (int32_t)event.value;
			netWorldApplied[6]++;
			break;
		case netwire::WorldEventType::Resources: {
			if (event.a >= MAX_MC_PLAYERS)
				return;
			const long points = (int32_t)event.value;
			// what this commander spent went at the button
			if ((event.a != player->commanderID) || (points > 0)) {
				player->playerInfo[event.a].resourcePoints += points;
				if (points > 0)
					player->playerInfo[event.a].resourcePointsGained += points;
				if (event.a == player->commanderID)
					LogisticsData::instance->addResourcePoints(points);
			}
			netWorldApplied[7]++;
			break;
		}
		case netwire::WorldEventType::PilotKill: {
			MoverPtr mover = (event.a < MAX_MULTIPLAYER_MOVERS) ? player->moverRoster[event.a] : NULL;
			if (!mover || !mover->getPilot() || (event.b >= NUM_VEHICLE_CLASSES))
				return;
			mover->getPilot()->numMechKills[event.b][COMBAT_STAT_MISSION]++;
			netWorldApplied[8]++;
			break;
		}
		case netwire::WorldEventType::TurretTarget: {
			if ((long)event.a >= ObjectManager->getNumTurrets())
				return;
			MoverPtr mover = (event.b && (event.b <= MAX_MULTIPLAYER_MOVERS)) ? player->moverRoster[event.b - 1] : NULL;
			ObjectManager->getTurret(event.a)->targetWID = mover ? mover->getWatchID() : 0;
			netWorldApplied[9]++;
			break;
		}
	}
}

//---------------------------------------------------------------------------

// Client: a support vehicle or a recovery, as the host decided.
static void applySupport (MultiPlayer* player, const netwire::ChunkBatch& batch) {

	if (batch.subject != netwire::NO_SUBJECT) {
		netApplyArmour(player, batch);
		return;
	}
	netwire::SupportEvent event;
	if ((batch.words.size() != netwire::SUPPORT_EVENT_WORDS) || !netwire::decodeSupportEvent(batch.words.data(), event))
		return;
	if (event.commander == player->commanderID)
		netSupportAsked = false;
	const bool known = Commander::commanders[event.commander] && Commander::commanders[event.commander]->getTeam()
		&& (ControlGui::supportCost(event.vehicleId) >= 0);
	MoverPtr mover = (event.rosterIndex < MAX_MULTIPLAYER_MOVERS) ? player->moverRoster[event.rosterIndex] : NULL;
	switch (event.stage) {
		case netwire::SupportStage::BeginVehicle: {
			if (!known || (event.vehicleId == 147))
				return;
			// a unit in another slot from the host's would get another unit's updates
			if (mover || !netBeginVehicle(player, event) || (player->reinforcements[event.commander][0] != event.rosterIndex))
				printf("[NET]: ERROR: support unit for slot %u is not in that slot here\n", (unsigned)event.rosterIndex);
			break;
		}
		case netwire::SupportStage::DropVehicle:
			if (!mover || !mover->getPilot())
				return;
			mover->setExists(true);
			netSupportCounts[1]++;
			break;
		case netwire::SupportStage::BeginRecovery:
			if (!known || (event.vehicleId != 147) || !mover || (mover->getObjectClass() != BATTLEMECH))
				return;
			netBeginRecovery(player, event);
			break;
		case netwire::SupportStage::Recovered:
			if (!known || !mover || (mover->getObjectClass() != BATTLEMECH))
				return;
			netSupportCounts[3]++;
			MissionInterfaceManager::instance()->recoverMech(event.commander, mover);
			break;
		case netwire::SupportStage::Refused:
			netApplyRefused(player, event);
			break;
		default:
			break;
	}
}

//---------------------------------------------------------------------------

// Client: what the host sent. The roster indices and chunks are the host's, and are
// still checked: a bad one must not end the game here.
static void applyHostBatch (MultiPlayer* player, const netwire::ChunkBatch& batch) {

	static uint32_t lastSweep = 0;
	unsigned long buffer[MAX_WEAPONFIRE_CHUNKS];

	MoverPtr mover = (batch.subject < MAX_MULTIPLAYER_MOVERS) ? player->moverRoster[batch.subject] : NULL;

	switch (batch.kind) {
		case netwire::ChunkKind::Move: {
			if (batch.words.empty() || ((int32_t)(batch.words[0] - lastSweep) <= 0))
				break;
			lastSweep = batch.words[0];
			for (size_t i = 1; (i + 3) <= batch.words.size(); i += 3) {
				const uint32_t index = batch.words[i] & ~NET_SWEEP_AWAKE;
				MoverPtr unit = (index < MAX_MULTIPLAYER_MOVERS) ? player->moverRoster[index] : NULL;
				if (!unit || !unit->getPilot())
					continue;
				// GameObject's: Mover's also orders a power up, which is the host's to send
				unit->GameObject::setAwake((batch.words[i] & NET_SWEEP_AWAKE) != 0);
				unit->handleMoveChunk(batch.words[i + 1]);
				unit->handleStatusChunk(0, batch.words[i + 2]);
			}
			break;
		}
		case netwire::ChunkKind::WeaponFire: {
			const size_t count = batch.words.size();
			if (count > MAX_WEAPONFIRE_CHUNKS)
				break;
			std::copy(batch.words.begin(), batch.words.end(), buffer);
			if (mover) {
				// a sleeping unit does not empty its queue, and a full one is Fatal
				if ((mover->numWeaponFireChunks[CHUNK_RECEIVE] + (long)count) < MAX_WEAPONFIRE_CHUNKS)
					mover->addWeaponFireChunks(CHUNK_RECEIVE, buffer, (long)count);
				}
			else if ((batch.subject >= NET_TURRET_SUBJECT) && ((batch.subject - NET_TURRET_SUBJECT) < ObjectManager->getNumTurrets())) {
				TurretPtr turret = ObjectManager->getTurret(batch.subject - NET_TURRET_SUBJECT);
				if ((turret->numWeaponFireChunks[CHUNK_RECEIVE] + (long)count) < MAX_WEAPONFIRE_CHUNKS)
					turret->addWeaponFireChunks(CHUNK_RECEIVE, buffer, (long)count);
			}
			break;
		}
		case netwire::ChunkKind::Radio: {
			if (!mover)
				break;
			for (size_t i = 0; i < batch.words.size(); i++)
				if (batch.words[i] < RADIO_MESSAGE_COUNT)
					mover->addRadioChunk(CHUNK_RECEIVE, (unsigned char)batch.words[i]);
			break;
		}
		case netwire::ChunkKind::WeaponHit:
			for (size_t i = 0; i < batch.words.size(); i++)
				applyWeaponHit(player, batch.words[i]);
			break;
		case netwire::ChunkKind::World: {
			netwire::WorldEvent event;
			for (size_t i = 0; (i + netwire::WORLD_EVENT_WORDS) <= batch.words.size(); i += netwire::WORLD_EVENT_WORDS)
				if (netwire::decodeWorldEvent(&batch.words[i], event))
					applyWorldEvent(player, event);
			break;
		}
		case netwire::ChunkKind::Support:
			applySupport(player, batch);
			break;
		case netwire::ChunkKind::Check:
			applyNetCheck(player, batch);
			break;
		default:
			break;
	}
}

//---------------------------------------------------------------------------

// Host: an order from a client, who may only order their own units.
static void applyClientOrder (MultiPlayer* player, const netsession::Received& received) {

	const netwire::ChunkBatch& batch = received.batch;
	if (batch.subject == netwire::NO_SUBJECT) {
		if (batch.words.size() == netwire::SUPPORT_EVENT_WORDS) {
			netwire::SupportEvent event;
			if (!netwire::decodeSupportEvent(batch.words.data(), event)
				|| ((event.stage != netwire::SupportStage::RequestVehicle) && (event.stage != netwire::SupportStage::RequestRecovery)))
				return;
			// whatever commander it names, it is the sender's
			netHostRequest(player, received.commander, event.vehicleId, event.rosterIndex, Stuff::Vector3D(event.x, event.y, event.z), event.stage == netwire::SupportStage::RequestRecovery);
			netOrdersApplied++;
			return;
		}
		// a strike the client called: the host keeps the count, and it is theirs
		if (batch.words.size() != 4)
			return;
		const long strike = (long)batch.words[0];
		const long row = (long)batch.words[1];
		const long col = (long)batch.words[2];
		// more than five minutes is not a strike
		const long seconds = (long)(int32_t)batch.words[3];
		Stuff::Vector3D location;
		const long cost = ControlGui::strikeCost(strike);
		if ((cost < 0) || (seconds < -1) || (seconds > 300) || !netStrikeLocation(strike, row, col, location))
			return;
		const bool enabled = (strike == ARTILLERY_SENSOR) ? player->missionSettings.sensorProbe : player->missionSettings.airStrike;
		if (!enabled || (player->playerInfo[received.commander].resourcePoints < cost)) {
			netHostRefuse(player, received.commander, 0, cost);
			return;
		}
		netHostResources(player, received.commander, -cost);
		CallArtillery(received.commander, strike, location, seconds, false);
		netOrdersApplied++;
		return;
	}
	if ((batch.subject >= MAX_MULTIPLAYER_MOVERS) || ((batch.words.size() != 3) && (batch.words.size() != 1)))
		return;
	MoverPtr mover = player->moverRoster[batch.subject];
	if (!mover || !mover->getPilot() || (mover->getCommanderId() != received.commander))
		return;
	if (batch.words.size() == 1) {
		// hold position: as ControlGui::toggleHoldPosition does on a host
		const long range = (long)(batch.words[0] & 0xFF) + FIRERANGE_DEFAULT;
		if (range >= NUM_FIRERANGES)
			return;
		mover->attackRange = range;
		netHoldsApplied++;
		TacticalOrderPtr current = mover->getPilot()->getCurTacOrder();
		if ((current->code != TACTICAL_ORDER_NONE) && !current->targetIsGone()) {
			current->attackParams.range = (FireRangeType)range;
			current->attackParams.pursue = (batch.words[0] & 0x100u) != 0;
			current->pack(NULL, NULL);
			mover->handleTacticalOrder(*current);
		}
		return;
	}
	if (!TacticalOrder::validPacked(batch.words[0], batch.words[1]))
		return;
	TacticalOrder order;
	order.init();
	order.data[0] = batch.words[0];
	order.data[1] = batch.words[1];
	order.unpack();
	order.origin = ORDER_ORIGIN_PLAYER;
	netOrdersApplied++;
	if ((order.code >= 0) && (order.code < NUM_TACTICAL_ORDERS))
		netOrdersByCode[order.code]++;
	if (batch.words[2]) {
		mover->getPilot()->setExecutingQueue(FALSE);
		mover->getPilot()->addQueuedTacOrder(order);
		}
	else
		mover->handleTacticalOrder(order);
}

//---------------------------------------------------------------------------

void MultiPlayer::handlePlayerLeftSession (NETPLAYER leftPlayer) {

}

//---------------------------------------------------------------------------

void MultiPlayer::handleTerminateSession (void) {

}

//---------------------------------------------------------------------------

bool MultiPlayer::isServerMissing (void) {

	return(false);
}

//---------------------------------------------------------------------------

bool MultiPlayer::processGameMessage (NETMESSAGE* msg) {

	return(true);
}

//---------------------------------------------------------------------------

void MultiPlayer::processMessages (void) {

	if (!netSession)
		return;
	netSession->update();
	netsession::Received received;
	// The results are up and the units are gone, so what is still on its way is for nobody,
	// and a host closing its results is not a reason to close these.
	if (netGameEnded) {
		while (netSession->nextBatch(received))
			;
		return;
	}
	while (netSession->nextBatch(received)) {
		if (iAmHost)
			applyClientOrder(this, received);
		else
			applyHostBatch(this, received.batch);
	}
	if (!iAmHost && netSession->hostLost() && !hostLeft) {
		hostLeft = true;
		printf("[NET] the host has gone\n");
		fflush(stdout);
		quitGame = true;
	}
}

//-----------------------------------------------------------------------------
// Misc. routines
//-----------------------------------------------------------------------------

void MultiPlayer::buildMoverRosterRLE (void) {

}

//---------------------------------------------------------------------------

long MultiPlayer::updateClients (bool forceIt) {

	if (!mission || !mission->isActive())
		return(MPLAYER_NO_ERR);

	if (netSession) {
		sendTurretUpdate();
		sendMoverWeaponFireUpdate();
		sendTurretWeaponFireUpdate();
		sendMoverCriticalUpdate();
		sendWeaponHitUpdate();
		flushWorldEvents();
		// on their own clock, not the frame
		const double now = netNow();
		static double nextMoverUpdate = 0.0, nextCheck = 0.0;
		if (forceIt || (now >= nextMoverUpdate)) {
			nextMoverUpdate = now + moverUpdateFrequency;
			sendMoverUpdate();
		}
		if (forceIt || (now >= nextCheck)) {
			nextCheck = now + worldUpdateFrequency;
			sendNetCheck(this);
		}
		if ((turn % 600) == 0)
			printf("[NET] turn %ld: sent weapon fire %ld (turrets %ld), weapon hit %ld, critical hit %ld; orders from clients %ld, holds %ld; %d clients\n",
				(long)turn, moverWeaponFireLoad + turretWeaponFireLoad, turretWeaponFireLoad, weaponHitLoad, moverCriticalLoad, netOrdersApplied, netHoldsApplied, netSession->connectedClients());
		return(MPLAYER_NO_ERR);
	}

	// Nobody to send to yet, so empty the queues the game fills for clients (a
	// full weapon fire queue is Fatal), and log what they held.
	for (long i = 0; i < ObjectManager->getNumMovers(); i++) {
		MoverPtr mover = ObjectManager->getMover(i);
		moverWeaponFireLoad += mover->clearWeaponFireChunks(CHUNK_SEND);
		moverCriticalLoad += mover->clearCriticalHitChunks(CHUNK_SEND);
		mover->clearRadioChunks(CHUNK_SEND);
	}
	for (long i = 0; i < ObjectManager->getNumTurrets(); i++)
		turretWeaponFireLoad += ObjectManager->getTurret(i)->clearWeaponFireChunks(CHUNK_SEND);
	if ((turn % 600) == 0)
		printf("[NETHOST] turn %ld: chunks queued so far: weapon fire %ld (turrets %ld), weapon hit %ld, critical hit %ld\n",
			(long)turn, moverWeaponFireLoad + turretWeaponFireLoad, turretWeaponFireLoad, weaponHitLoad, moverCriticalLoad);
	return(MPLAYER_NO_ERR);
}

//---------------------------------------------------------------------------

bool MultiPlayer::calcMissionStatus (void) {

	// With no network: no win conditions yet, and true would end the mission on its
	// first frame.
	if (!netSession)
		return(false);

	long winner = netwire::NO_WINNER;
	if (iAmHost) {
		// not before anyone has had time to arrive
		if (scenarioTime < 10.0f)
			return(false);

		netrules::GameState state;
		state.mode = missionSettings.missionType;
		state.timeUp = (missionSettings.timeLimit > 0.0f) && (scenarioTime >= missionSettings.timeLimit);
		for (long i = 0; i < MAX_MC_PLAYERS; i++) {
			const long team = playerInfo[i].team;
			if ((playerInfo[i].commanderID > -1) && (team >= 0) && (team < netrules::NUM_TEAMS))
				state.teams[team].playing = true;
		}
		for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++) {
			MoverPtr mover = moverRoster[i];
			const long team = mover ? mover->getTeamId() : -1;
			// a repair truck called in does not keep a team in the game
			if (mover && !netSupportUnit[i] && (team >= 0) && (team < netrules::NUM_TEAMS) && mover->getExists() && !mover->isDisabled() && !mover->isDestroyed())
				state.teams[team].liveUnits++;
		}
		for (long team = 0; team < MAX_TEAMS; team++) {
			state.teams[team].score = teamScore[team];
			if (mainHqBuildings[team]) {
				state.teams[team].hasBase = true;
				state.teams[team].lostBase = (mainHqBuildings[team]->getTeamId() != team);
			}
		}
		for (size_t i = 0; i < netHqs.size(); i++) {
			const long team = netHqs[i]->getTeamId();
			if ((team >= 0) && (team < netrules::NUM_TEAMS))
				state.teams[team].held++;
		}
		if (!netHqs.empty())
			state.hillTeam = netHqs[0]->getTeamId();

		// holding is worth a point a second, to one clock on the host
		static long lastPointsAt = 0;
		const long second = (long)scenarioTime;
		if (second != lastPointsAt) {
			lastPointsAt = second;
			for (long team = 0; team < MAX_TEAMS; team++) {
				const long points = netrules::pointsPerSecond(state.mode, state.teams[team]);
				if (points > 0) {
					addTeamScore(team, points);
					state.teams[team].score += points;
				}
			}
		}

		const netrules::Result result = netrules::decide(state);
		if (!result.over)
			return(false);
		winner = (result.winner == netrules::NO_TEAM) ? (long)netwire::NO_WINNER : result.winner;
		updateClients(true);
		netSession->endGame((uint8_t)winner);
		}
	else {
		if (!netSession->gameOver())
			return(false);
		winner = netSession->winningTeam();
	}
	for (long i = 0; i < MAX_MC_PLAYERS; i++)
		playerInfo[i].winner = (playerInfo[i].commanderID > -1) && (playerInfo[i].team == winner);
	printf("[NET] game over: team %ld wins\n", winner);
	// what both ends must agree on by now: net_smoke.sh compares the lines
	long mines = 0;
	for (long row = 0; row < GameMap->getHeight(); row++)
		for (long col = 0; col < GameMap->getWidth(); col++)
			if (GameMap->getMine(row, col) == 1)
				mines++;
	long buildings[MAX_TEAMS] = {0};
	for (long i = 0; i < ObjectManager->getNumBuildings(); i++) {
		BuildingPtr building = ObjectManager->getBuilding(i);
		const long owner = building ? building->getTeamId() : -1;
		if ((owner >= 0) && (owner < MAX_TEAMS))
			buildings[owner]++;
	}
	for (long i = 0; i < MAX_MC_PLAYERS; i++)
		if (playerInfo[i].commanderID > -1)
			printf("[NETSCORE] commander %ld: kills %d, losses %d, team's buildings %ld, team's score %d\n", i, playerInfo[i].kills, playerInfo[i].losses, buildings[playerInfo[i].team], teamScore[playerInfo[i].team]);
	printf("[NETSCORE] mines %ld\n", mines);
	// compared where a run spends through the buttons, as a test that calls strikes does not
	for (long i = 0; i < MAX_MC_PLAYERS; i++)
		if (playerInfo[i].commanderID > -1)
			printf("[NETPOINTS] commander %ld: resource points %ld\n", i, playerInfo[i].resourcePoints);
	long pilotKills = 0;
	for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++)
		if (moverRoster[i] && moverRoster[i]->getPilot())
			for (long j = 0; j < NUM_VEHICLE_CLASSES; j++)
				pilotKills += moverRoster[i]->getPilot()->numMechKills[j][COMBAT_STAT_MISSION];
	printf("[NETSCORE] pilot kills %ld\n", pilotKills);
	if (iAmHost)
		printf("[NETORDERS] move %ld, jump %ld, attack object %ld, attack point %ld, guard %ld, stop %ld, wait %ld, power down %ld, power up %ld, eject %ld, repair %ld, capture %ld\n",
			netOrdersByCode[TACTICAL_ORDER_MOVETO_POINT], netOrdersByCode[TACTICAL_ORDER_JUMPTO_POINT], netOrdersByCode[TACTICAL_ORDER_ATTACK_OBJECT], netOrdersByCode[TACTICAL_ORDER_ATTACK_POINT],
			netOrdersByCode[TACTICAL_ORDER_GUARD], netOrdersByCode[TACTICAL_ORDER_STOP], netOrdersByCode[TACTICAL_ORDER_WAIT], netOrdersByCode[TACTICAL_ORDER_POWERDOWN], netOrdersByCode[TACTICAL_ORDER_POWERUP],
			netOrdersByCode[TACTICAL_ORDER_EJECT], netOrdersByCode[TACTICAL_ORDER_REFIT], netOrdersByCode[TACTICAL_ORDER_CAPTURE]);
	fflush(stdout);
	return(true);
}

//-----------------------------------------------------------------------------

int __cdecl sortPlayerRanks (const void* player1, const void* player2) {

	return(0);
}

//-----------------------------------------------------------------------------

void MultiPlayer::logResults (void) {

	netGameEnded = true;
	calcPlayerRanks();
	for (long i = 0; i < MAX_MC_PLAYERS; i++)
		if (playerInfo[i].commanderID > -1)
			printf("[NETRESULT] rank %d: %s, team %d, score %d, kills %d, losses %d, %s\n", (int)playerInfo[i].rank, playerInfo[i].name, (int)playerInfo[i].team, (int)playerInfo[i].score,
				(int)playerInfo[i].kills, (int)playerInfo[i].losses, playerInfo[i].winner ? "winner" : "not the winner");
	fflush(stdout);
}

//-----------------------------------------------------------------------------

// Nobody is there to press Close on an automated run. A frame goes through all of the screen's
// code; MC2_NET_RESULTS_FRAMES keeps it up longer, and under -bench takes a screenshot of it.
bool MultiPlayer::resultsFrameDone (void) {

	if (!gos_IsAutomatedRun())
		return(false);
	static long frames = -1;
	if (frames < 0) {
		const char* wanted = getenv("MC2_NET_RESULTS_FRAMES");
		frames = wanted ? atol(wanted) : 1;
		if (frames < 1)
			frames = 1;
	}
	if (frames == 2)
		gos_BenchScreenshot("results");
	return(--frames < 1);
}

//-----------------------------------------------------------------------------

void MultiPlayer::calcPlayerRanks (void) {

	if (!netSession)
		return;
	// the winners first, then each team's score
	long standings[MAX_MC_PLAYERS] = {0};
	long ranks[MAX_MC_PLAYERS] = {0};
	long ids[MAX_MC_PLAYERS] = {0};
	int count = 0;
	for (long i = 0; i < MAX_MC_PLAYERS; i++)
		if ((playerInfo[i].commanderID > -1) && (playerInfo[i].team >= 0)) {
			playerInfo[i].score = teamScore[playerInfo[i].team];
			standings[count] = playerInfo[i].score + (playerInfo[i].winner ? 100000000L : 0L);
			ids[count++] = i;
		}
	netsession::rankPlayers(standings, count, ranks);
	for (int i = 0; i < count; i++)
		playerInfo[ids[i]].rank = ranks[i];
}

//-----------------------------------------------------------------------------

bool MultiPlayer::allPlayersCheckedIn (void) {

	return(true);
}

//---------------------------------------------------------------------------

bool MultiPlayer::allPlayersReady (void) {

	return(true);
}

//---------------------------------------------------------------------------

void MultiPlayer::switchServers (void) {

}


//---------------------------------------------------------------------------

void MultiPlayer::calcDropZones (char dropZonesCID[MAX_MC_PLAYERS], char hqs[MAX_TEAMS]) {

}

//---------------------------------------------------------------------------

void MultiPlayer::initStartupParameters (bool fresh) {

}


//---------------------------------------------------------------------------
extern char* GetTime();

extern const char *SpecialtySkillsTable[NUM_SPECIALTY_SKILLS];

long MultiPlayer::saveTranscript (const char* fileName, bool debugging) {

	return(MPLAYER_NO_ERR);
}

//---------------------------------------------------------------------------

void MultiPlayer::destroy (void) {

	netClose();
}

//---------------------------------------------------------------------------

void MultiPlayer::getChatMessages( char** buffer, long* playerIDs, long& count )
{
}

void MultiPlayer::redistributeRP( )
{
}


//***************************************************************************
