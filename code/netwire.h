// code/netwire.h
//
// The multiplayer wire format (planning/future-features/NETWORKING.md): fixed
// width little-endian fields, the same on every platform, whatever long is.
// No GL, SDL or game dependency. Everything received is untrusted, so a decoder
// checks every field and refuses the whole message rather than end the game.
//
// A message is one type byte and its body. What the words of a chunk mean is
// the game's business (MoveChunk::pack and so on), and the game must range-check
// them too when it unpacks them.

#ifndef NETWIRE_H
#define NETWIRE_H

#include "gos_net.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace netwire {

// Raise on any change to a message's layout. Two things must never change, so
// that a host can refuse a client of any version: Hello opens with the type byte
// and the protocol (peekProtocol), and Welcome's layout stays as it is.
constexpr uint16_t PROTOCOL_VERSION = 8;

constexpr size_t MAX_NAME_LENGTH = 31;
constexpr size_t MAX_BATCH_CHUNKS = 1024;	// the largest game queue, MAX_WEAPONHIT_CHUNKS
constexpr size_t MAX_MESSAGE_SIZE = gos_net::MAX_MESSAGE_SIZE;
constexpr uint8_t MAX_PLAYERS = 8;			// MAX_MC_PLAYERS
constexpr uint16_t NO_SUBJECT = 0xFFFF;

enum class MessageType : uint8_t {
	Hello = 1,
	Welcome = 2,
	ChunkBatch = 3,
	Ready = 4,
	Start = 5,
	EndGame = 6,
	LanQuery = 7,
	LanAnnounce = 8,
	LobbyState = 9,
	LobbyChoice = 10,
	LobbyLaunch = 11
};

enum class RejectReason : uint8_t {
	None = 0,
	WrongVersion = 1,
	WrongData = 2,
	GameFull = 3,
	WrongRoster = 4		// the mission loaded with other units than the host's
};

enum class ChunkKind : uint8_t {
	Move = 1,
	Status = 2,
	WeaponFire = 3,
	WeaponHit = 4,
	CriticalHit = 5,
	Radio = 6,
	World = 7,
	Order = 8,
	Check = 9,
	Support = 10		// a support vehicle or a recovery: SupportEvent
};

// client to host, first thing on a connection
struct Hello {
	uint16_t protocol = PROTOCOL_VERSION;
	uint32_t dataHash = 0;		// of the mission and object data and the mod set
	std::string name;
};

// host to client, the answer to Hello
struct Welcome {
	RejectReason reject = RejectReason::None;
	uint8_t commanderId = 0;	// below numPlayers, when not rejected
	uint8_t numPlayers = 0;		// 1 to MAX_PLAYERS, when not rejected
};

// client to host, once the mission has loaded
struct Ready {
	uint32_t rosterHash = 0;	// of the units it loaded, in order
};

// host to clients, once all are ready: no body
struct Start {
};

constexpr uint8_t NO_WINNER = 0xFF;

// host to clients: the mission is over
struct EndGame {
	uint8_t winningTeam = NO_WINNER;	// below MAX_PLAYERS, or NO_WINNER
};

// A run of packed game chunks of one kind, for one mover or turret (its index
// in subject) or for nobody in particular (NO_SUBJECT).
struct ChunkBatch {
	ChunkKind kind = ChunkKind::Move;
	uint16_t subject = NO_SUBJECT;
	std::vector<uint32_t> words;
};

// Something that happened to the world, from the host: two words in a ChunkKind::World
// batch. What the fields mean by type is the game's, and it must check their ranges.
enum class WorldEventType : uint8_t {
	Artillery = 1,		// a, b: cell; value: commander | strike << 8 | seconds << 16
	Mine = 2,			// a, b: cell; value: state | explosion << 8
	TerrainFire = 3,	// a: seconds; value: part id
	Capture = 4,		// a: commander; b: team; value: part id
	KillLoss = 5,		// a: killer; b: loser
	ScriptMessage = 6,	// a, b: parameter, low 12 bits and high 12 bits; value: code
	Score = 7,			// a: team; value: points to add
	Resources = 8,		// a: commander; value: resource points to add, may be negative
	PilotKill = 9,		// a: roster index; b: vehicle class
	TurretTarget = 10	// a: turret index; b: roster index + 1, or 0 for none
};

constexpr size_t WORLD_EVENT_WORDS = 2;
constexpr uint16_t MAX_WORLD_FIELD = 0xFFF;	// a and b are 12 bits

