// code/netwire.cpp: see netwire.h

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
	return m.reject == RejectReason::WrongVersion || m.reject == RejectReason::WrongData || m.reject == RejectReason::GameFull;
}

bool validKind(ChunkKind kind)
{
	const uint8_t k = static_cast<uint8_t>(kind);
	return k >= static_cast<uint8_t>(ChunkKind::Move) && k <= static_cast<uint8_t>(ChunkKind::Order);
}

// Reads the type byte and checks it, for a decoder.
bool expectType(Reader& in, MessageType type)
{
	return in.u8() == static_cast<uint8_t>(type) && in.ok();
}

}

bool peekType(const void* data, size_t size, MessageType& type)
{
	if (size < 1)
		return false;
	const uint8_t t = *static_cast<const uint8_t*>(data);
	if (t < static_cast<uint8_t>(MessageType::Hello) || t > static_cast<uint8_t>(MessageType::ChunkBatch))
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

}
