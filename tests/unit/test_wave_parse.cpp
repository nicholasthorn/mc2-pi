// tests/unit/test_wave_parse.cpp
//
// wave_ParseWaveMemory(): the RIFF chunk walk behind every sound bite. RIFF
// pads chunks to even sizes only, so after an 18-byte "fmt " or an odd-sized
// chunk plus its pad byte, the next chunk header sits on a 2-byte boundary.
// The walk read those headers through a DWORD pointer, which UBSan flags as
// a misaligned load (MechCommander Omnitech's sounds); it now uses memcpy.

#include "doctest.h"

#include "waveparse.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace
{

void put32(std::vector<uint8_t>& f, uint32_t v)
{
	for (int i = 0; i < 4; ++i)
		f.push_back((uint8_t)(v >> (8 * i)));
}

void put16(std::vector<uint8_t>& f, uint16_t v)
{
	f.push_back((uint8_t)v);
	f.push_back((uint8_t)(v >> 8));
}

void putChunk(std::vector<uint8_t>& f, const char* id, const std::vector<uint8_t>& body)
{
	f.insert(f.end(), id, id + 4);
	put32(f, (uint32_t)body.size());
	f.insert(f.end(), body.begin(), body.end());
	if (body.size() & 1)
		f.push_back(0);		// RIFF pad byte
}

// PCM "fmt " body: 16 bytes, plus a cbSize word (18 bytes) when extended.
std::vector<uint8_t> fmtBody(bool extended)
{
	std::vector<uint8_t> b;
	put16(b, 1);			// wFormatTag: PCM
	put16(b, 1);			// nChannels
	put32(b, 22050);		// nSamplesPerSec
	put32(b, 44100);		// nAvgBytesPerSec
	put16(b, 2);			// nBlockAlign
	put16(b, 16);			// wBitsPerSample
	if (extended)
		put16(b, 0);		// cbSize
	return b;
}

// RIFF WAVE image holding the given chunks (already serialised).
std::vector<uint8_t> riffWave(const std::vector<uint8_t>& chunks)
{
	std::vector<uint8_t> f;
	const char riff[] = "RIFF";
	const char wave[] = "WAVE";
	f.insert(f.end(), riff, riff + 4);
	put32(f, (uint32_t)(4 + chunks.size()));
	f.insert(f.end(), wave, wave + 4);
	f.insert(f.end(), chunks.begin(), chunks.end());
	return f;
}

const std::vector<uint8_t> kSamples = {1, 2, 3, 4, 5, 6};

struct Parsed
{
	bool ok = false;
	MC2_WAVEFORMATEX* header = nullptr;
	MemoryPtr samples = nullptr;
	DWORD size = 0;
};

// Parses `image` placed `shift` bytes into a buffer, so a test can also put
// the whole image at an odd address.
Parsed parseAt(std::vector<uint8_t>& storage, const std::vector<uint8_t>& image, size_t shift)
{
	storage.assign(image.size() + shift, 0xCD);
	std::memcpy(storage.data() + shift, image.data(), image.size());
	Parsed r;
	r.ok = wave_ParseWaveMemory(storage.data() + shift, &r.header, &r.samples, &r.size);
	return r;
}

void checkPcm(const Parsed& r)
{
	REQUIRE(r.ok);
	REQUIRE(r.header != nullptr);
	// copy out first: CHECK binds a reference, and the packed fields may be
	// misaligned
	MC2_WAVEFORMATEX h;
	std::memcpy(&h, r.header, sizeof(h));
	const unsigned formatTag = h.wFormatTag, channels = h.nChannels, blockAlign = h.nBlockAlign;
	const unsigned rate = h.nSamplesPerSec, avgBytes = h.nAvgBytesPerSec;
	CHECK(formatTag == 1);
	CHECK(channels == 1);
	CHECK(rate == 22050);
	CHECK(avgBytes == 44100);
	CHECK(blockAlign == 2);
	REQUIRE(r.samples != nullptr);
	CHECK(r.size == kSamples.size());
	CHECK(std::memcmp(r.samples, kSamples.data(), kSamples.size()) == 0);
}

} // namespace

TEST_SUITE("WaveParse")
{

TEST_CASE("stock layout: 16-byte fmt then data")
{
	std::vector<uint8_t> chunks;
	putChunk(chunks, "fmt ", fmtBody(false));
	putChunk(chunks, "data", kSamples);
	const std::vector<uint8_t> image = riffWave(chunks);

	std::vector<uint8_t> buf;
	const Parsed r = parseAt(buf, image, 0);
	checkPcm(r);
	CHECK(r.samples == buf.data() + 12 + 8 + 16 + 8);
}

TEST_CASE("18-byte fmt leaves the data header 2-byte aligned")
{
	std::vector<uint8_t> chunks;
	putChunk(chunks, "fmt ", fmtBody(true));
	putChunk(chunks, "data", kSamples);
	const std::vector<uint8_t> image = riffWave(chunks);

	std::vector<uint8_t> buf;
	const Parsed r = parseAt(buf, image, 0);
	checkPcm(r);
	CHECK(r.samples == buf.data() + 12 + 8 + 18 + 8);
}

TEST_CASE("odd-sized chunk: the pad byte is skipped")
{
	std::vector<uint8_t> chunks;
	putChunk(chunks, "fmt ", fmtBody(false));
	putChunk(chunks, "LIST", {'a', 'b', 'c', 'd', 'e'});	// 5 bytes + pad
	putChunk(chunks, "data", kSamples);
	const std::vector<uint8_t> image = riffWave(chunks);

	std::vector<uint8_t> buf;
	const Parsed r = parseAt(buf, image, 0);
	checkPcm(r);
	CHECK(r.samples == buf.data() + 12 + 8 + 16 + 8 + 6 + 8);
}

TEST_CASE("image at an odd address")
{
	std::vector<uint8_t> chunks;
	putChunk(chunks, "fmt ", fmtBody(true));
	putChunk(chunks, "data", kSamples);
	const std::vector<uint8_t> image = riffWave(chunks);

	std::vector<uint8_t> buf;
	checkPcm(parseAt(buf, image, 1));
	checkPcm(parseAt(buf, image, 3));
}

TEST_CASE("rejects non-RIFF, non-WAVE and short fmt")
{
	std::vector<uint8_t> chunks;
	putChunk(chunks, "fmt ", fmtBody(false));
	putChunk(chunks, "data", kSamples);
	std::vector<uint8_t> buf;

	std::vector<uint8_t> notRiff = riffWave(chunks);
	notRiff[0] = 'X';
	CHECK_FALSE(parseAt(buf, notRiff, 0).ok);

	std::vector<uint8_t> notWave = riffWave(chunks);
	notWave[8] = 'X';
	CHECK_FALSE(parseAt(buf, notWave, 0).ok);

	std::vector<uint8_t> shortFmt;
	putChunk(shortFmt, "fmt ", {1, 0, 1, 0});
	putChunk(shortFmt, "data", kSamples);
	CHECK_FALSE(parseAt(buf, riffWave(shortFmt), 0).ok);
}

TEST_CASE("no data chunk fails")
{
	std::vector<uint8_t> chunks;
	putChunk(chunks, "fmt ", fmtBody(false));
	std::vector<uint8_t> buf;
	const Parsed r = parseAt(buf, riffWave(chunks), 0);
	CHECK_FALSE(r.ok);
	CHECK(r.header != nullptr);
	CHECK(r.samples == nullptr);
}

}
