// code/netwire.cpp: see netwire.h

#include <cmath>
#include "netwire.h"

#include <cstring>

namespace netwire {

void Writer::u8(uint8_t v)
{
	out_.push_back(v);
}

void Writer::u16(uint16_t v)
{
	u8(static_cast<uint8_t>(v & 0xFFu));
	u8(static_cast<uint8_t>(v >> 8));
}

void Writer::u32(uint32_t v)
{
	u16(static_cast<uint16_t>(v & 0xFFFFu));
	u16(static_cast<uint16_t>(v >> 16));
}

void Writer::bytes(const void* data, size_t size)
{
	const uint8_t* p = static_cast<const uint8_t*>(data);
	out_.insert(out_.end(), p, p + size);
}

bool Reader::take(size_t size)
{
	if (!ok_ || size > remaining()) {
		ok_ = false;
		return false;
	}
	return true;
}

uint8_t Reader::u8()
{
	if (!take(1))
		return 0;
	return data_[pos_++];
}

uint16_t Reader::u16()
{
	if (!take(2))
		return 0;
	const uint16_t v = static_cast<uint16_t>(data_[pos_] | (data_[pos_ + 1] << 8));
	pos_ += 2;
	return v;
}

uint32_t Reader::u32()
{
	if (!take(4))
		return 0;
	const uint32_t v = static_cast<uint32_t>(data_[pos_])
		| (static_cast<uint32_t>(data_[pos_ + 1]) << 8)
		| (static_cast<uint32_t>(data_[pos_ + 2]) << 16)
		| (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
	pos_ += 4;
	return v;
}

bool Reader::bytes(void* out, size_t size)
{
	if (!take(size))
		return false;
	if (size > 0)
		memcpy(out, data_ + pos_, size);
	pos_ += size;
	return true;
}

namespace {

// The name goes into the game's chat and player lists, which are 2001 code with
// signed char: printable ASCII only, and no % in case one reaches a format string.
bool validName(const std::string& name)
{
	if (name.size() > MAX_NAME_LENGTH)
		return false;
	for (char c : name) {
		const uint8_t b = static_cast<uint8_t>(c);
		if (b < 0x20u || b > 0x7Eu || b == '%')
			return false;
	}
	return true;
}

bool validWelcome(const Welcome& m)
{
	if (m.reject == RejectReason::None)
		return m.numPlayers >= 1 && m.numPlayers <= MAX_PLAYERS && m.commanderId < m.numPlayers;
	return m.reject == RejectReason::WrongVersion || m.reject == RejectReason::WrongData || m.reject == RejectReason::GameFull || m.reject == RejectReason::WrongRoster;
}

bool validKind(ChunkKind kind)
{
	const uint8_t k = static_cast<uint8_t>(kind);
	return k >= static_cast<uint8_t>(ChunkKind::Move) && k <= static_cast<uint8_t>(ChunkKind::Support);
}

// Reads the type byte and checks it, for a decoder.
bool expectType(Reader& in, MessageType type)
{
	return in.u8() == static_cast<uint8_t>(type) && in.ok();
}

}

bool encodeWorldEvent(const WorldEvent& e, uint32_t words[WORLD_EVENT_WORDS])
{
	const uint8_t t = static_cast<uint8_t>(e.type);
	if (t < static_cast<uint8_t>(WorldEventType::Artillery) || t > static_cast<uint8_t>(WorldEventType::TurretTarget)
		|| e.a > MAX_WORLD_FIELD || e.b > MAX_WORLD_FIELD)
		return false;
	words[0] = static_cast<uint32_t>(t) | (static_cast<uint32_t>(e.a) << 8) | (static_cast<uint32_t>(e.b) << 20);
	words[1] = e.value;
	return true;
}

static bool validSupportEvent(const SupportEvent& e)
{
	switch (e.stage) {
		case SupportStage::RequestVehicle:
		case SupportStage::BeginVehicle:
		case SupportStage::DropVehicle:
		case SupportStage::RequestRecovery:
		case SupportStage::BeginRecovery:
		case SupportStage::Recovered:
		case SupportStage::Refused:
			break;
		default:
			return false;
	}
	return e.commander < MAX_PLAYERS && e.cost >= 0 && std::isfinite(e.x) && std::isfinite(e.y) && std::isfinite(e.z);
}

bool encodeSupportEvent(const SupportEvent& e, uint32_t words[SUPPORT_EVENT_WORDS])
{
	if (!validSupportEvent(e))
		return false;
	words[0] = static_cast<uint32_t>(e.stage) | (static_cast<uint32_t>(e.commander) << 8) | (static_cast<uint32_t>(e.rosterIndex) << 16);
	words[1] = e.vehicleId;
	words[2] = static_cast<uint32_t>(e.cost);
	std::memcpy(&words[3], &e.x, 4);
	std::memcpy(&words[4], &e.y, 4);
	std::memcpy(&words[5], &e.z, 4);
	return true;
}

bool decodeSupportEvent(const uint32_t words[SUPPORT_EVENT_WORDS], SupportEvent& e)
{
	e.stage = static_cast<SupportStage>(words[0] & 0xFFu);
	e.commander = static_cast<uint8_t>((words[0] >> 8) & 0xFFu);
	e.rosterIndex = static_cast<uint16_t>(words[0] >> 16);
	if (words[1] > 0xFFFFu)
		return false;
	e.vehicleId = static_cast<uint16_t>(words[1]);
	e.cost = static_cast<int32_t>(words[2]);
	std::memcpy(&e.x, &words[3], 4);
	std::memcpy(&e.y, &words[4], 4);
	std::memcpy(&e.z, &words[5], 4);
	return validSupportEvent(e);
}

bool decodeWorldEvent(const uint32_t words[WORLD_EVENT_WORDS], WorldEvent& e)
{
	const uint8_t t = static_cast<uint8_t>(words[0] & 0xFFu);
	if (t < static_cast<uint8_t>(WorldEventType::Artillery) || t > static_cast<uint8_t>(WorldEventType::TurretTarget))
		return false;
	e.type = static_cast<WorldEventType>(t);
	e.a = static_cast<uint16_t>((words[0] >> 8) & MAX_WORLD_FIELD);
	e.b = static_cast<uint16_t>(words[0] >> 20);
	e.value = words[1];
	return true;
}

bool peekType(const void* data, size_t size, MessageType& type)
{
	if (size < 1)
		return false;
	const uint8_t t = *static_cast<const uint8_t*>(data);
	if (t < static_cast<uint8_t>(MessageType::Hello) || t > static_cast<uint8_t>(MessageType::LobbyLaunch))
		return false;
	type = static_cast<MessageType>(t);
	return true;
}

bool peekProtocol(const void* data, size_t size, uint16_t& protocol)
{
	Reader in(data, size);
	if (!expectType(in, MessageType::Hello))
		return false;
	protocol = in.u16();
	return in.ok();
}

uint32_t hashBytes(uint32_t hash, const void* data, size_t size)
{
	const uint8_t* bytes = static_cast<const uint8_t*>(data);
	for (size_t i = 0; i < size; i++) {
		hash ^= bytes[i];
		hash *= 16777619u;
	}
	return hash;
}

static bool validAnnounce(const LanAnnounce& m)
{
	return m.gamePort != 0 && m.maxPlayers >= 1 && m.maxPlayers <= MAX_PLAYERS && m.numPlayers <= m.maxPlayers
		&& validName(m.name) && validName(m.map);	// a lobby's map can still be to choose
}

bool encode(const LanQuery& m, std::vector<uint8_t>& out)
{
	out.clear();
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::LanQuery));
	w.u32(LAN_MAGIC);
	w.u16(m.protocol);
	return true;
}

