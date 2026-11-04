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

#include<algorithm>
#include<cctype>
#include<chrono>
#include<cmath>
#include<memory>
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

static void netClose (void) {

	netSession.reset();
	netTransport.reset();
}

static const float NET_WAIT_SECONDS = 300.0f;
static long netOrdersApplied = 0;
static long netRepairsApplied = 0;
static long netWorldApplied[5] = {0};	// by event type: artillery, mine, fire, capture, kill or loss
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

static unsigned long netHash (unsigned long hash, const char* text) {

	for (; *text; text++) {
		hash ^= (unsigned char)tolower((unsigned char)*text);
		hash = (hash * 16777619u) & 0xFFFFFFFFu;
	}
	return(hash);
}

static const char* netRejectName (netwire::RejectReason reason) {

	switch (reason) {
		case netwire::RejectReason::WrongVersion: return("another version of the game");
		case netwire::RejectReason::WrongData: return("another mission");
		case netwire::RejectReason::GameFull: return("the game is full");
		case netwire::RejectReason::WrongRoster: return("other units in the mission");
		default: return("unknown");
	}
}

//---------------------------------------------------------------------------

void MultiPlayer::hostNetGame (long numPlayers, unsigned short port, const char* missionName) {

	std::string error;
	netTransport = gos_net::createGnsTransport(&error);
	if (!netTransport)
		Fatal(0, (std::string(" -netlisten: no network: ") + error).c_str());
	netSession.reset(new netsession::Session(*netTransport));
	if (!netSession->host(port, (uint8_t)numPlayers, (uint32_t)netHash(2166136261u, missionName)))
		Fatal(port, " -netlisten: cannot listen on this port ");
	startLocalHost(numPlayers);
	initUpdateFrequencies();
	printf("[NET] hosting %s on port %d for %ld players\n", missionName, (int)port, numPlayers);
	fflush(stdout);
	if (!netWait([]() { return netSession->allJoined(); }))
		Fatal(0, " -netlisten: not everyone joined in time ");
}

//---------------------------------------------------------------------------

void MultiPlayer::joinNetGame (const char* address, unsigned short port, const char* missionName) {

	std::string error;
	netTransport = gos_net::createGnsTransport(&error);
	if (!netTransport)
		Fatal(0, (std::string(" -netjoin: no network: ") + error).c_str());
	netSession.reset(new netsession::Session(*netTransport));
	if (!netSession->join(address, port, (uint32_t)netHash(2166136261u, missionName), "player"))
		Fatal(0, " -netjoin: needs an IP address ");
	printf("[NET] joining %s at %s port %d\n", missionName, address, (int)port);
	fflush(stdout);
	const bool answered = netWait([]() {
		return netSession->joined() || netSession->hostLost() || (netSession->rejected() != netwire::RejectReason::None);
	});
	if (!answered || netSession->hostLost())
		Fatal(0, " -netjoin: no answer from the host ");
	if (netSession->rejected() != netwire::RejectReason::None)
		Fatal(0, (std::string(" -netjoin: refused: ") + netRejectName(netSession->rejected())).c_str());
	startLocalHost(netSession->numPlayers());
	iAmHost = false;
	commanderID = netSession->commander();
	initUpdateFrequencies();
	printf("[NET] joined as commander %ld of %d\n", commanderID, (int)netSession->numPlayers());
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
	fflush(stdout);
}

//---------------------------------------------------------------------------

bool MultiPlayer::isNetworked (void) {

	return(netSession != nullptr);
}

//---------------------------------------------------------------------------

