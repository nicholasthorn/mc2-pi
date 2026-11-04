// tests/unit/test_netwire.cpp
//
// netwire: the multiplayer wire format. The exact bytes are pinned, since they
// must match between x64 and arm64, Linux and Windows (where long differs), and
// the decoders must survive anything: their input comes from the network. CI
// runs this under ASan + UBSan, which is what makes the garbage cases a fuzz test.

#include "doctest.h"

#include "netwire.h"

#include <cstdint>
#include <vector>

using namespace netwire;

// Fixed seed, so a failure repeats: no test depends on the clock.
static uint32_t nextRandom(uint32_t& state)
{
	state = state * 1664525u + 1013904223u;
	return state >> 8;
}

TEST_SUITE("netwire") {

TEST_CASE("fields are little-endian")
{
	std::vector<uint8_t> out;
	Writer w(out);
	w.u8(0x12);
	w.u16(0x3456);
	w.u32(0x789ABCDEu);
	const std::vector<uint8_t> expected = { 0x12, 0x56, 0x34, 0xDE, 0xBC, 0x9A, 0x78 };
	CHECK(out == expected);

	Reader r(out.data(), out.size());
	CHECK(r.u8() == 0x12);
	CHECK(r.u16() == 0x3456);
	CHECK(r.u32() == 0x789ABCDEu);
	CHECK(r.finished());
}

TEST_CASE("reading past the end fails and stays failed")
{
	const uint8_t data[3] = { 1, 2, 3 };
	Reader r(data, sizeof(data));
	CHECK(r.u32() == 0);
	CHECK_FALSE(r.ok());
	CHECK(r.u8() == 0);	// there was a byte left, but the reader has failed
	CHECK_FALSE(r.finished());
}

TEST_CASE("Hello: exact bytes and round trip")
{
	Hello m;
	m.protocol = 0x0102;
	m.dataHash = 0xAABBCCDDu;
	m.name = "Ace";
	std::vector<uint8_t> out;
	REQUIRE(encode(m, out));
	const std::vector<uint8_t> expected = { 1, 0x02, 0x01, 0xDD, 0xCC, 0xBB, 0xAA, 3, 'A', 'c', 'e' };
	CHECK(out == expected);

	MessageType type = MessageType::Welcome;
	REQUIRE(peekType(out.data(), out.size(), type));
	CHECK(type == MessageType::Hello);

	Hello back;
	REQUIRE(decode(out.data(), out.size(), back));
	CHECK(back.protocol == m.protocol);
	CHECK(back.dataHash == m.dataHash);
	CHECK(back.name == m.name);
}

TEST_CASE("Hello: an empty name is fine")
{
	std::vector<uint8_t> out;
	REQUIRE(encode(Hello(), out));
	Hello back;
	back.name = "stale";
	REQUIRE(decode(out.data(), out.size(), back));
	CHECK(back.name.empty());
}

TEST_CASE("Hello: bad names are refused both ways")
{
	std::vector<uint8_t> out;
	Hello m;
	m.name = std::string(MAX_NAME_LENGTH + 1, 'x');
	CHECK_FALSE(encode(m, out));
	m.name = "a\nb";
	CHECK_FALSE(encode(m, out));

	m.name = "abc";
	REQUIRE(encode(m, out));
	out[out.size() - 1] = 0x07;	// a control character on the wire
	Hello back;
	CHECK_FALSE(decode(out.data(), out.size(), back));

	m.name = "abc";
	REQUIRE(encode(m, out));
	out[7] = 200;				// a length past the end of the message
	CHECK_FALSE(decode(out.data(), out.size(), back));
}

TEST_CASE("Welcome: round trip and exact bytes")
{
	Welcome m;
	m.commanderId = 3;
	m.numPlayers = 4;
	std::vector<uint8_t> out;
	REQUIRE(encode(m, out));
	const std::vector<uint8_t> expected = { 2, 0, 3, 4 };
	CHECK(out == expected);

	Welcome back;
	REQUIRE(decode(out.data(), out.size(), back));
	CHECK(back.reject == RejectReason::None);
	CHECK(back.commanderId == 3);
	CHECK(back.numPlayers == 4);
}

TEST_CASE("Welcome: a rejection needs no commander")
{
	Welcome m;
	m.reject = RejectReason::WrongData;
	std::vector<uint8_t> out;
	REQUIRE(encode(m, out));
	Welcome back;
	REQUIRE(decode(out.data(), out.size(), back));
	CHECK(back.reject == RejectReason::WrongData);
}

TEST_CASE("Welcome: out of range values are refused")
{
	std::vector<uint8_t> out;
	Welcome m;
	m.numPlayers = 0;
	CHECK_FALSE(encode(m, out));
	m.numPlayers = MAX_PLAYERS + 1;
	CHECK_FALSE(encode(m, out));
	m.numPlayers = 2;
	m.commanderId = 2;		// commanders count from 0
	CHECK_FALSE(encode(m, out));

	Welcome back;
	const uint8_t badReason[] = { 2, 99, 0, 0 };
	CHECK_FALSE(decode(badReason, sizeof(badReason), back));
	const uint8_t badCommander[] = { 2, 0, 200, 4 };
	CHECK_FALSE(decode(badCommander, sizeof(badCommander), back));
}

TEST_CASE("ChunkBatch: exact bytes and round trip")
{
	ChunkBatch m;
	m.kind = ChunkKind::WeaponFire;
	m.subject = 0x0102;
	m.words = { 0x11223344u, 0xFFFFFFFFu };
	std::vector<uint8_t> out;
	REQUIRE(encode(m, out));
	const std::vector<uint8_t> expected = {
		3, 3, 0x02, 0x01, 0x02, 0x00,
		0x44, 0x33, 0x22, 0x11,
		0xFF, 0xFF, 0xFF, 0xFF };
	CHECK(out == expected);

	ChunkBatch back;
	REQUIRE(decode(out.data(), out.size(), back));
	CHECK(back.kind == m.kind);
	CHECK(back.subject == m.subject);
	CHECK(back.words == m.words);
}

TEST_CASE("ChunkBatch: the largest batch fits a message")
{
	ChunkBatch m;
	m.words.assign(MAX_BATCH_CHUNKS, 7u);
	std::vector<uint8_t> out;
	REQUIRE(encode(m, out));
	CHECK(out.size() <= MAX_MESSAGE_SIZE);
	ChunkBatch back;
	REQUIRE(decode(out.data(), out.size(), back));
	CHECK(back.words.size() == MAX_BATCH_CHUNKS);

	m.words.push_back(7u);
	CHECK_FALSE(encode(m, out));
}

TEST_CASE("ChunkBatch: a count that disagrees with the size is refused")
{
	ChunkBatch back;
	// claims 65535 words and holds none: must not allocate on the claim's say-so
	const uint8_t tooMany[] = { 3, 1, 0, 0, 0xFF, 0xFF };
	CHECK_FALSE(decode(tooMany, sizeof(tooMany), back));
	// claims one word and holds two
	const uint8_t tooFew[] = { 3, 1, 0, 0, 1, 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	CHECK_FALSE(decode(tooFew, sizeof(tooFew), back));
	const uint8_t badKind[] = { 3, 99, 0, 0, 0, 0 };
	CHECK_FALSE(decode(badKind, sizeof(badKind), back));
}

TEST_CASE("a message of the wrong type is refused")
{
	std::vector<uint8_t> out;
	REQUIRE(encode(Hello(), out));
	Welcome w;
	ChunkBatch b;
	CHECK_FALSE(decode(out.data(), out.size(), w));
	CHECK_FALSE(decode(out.data(), out.size(), b));

	MessageType type;
	const uint8_t unknown = 200;
	CHECK_FALSE(peekType(&unknown, 1, type));
	CHECK_FALSE(peekType(&unknown, 0, type));
}

TEST_CASE("every truncation of a valid message is refused")
{
	Hello h;
	h.name = "Ace";
	std::vector<uint8_t> out;
	REQUIRE(encode(h, out));
	for (size_t n = 0; n < out.size(); n++) {
		Hello back;
		CHECK_FALSE(decode(out.data(), n, back));
	}

	Welcome w;
	w.numPlayers = 2;
	REQUIRE(encode(w, out));
	for (size_t n = 0; n < out.size(); n++) {
		Welcome back;
		CHECK_FALSE(decode(out.data(), n, back));
	}

	ChunkBatch b;
	b.words = { 1u, 2u, 3u };
	REQUIRE(encode(b, out));
	for (size_t n = 0; n < out.size(); n++) {
		ChunkBatch back;
		CHECK_FALSE(decode(out.data(), n, back));
	}
}

TEST_CASE("a trailing byte is refused")
{
	std::vector<uint8_t> out;
	REQUIRE(encode(Hello(), out));
	out.push_back(0);
	Hello back;
	CHECK_FALSE(decode(out.data(), out.size(), back));
}

// Whatever a decoder accepts must encode back to the same bytes: there is one
// way to write each message, so nothing slips past validation in a second form.
template <typename Message>
static void decodeAndCheck(const std::vector<uint8_t>& bytes)
{
	Message m;
	if (decode(bytes.data(), bytes.size(), m)) {
		std::vector<uint8_t> again;
		REQUIRE(encode(m, again));
		CHECK(again == bytes);
	}
}

static void decodeAll(const std::vector<uint8_t>& bytes)
{
	MessageType type;
	peekType(bytes.data(), bytes.size(), type);
	decodeAndCheck<Hello>(bytes);
	decodeAndCheck<Welcome>(bytes);
	decodeAndCheck<ChunkBatch>(bytes);
}

TEST_CASE("garbage never crashes a decoder")
{
	uint32_t seed = 12345u;
	std::vector<uint8_t> bytes;
	for (int i = 0; i < 20000; i++) {
		bytes.resize(nextRandom(seed) % 64u);
		for (uint8_t& b : bytes)
			b = static_cast<uint8_t>(nextRandom(seed));
		// a valid type byte half the time, to get past the first check
		if (!bytes.empty() && (nextRandom(seed) & 1u))
			bytes[0] = static_cast<uint8_t>(1u + nextRandom(seed) % 3u);
		decodeAll(bytes);
	}
}

TEST_CASE("damaged valid messages never crash a decoder")
{
	Hello h;
	h.name = "A longer pilot name";
	ChunkBatch b;
	b.kind = ChunkKind::Status;
	b.subject = 12;
	b.words = { 1u, 2u, 3u, 4u, 5u };
	Welcome w;
	w.numPlayers = 8;
	w.commanderId = 7;

	std::vector<std::vector<uint8_t>> valid(3);
	REQUIRE(encode(h, valid[0]));
	REQUIRE(encode(b, valid[1]));
	REQUIRE(encode(w, valid[2]));

	uint32_t seed = 777u;
	for (int i = 0; i < 20000; i++) {
		std::vector<uint8_t> bytes = valid[nextRandom(seed) % valid.size()];
		const uint32_t flips = 1u + nextRandom(seed) % 4u;
		for (uint32_t f = 0; f < flips; f++)
			bytes[nextRandom(seed) % bytes.size()] = static_cast<uint8_t>(nextRandom(seed));
		decodeAll(bytes);
	}
}

}