struct WorldEvent {
	WorldEventType type = WorldEventType::Mine;
	uint16_t a = 0;
	uint16_t b = 0;
	uint32_t value = 0;
};

// False for a field that does not fit, or a type this build does not know.
bool encodeWorldEvent(const WorldEvent& event, uint32_t words[WORLD_EVENT_WORDS]);
bool decodeWorldEvent(const uint32_t words[WORLD_EVENT_WORDS], WorldEvent& event);

// A support vehicle or a recovery. A client asks with RequestVehicle or RequestRecovery, in a
// ChunkKind::Order batch with no subject; everything else goes from the host to the clients,
// in a ChunkKind::Support batch. The numbers are the stages of the 2001 sendReinforcement.
enum class SupportStage : uint8_t {
	RequestVehicle = 0,
	BeginVehicle = 1,		// the host took it: a unit that does not exist yet, and a VTOL
	DropVehicle = 2,		// the VTOL put it down on the host: it exists
	RequestRecovery = 3,
	BeginRecovery = 4,
	Recovered = 5,
	Refused = 8				// not allowed: cost goes back to the commander who paid it
};

constexpr size_t SUPPORT_EVENT_WORDS = 6;

struct SupportEvent {
	SupportStage stage = SupportStage::RequestVehicle;
	uint8_t commander = 0;		// below MAX_PLAYERS
	uint16_t rosterIndex = 0;	// of the new unit, or of the mech to recover
	uint16_t vehicleId = 0;
	int32_t cost = 0;			// not negative
	float x = 0.0f;				// exact, since a new unit starts here on every machine
	float y = 0.0f;
	float z = 0.0f;
};

// False for a stage this build does not know, or a field out of range.
bool encodeSupportEvent(const SupportEvent& event, uint32_t words[SUPPORT_EVENT_WORDS]);
bool decodeSupportEvent(const uint32_t words[SUPPORT_EVENT_WORDS], SupportEvent& event);

// A datagram on the local network, not on a connection: anyone can send anything to the
// port, so these open with a word of their own after the type. Who asks a question gets the
// answer whatever version it has: the announce says what the host speaks. Never change the
// fields before the name.
constexpr uint32_t LAN_MAGIC = 0x4D433247u;	// "MC2G"

struct LanQuery {
	uint16_t protocol = PROTOCOL_VERSION;
};

struct LanAnnounce {
	uint16_t protocol = PROTOCOL_VERSION;
	uint16_t gamePort = 0;
	uint32_t instance = 0;		// this game's own: one machine can be heard at two addresses
	uint8_t numPlayers = 0;		// who has joined, the host among them
	uint8_t maxPlayers = 0;
	bool inProgress = false;
	std::string name;			// what to call this game in a list
	std::string map;			// a mission's file name
};

// A lobby. The host holds it (netlobby.h) and sends the whole of it when it changes; a client
// asks for its own part of it to change; and a launch ends it.
constexpr uint8_t MAX_LOBBY_COLORS = 32;	// MAX_COLORS
constexpr uint8_t NO_COMMANDER = 0xFF;
constexpr int32_t NO_TIME_LIMIT = -1;

// what a game allows: LobbyState::flags
enum LobbyFlags : uint16_t {
	LOBBY_AIRSTRIKE = 1u << 0,
	LOBBY_GUARD_TOWER = 1u << 1,
	LOBBY_REPAIR_VEHICLE = 1u << 2,
	LOBBY_RECOVERY_TEAM = 1u << 3,
	LOBBY_SENSOR_PROBE = 1u << 4,
	LOBBY_SCOUT_COPTER = 1u << 5,
	LOBBY_MINELAYER = 1u << 6,
	LOBBY_RESOURCE_BUILDINGS = 1u << 7,
	LOBBY_RESOURCE_FOR_MECHS = 1u << 8,
	LOBBY_UNLIMITED_AMMO = 1u << 9,
	LOBBY_ALL_FLAGS = (1u << 10) - 1u
};

struct LobbyPlayer {
	uint8_t slot = 0;			// their commander in the session, below MAX_PLAYERS
	std::string name;
	uint8_t team = 0;			// below MAX_PLAYERS
	uint8_t baseColor = 0;		// below MAX_LOBBY_COLORS
	uint8_t stripeColor = 0;	// below MAX_LOBBY_COLORS
	bool ready = false;
};