long MultiPlayer::update (void) {

	processMessages();
	if (iAmHost)
		updateClients();
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

	// networking is stubbed: no players (callers loop to playerCount)
	playerCount = 0;
	return(NULL);
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

void MultiPlayer::initSpecialBuildings (char commandersToLoad[8][3]) {

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

long MultiPlayer::addWeaponHitChunk (GameObjectPtr target, WeaponShotInfoPtr shotInfo, bool isRefit) {

	if (!netSession) {
		weaponHitLoad++;
		return(0);
	}
	WeaponHitChunk chunk;
	chunk.init();
	chunk.build(target, shotInfo, isRefit);
	return(addWeaponHitChunk(&chunk));
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

void MultiPlayer::sendReinforcement (long vehicleID, long rosterIndex, const char pilotName[16], long commanderID, Stuff::Vector3D pos, unsigned char stage) {

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

void MultiPlayer::sendTurretUpdate (void) {

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
		if ((uint32_t)mover->getStatus() != batch.words[i + 1])
			statusDiffers++;
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
	printf("[NETWORLD] artillery %ld, mines %ld, fires %ld, captures %ld, kills and losses %ld, repairs %ld\n",
		netWorldApplied[0], netWorldApplied[1], netWorldApplied[2], netWorldApplied[3], netWorldApplied[4], netRepairsApplied);
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
		// a strike the client called: the host keeps the count, and it is theirs
		if (batch.words.size() != 4)
			return;
		const long strike = (long)batch.words[0];
		const long row = (long)batch.words[1];
		const long col = (long)batch.words[2];
		// more than five minutes is not a strike
		const long seconds = (long)(int32_t)batch.words[3];
		Stuff::Vector3D location;
		if ((seconds < -1) || (seconds > 300) || !netStrikeLocation(strike, row, col, location))
			return;
		CallArtillery(received.commander, strike, location, seconds, false);
		netOrdersApplied++;
		return;
	}
	if ((batch.subject >= MAX_MULTIPLAYER_MOVERS) || (batch.words.size() != 3))
		return;
	MoverPtr mover = player->moverRoster[batch.subject];
	if (!mover || !mover->getPilot() || (mover->getCommanderId() != received.commander))
		return;
	if (!TacticalOrder::validPacked(batch.words[0], batch.words[1]))
		return;
	TacticalOrder order;
	order.init();
	order.data[0] = batch.words[0];
	order.data[1] = batch.words[1];
	order.unpack();
	order.origin = ORDER_ORIGIN_PLAYER;
	netOrdersApplied++;
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
			printf("[NET] turn %ld: sent weapon fire %ld (turrets %ld), weapon hit %ld, critical hit %ld; orders from clients %ld; %d clients\n",
				(long)turn, moverWeaponFireLoad + turretWeaponFireLoad, turretWeaponFireLoad, weaponHitLoad, moverCriticalLoad, netOrdersApplied, netSession->connectedClients());
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
	// first frame. Phase 3 has the game modes; for now the last team standing wins.
	if (!netSession)
		return(false);

	long winner = netwire::NO_WINNER;
	if (iAmHost) {
		// not before anyone has had time to arrive
		if (scenarioTime < 10.0f)
			return(false);
		bool alive[MAX_TEAMS] = {false};
		long teamsAlive = 0;
		for (long i = 0; i < MAX_MULTIPLAYER_MOVERS; i++) {
			MoverPtr mover = moverRoster[i];
			if (!mover || !mover->getExists() || mover->isDisabled() || mover->isDestroyed() || (mover->getTeamId() < 0))
				continue;
			if (!alive[mover->getTeamId()]) {
				alive[mover->getTeamId()] = true;
				teamsAlive++;
				winner = mover->getTeamId();
			}
		}
		if (teamsAlive > 1)
			return(false);
		if (teamsAlive == 0)
			winner = netwire::NO_WINNER;
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
			printf("[NETSCORE] commander %ld: kills %d, losses %d, team's buildings %ld\n", i, playerInfo[i].kills, playerInfo[i].losses, buildings[playerInfo[i].team]);
	printf("[NETSCORE] mines %ld\n", mines);
	fflush(stdout);
	return(true);
}

//-----------------------------------------------------------------------------

int __cdecl sortPlayerRanks (const void* player1, const void* player2) {

	return(0);
}

//-----------------------------------------------------------------------------

void MultiPlayer::calcPlayerRanks (void) {

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