bool decode(const void* data, size_t size, LanQuery& m)
{
	if (size > MAX_MESSAGE_SIZE)
		return false;
	Reader in(data, size);
	if (!expectType(in, MessageType::LanQuery) || in.u32() != LAN_MAGIC)
		return false;
	m.protocol = in.u16();
	return in.finished();
}

bool encode(const LanAnnounce& m, std::vector<uint8_t>& out)
{
	out.clear();
	if (!validAnnounce(m))
		return false;
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::LanAnnounce));
	w.u32(LAN_MAGIC);
	w.u16(m.protocol);
	w.u16(m.gamePort);
	w.u32(m.instance);
	w.u8(m.numPlayers);
	w.u8(m.maxPlayers);
	w.u8(m.inProgress ? 1 : 0);
	w.u8(static_cast<uint8_t>(m.name.size()));
	w.bytes(m.name.data(), m.name.size());
	w.u8(static_cast<uint8_t>(m.map.size()));
	w.bytes(m.map.data(), m.map.size());
	return true;
}

static bool readShortText(Reader& in, std::string& text)
{
	const size_t length = in.u8();
	if (!in.ok() || length > MAX_NAME_LENGTH || length > in.remaining())
		return false;
	text.resize(length);
	return (length == 0) || in.bytes(&text[0], length);
}

bool decode(const void* data, size_t size, LanAnnounce& m)
{
	if (size > MAX_MESSAGE_SIZE)
		return false;
	Reader in(data, size);
	if (!expectType(in, MessageType::LanAnnounce) || in.u32() != LAN_MAGIC)
		return false;
	m.protocol = in.u16();
	m.gamePort = in.u16();
	m.instance = in.u32();
	m.numPlayers = in.u8();
	m.maxPlayers = in.u8();
	const uint8_t inProgress = in.u8();
	if (!in.ok() || inProgress > 1)
		return false;
	m.inProgress = (inProgress != 0);
	if (!readShortText(in, m.name) || !readShortText(in, m.map))
		return false;
	return in.finished() && validAnnounce(m);
}