// host to clients
struct LobbyState {
	uint32_t revision = 0;		// a different one is a change
	uint8_t maxPlayers = 0;		// 1 to MAX_PLAYERS, and no fewer than are in it
	std::string map;			// a mission's file name; empty until one is chosen
	int32_t timeLimit = NO_TIME_LIMIT;	// seconds
	int32_t resourcePoints = 0;
	uint16_t flags = 0;			// LobbyFlags
	std::vector<LobbyPlayer> players;
};

// client to host
struct LobbyChoice {
	uint8_t team = 0;
	uint8_t baseColor = 0;
	uint8_t stripeColor = 0;
	bool ready = false;
	uint32_t mapHash = 0;		// of the chosen map as this machine has it
};

// host to clients, ending a lobby: each slot's commander in the game, from 0 with no gaps.
struct LobbyLaunch {
	uint8_t numPlayers = 0;		// 1 to MAX_PLAYERS
	uint8_t commanders[MAX_PLAYERS] = { NO_COMMANDER, NO_COMMANDER, NO_COMMANDER, NO_COMMANDER, NO_COMMANDER, NO_COMMANDER, NO_COMMANDER, NO_COMMANDER };
};

// FNV-1a: fixed for good, as both ends of a game compare it. Run it on from one piece to the next.
constexpr uint32_t HASH_START = 2166136261u;
uint32_t hashBytes(uint32_t hash, const void* data, size_t size);

// Appends little-endian fields to a buffer.
class Writer {
public:
	explicit Writer(std::vector<uint8_t>& out) : out_(out) {}
	void u8(uint8_t v);
	void u16(uint16_t v);
	void u32(uint32_t v);
	void bytes(const void* data, size_t size);

private:
	std::vector<uint8_t>& out_;
};

// Reads little-endian fields from a buffer. Reading past the end sets a sticky
// failure and yields zeros, so a decoder can read first and check once.
class Reader {
public:
	Reader(const void* data, size_t size) : data_(static_cast<const uint8_t*>(data)), size_(size) {}
	uint8_t u8();
	uint16_t u16();
	uint32_t u32();
	bool bytes(void* out, size_t size);
	size_t remaining() const { return size_ - pos_; }
	bool ok() const { return ok_; }
	// All of the message was read, and nothing past it.
	bool finished() const { return ok_ && pos_ == size_; }

private:
	bool take(size_t size);
	const uint8_t* data_;
	size_t size_;
	size_t pos_ = 0;
	bool ok_ = true;
};

// The type of a message, without checking its body.
bool peekType(const void* data, size_t size, MessageType& type);

// The protocol of a Hello, readable even when the rest of it is from a version
// this build cannot decode. Check it first, and answer RejectReason::WrongVersion.
bool peekProtocol(const void* data, size_t size, uint16_t& protocol);

// Each encoder replaces out, and refuses a value its decoder would refuse.
bool encode(const Hello& message, std::vector<uint8_t>& out);
bool encode(const Welcome& message, std::vector<uint8_t>& out);
bool encode(const ChunkBatch& message, std::vector<uint8_t>& out);
bool encode(const Ready& message, std::vector<uint8_t>& out);
bool encode(const Start& message, std::vector<uint8_t>& out);
bool encode(const EndGame& message, std::vector<uint8_t>& out);
bool encode(const LanQuery& message, std::vector<uint8_t>& out);
bool encode(const LanAnnounce& message, std::vector<uint8_t>& out);
bool encode(const LobbyState& message, std::vector<uint8_t>& out);
bool encode(const LobbyChoice& message, std::vector<uint8_t>& out);
bool encode(const LobbyLaunch& message, std::vector<uint8_t>& out);

// Each decoder returns false, and leaves message unusable, unless the whole of
// data is one valid message of that type.
bool decode(const void* data, size_t size, Hello& message);
bool decode(const void* data, size_t size, Welcome& message);
bool decode(const void* data, size_t size, ChunkBatch& message);
bool decode(const void* data, size_t size, Ready& message);
bool decode(const void* data, size_t size, Start& message);
bool decode(const void* data, size_t size, EndGame& message);
bool decode(const void* data, size_t size, LanQuery& message);
bool decode(const void* data, size_t size, LanAnnounce& message);
bool decode(const void* data, size_t size, LobbyState& message);
bool decode(const void* data, size_t size, LobbyChoice& message);
bool decode(const void* data, size_t size, LobbyLaunch& message);

}

#endif
