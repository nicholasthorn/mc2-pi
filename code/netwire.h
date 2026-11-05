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
constexpr uint16_t PROTOCOL_VERSION = 6;

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
	EndGame = 6
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

// Each decoder returns false, and leaves message unusable, unless the whole of
// data is one valid message of that type.
bool decode(const void* data, size_t size, Hello& message);
bool decode(const void* data, size_t size, Welcome& message);
bool decode(const void* data, size_t size, ChunkBatch& message);
bool decode(const void* data, size_t size, Ready& message);
bool decode(const void* data, size_t size, Start& message);
bool decode(const void* data, size_t size, EndGame& message);

}

#endif