static bool validLobbyState(const LobbyState& m)
{
	if (m.maxPlayers < 1 || m.maxPlayers > MAX_PLAYERS || m.players.size() > m.maxPlayers
		|| !validName(m.map) || m.timeLimit < NO_TIME_LIMIT || m.resourcePoints < 0 || (m.flags & ~LOBBY_ALL_FLAGS) != 0)
		return false;
	bool slotTaken[MAX_PLAYERS] = {};
	for (const LobbyPlayer& player : m.players) {
		if (player.slot >= MAX_PLAYERS || slotTaken[player.slot] || !validName(player.name)
			|| player.team >= MAX_PLAYERS || player.baseColor >= MAX_LOBBY_COLORS || player.stripeColor >= MAX_LOBBY_COLORS)
			return false;
		slotTaken[player.slot] = true;
	}
	return true;
}

bool encode(const LobbyState& m, std::vector<uint8_t>& out)
{
	out.clear();
	if (!validLobbyState(m))
		return false;
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::LobbyState));
	w.u32(m.revision);
	w.u8(m.maxPlayers);
	w.u8(static_cast<uint8_t>(m.map.size()));
	w.bytes(m.map.data(), m.map.size());
	w.u32(static_cast<uint32_t>(m.timeLimit));
	w.u32(static_cast<uint32_t>(m.resourcePoints));
	w.u16(m.flags);
	w.u8(static_cast<uint8_t>(m.players.size()));
	for (const LobbyPlayer& player : m.players) {
		w.u8(player.slot);
		w.u8(static_cast<uint8_t>(player.name.size()));
		w.bytes(player.name.data(), player.name.size());
		w.u8(player.team);
		w.u8(player.baseColor);
		w.u8(player.stripeColor);
		w.u8(player.ready ? 1 : 0);
	}
	return true;
}

bool decode(const void* data, size_t size, LobbyState& m)
{
	if (size > MAX_MESSAGE_SIZE)
		return false;
	Reader in(data, size);
	if (!expectType(in, MessageType::LobbyState))
		return false;
	m.revision = in.u32();
	m.maxPlayers = in.u8();
	if (!readShortText(in, m.map))
		return false;
	m.timeLimit = static_cast<int32_t>(in.u32());
	m.resourcePoints = static_cast<int32_t>(in.u32());
	m.flags = in.u16();
	const size_t count = in.u8();
	if (!in.ok() || count > MAX_PLAYERS)
		return false;
	m.players.clear();
	for (size_t i = 0; i < count; i++) {
		LobbyPlayer player;
		player.slot = in.u8();
		if (!readShortText(in, player.name))
			return false;
		player.team = in.u8();
		player.baseColor = in.u8();
		player.stripeColor = in.u8();
		const uint8_t ready = in.u8();
		if (!in.ok() || ready > 1)
			return false;
		player.ready = (ready != 0);
		m.players.push_back(player);
	}
	return in.finished() && validLobbyState(m);
}

static bool validLobbyChoice(const LobbyChoice& m)
{
	return m.team < MAX_PLAYERS && m.baseColor < MAX_LOBBY_COLORS && m.stripeColor < MAX_LOBBY_COLORS;
}

bool encode(const LobbyChoice& m, std::vector<uint8_t>& out)
{
	out.clear();
	if (!validLobbyChoice(m))
		return false;
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::LobbyChoice));
	w.u8(m.team);
	w.u8(m.baseColor);
	w.u8(m.stripeColor);
	w.u8(m.ready ? 1 : 0);
	w.u32(m.mapHash);
	return true;
}

bool decode(const void* data, size_t size, LobbyChoice& m)
{
	if (size > MAX_MESSAGE_SIZE)
		return false;
	Reader in(data, size);
	if (!expectType(in, MessageType::LobbyChoice))
		return false;
	m.team = in.u8();
	m.baseColor = in.u8();
	m.stripeColor = in.u8();
	const uint8_t ready = in.u8();
	m.mapHash = in.u32();
	if (!in.finished() || ready > 1)
		return false;
	m.ready = (ready != 0);
	return validLobbyChoice(m);
}

static bool validLobbyLaunch(const LobbyLaunch& m)
{
	if (m.numPlayers < 1 || m.numPlayers > MAX_PLAYERS)
		return false;
	bool taken[MAX_PLAYERS] = {};
	int count = 0;
	for (uint8_t commander : m.commanders) {
		if (commander == NO_COMMANDER)
			continue;
		if (commander >= m.numPlayers || taken[commander])
			return false;
		taken[commander] = true;
		count++;
	}
	return count == m.numPlayers;
}

bool encode(const LobbyLaunch& m, std::vector<uint8_t>& out)
{
	out.clear();
	if (!validLobbyLaunch(m))
		return false;
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::LobbyLaunch));
	w.u8(m.numPlayers);
	w.bytes(m.commanders, MAX_PLAYERS);
	return true;
}

bool decode(const void* data, size_t size, LobbyLaunch& m)
{
	if (size > MAX_MESSAGE_SIZE)
		return false;
	Reader in(data, size);
	if (!expectType(in, MessageType::LobbyLaunch))
		return false;
	m.numPlayers = in.u8();
	if (!in.bytes(m.commanders, MAX_PLAYERS))
		return false;
	return in.finished() && validLobbyLaunch(m);
}

bool encode(const Hello& m, std::vector<uint8_t>& out)
{
	out.clear();
	if (!validName(m.name))
		return false;
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::Hello));
	w.u16(m.protocol);
	w.u32(m.dataHash);
	w.u8(static_cast<uint8_t>(m.name.size()));
	w.bytes(m.name.data(), m.name.size());
	return true;
}

bool decode(const void* data, size_t size, Hello& m)
{
	if (size > MAX_MESSAGE_SIZE)
		return false;
	Reader in(data, size);
	if (!expectType(in, MessageType::Hello))
		return false;
	m.protocol = in.u16();
	m.dataHash = in.u32();
	const size_t length = in.u8();
	if (!in.ok() || length > MAX_NAME_LENGTH || length > in.remaining())
		return false;
	m.name.resize(length);
	if (length > 0)
		in.bytes(&m.name[0], length);
	return in.finished() && validName(m.name);
}

bool encode(const Welcome& m, std::vector<uint8_t>& out)
{
	out.clear();
	if (!validWelcome(m))
		return false;
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::Welcome));
	w.u8(static_cast<uint8_t>(m.reject));
	w.u8(m.commanderId);
	w.u8(m.numPlayers);
	return true;
}

bool decode(const void* data, size_t size, Welcome& m)
{
	if (size > MAX_MESSAGE_SIZE)
		return false;
	Reader in(data, size);
	if (!expectType(in, MessageType::Welcome))
		return false;
	m.reject = static_cast<RejectReason>(in.u8());
	m.commanderId = in.u8();
	m.numPlayers = in.u8();
	return in.finished() && validWelcome(m);
}

bool encode(const ChunkBatch& m, std::vector<uint8_t>& out)
{
	out.clear();
	if (!validKind(m.kind) || m.words.size() > MAX_BATCH_CHUNKS)
		return false;
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::ChunkBatch));
	w.u8(static_cast<uint8_t>(m.kind));
	w.u16(m.subject);
	w.u16(static_cast<uint16_t>(m.words.size()));
	for (uint32_t word : m.words)
		w.u32(word);
	return true;
}

bool decode(const void* data, size_t size, ChunkBatch& m)
{
	if (size > MAX_MESSAGE_SIZE)
		return false;
	Reader in(data, size);
	if (!expectType(in, MessageType::ChunkBatch))
		return false;
	m.kind = static_cast<ChunkKind>(in.u8());
	m.subject = in.u16();
	const size_t count = in.u16();
	// the size is checked before the count is trusted with an allocation
	if (!in.ok() || !validKind(m.kind) || count > MAX_BATCH_CHUNKS || count * 4 != in.remaining())
		return false;
	m.words.resize(count);
	for (size_t i = 0; i < count; i++)
		m.words[i] = in.u32();
	return in.finished();
}

bool encode(const Ready& m, std::vector<uint8_t>& out)
{
	out.clear();
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::Ready));
	w.u32(m.rosterHash);
	return true;
}

bool decode(const void* data, size_t size, Ready& m)
{
	Reader in(data, size);
	if (!expectType(in, MessageType::Ready))
		return false;
	m.rosterHash = in.u32();
	return in.finished();
}

bool encode(const Start&, std::vector<uint8_t>& out)
{
	out.clear();
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::Start));
	return true;
}

bool decode(const void* data, size_t size, Start&)
{
	Reader in(data, size);
	return expectType(in, MessageType::Start) && in.finished();
}

bool encode(const EndGame& m, std::vector<uint8_t>& out)
{
	out.clear();
	if (m.winningTeam >= MAX_PLAYERS && m.winningTeam != NO_WINNER)
		return false;
	Writer w(out);
	w.u8(static_cast<uint8_t>(MessageType::EndGame));
	w.u8(m.winningTeam);
	return true;
}

bool decode(const void* data, size_t size, EndGame& m)
{
	Reader in(data, size);
	if (!expectType(in, MessageType::EndGame))
		return false;
	m.winningTeam = in.u8();
	return in.finished() && (m.winningTeam < MAX_PLAYERS || m.winningTeam == NO_WINNER);
}

}
