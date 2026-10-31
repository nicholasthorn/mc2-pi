// tests/unit/test_lzdecomp.cpp
//
// LZDecomp reads two formats:
//   - zlib, which this port's LZCompress writes (FSTs built by makefst,
//     cached textures, packets);
//   - Microsoft's original variable-width LZW (9 to 12 bit codes, CLEAR 256,
//     EOF 257, first free code 258), which the 2001 x86 asm compressor in
//     mclib/lzcomp.cpp wrote. Retail-era and mod archives (MechCommander
//     Omnitech's tgl.fst) use it.
//
// No third-party data is used: original-format streams come from a test-only
// encoder that follows the asm compressor step by step, or are built by hand
// code by code.

#include "doctest.h"

#include "file.h"
#include "ffile.h"
#include "fastfile.h"
#include "lz.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace
{

typedef std::vector<uint8_t> Bytes;

const uint32_t kClear = 256;
const uint32_t kEof = 257;
const uint32_t kFirstFree = 258;

// LSB-first bit packer, as the asm writeCode (code << BitOffset, OR into the
// partial byte).
struct BitWriter
{
	Bytes out;
	size_t bitPos = 0;

	void put (uint32_t code, int bits)
	{
		for (int i = 0; i < bits; i++, bitPos++)
		{
			if (bitPos / 8 >= out.size())
				out.push_back(0);
			if (code & (1u << i))
				out[bitPos / 8] |= (uint8_t)(1u << (bitPos % 8));
		}
	}
};

// Test-only port of the asm LZCompress in mclib/lzcomp.cpp (#ifndef
// LINUX_BUILD): CLEAR first, a new code is assigned before its prefix is
// written, the width grows once the assigned code reaches MaxCode, and once
// all 12-bit codes are used a CLEAR is written at 12 bits and the table
// restarts. clears counts those mid-stream restarts.
Bytes encodeOriginal (const Bytes &src, int *clears = nullptr)
{
	BitWriter w;
	int nbits = 9;
	uint32_t maxCode = 512;
	uint32_t freeCode = kFirstFree;
	std::unordered_map<uint32_t, uint32_t> table;
	int restarts = 0;

	w.put(kClear, nbits);
	if (!src.empty())
	{
		uint32_t prefix = src[0];
		for (size_t i = 1; i < src.size(); i++)
		{
			const uint8_t k = src[i];
			const uint32_t key = (prefix << 8) | k;
			auto it = table.find(key);
			if (it != table.end())
			{
				prefix = it->second;
				continue;
			}
			const uint32_t assigned = freeCode;
			if (freeCode != 4096)
				table[key] = freeCode++;
			w.put(prefix, nbits);
			prefix = k;
			if (assigned >= maxCode)
			{
				if (nbits < 12)
				{
					nbits++;
					maxCode <<= 1;
				}
				else
				{
					w.put(kClear, nbits);
					nbits = 9;
					maxCode = 512;
					freeCode = kFirstFree;
					table.clear();
					restarts++;
				}
			}
		}
		w.put(prefix, nbits);
	}
	w.put(kEof, nbits);
	if (clears)
		*clears = restarts;
	return w.out;
}

Bytes toBytes (const char *s)
{
	return Bytes(s, s + strlen(s));
}

// Deterministic data with plenty of repeats and plenty of new strings, so
// the code width climbs to 12 bits and the table fills several times.
Bytes pseudoText (size_t n)
{
	Bytes b(n);
	uint32_t x = 12345;
	for (size_t i = 0; i < n; i++)
	{
		x = x * 1103515245u + 12345u;
		b[i] = (uint8_t)('a' + ((x >> 16) % 13));
	}
	return b;
}

size_t decode (Bytes &dest, const Bytes &src, size_t destLen)
{
	return LZDecomp(dest.data(), const_cast<uint8_t *>(src.data()), src.size(), destLen);
}

void roundTrip (const Bytes &plain)
{
	const Bytes packed = encodeOriginal(plain);
	Bytes out(plain.size() + 16, 0xA5);
	REQUIRE(decode(out, packed, plain.size()) == plain.size());
	CHECK(memcmp(out.data(), plain.data(), plain.size()) == 0);
}

// ---- hand-built FST archives ---------------------------------------------

struct Entry
{
	std::string name;
	Bytes stored;
	uint32_t realSize;
};

void writeFst (const std::string &path, uint32_t version, const std::vector<Entry> &entries)
{
	FILE *fp = fopen(path.c_str(), "wb");
	REQUIRE(fp != nullptr);
	const uint32_t count = (uint32_t)entries.size();
	fwrite(&version, 4, 1, fp);
	fwrite(&count, 4, 1, fp);
	uint32_t offset = 8 + count * (uint32_t)sizeof(FILEENTRY);
	for (const Entry &e : entries)
	{
		FILEENTRY fe;
		memset(&fe, 0, sizeof(fe));
		fe.offset = offset;
		fe.size = (DWORD)e.stored.size();
		fe.realSize = e.realSize;
		strncpy(fe.name, e.name.c_str(), MAX_FILENAME_SIZE - 1);
		fe.hash = elfHash(fe.name);
		fwrite(&fe, sizeof(fe), 1, fp);
		offset += (uint32_t)e.stored.size();
	}
	for (const Entry &e : entries)
		if (!e.stored.empty())
			fwrite(e.stored.data(), 1, e.stored.size(), fp);
	fclose(fp);
}

struct FstRegistry
{
	explicit FstRegistry (const char *fstPath)
	{
		maxFastFiles = 4;
		numFastFiles = 0;
		fastFiles = (FastFile **)malloc(sizeof(FastFile *) * maxFastFiles);
		for (long i = 0; i < maxFastFiles; ++i)
			fastFiles[i] = nullptr;
		ok = FastFileInit(fstPath);
	}
	~FstRegistry ()
	{
		FastFileFini();
	}
	bool ok = false;
};

// Open an archive entry through File::open (the game's path) and return it.
Bytes readEntry (const char *name, bool *opened)
{
	File f;
	*opened = (f.open(name) == NO_ERR);
	Bytes out;
	if (!*opened)
		return out;
	const long len = (long)f.getLength();
	out.resize(len > 0 ? (size_t)len : 0u);
	if (len > 0)
	{
		f.seek(0);
		f.read(out.data(), len);
	}
	f.close();
	return out;
}

} // namespace

TEST_SUITE("LZDecomp")
{

TEST_CASE("hand-built original LZ streams decode")
{
	SUBCASE("CLEAR A B 258 EOF is ABAB")
	{
		BitWriter w;
		w.put(kClear, 9);
		w.put('A', 9);
		w.put('B', 9);
		w.put(258, 9);     // entry 258 = "AB", added when B was read
		w.put(kEof, 9);
		Bytes out(16, 0);
		REQUIRE(decode(out, w.out, out.size()) == 4);
		CHECK(memcmp(out.data(), "ABAB", 4) == 0);
	}
	SUBCASE("a code not yet in the table (KwKwK) is the previous string plus its first byte")
	{
		BitWriter w;
		w.put(kClear, 9);
		w.put('A', 9);
		w.put(258, 9);     // 258 is the next free code: "A" + 'A'
		w.put(kEof, 9);
		Bytes out(16, 0);
		REQUIRE(decode(out, w.out, out.size()) == 3);
		CHECK(memcmp(out.data(), "AAA", 3) == 0);
	}
}

TEST_CASE("the test encoder writes what the hand-built stream says")
{
	// pins the encoder to the format before it is trusted for bigger inputs
	BitWriter w;
	w.put(kClear, 9);
	w.put('A', 9);
	w.put('B', 9);
	w.put(258, 9);
	w.put(kEof, 9);
	CHECK(encodeOriginal(toBytes("ABAB")) == w.out);
}

TEST_CASE("original LZ round trip: small inputs")
{
	roundTrip(toBytes("A"));
	roundTrip(toBytes("TOBEORNOTTOBEORTOBEORNOT"));
	roundTrip(Bytes(1000, 'z'));
	Bytes all(256);
	for (int i = 0; i < 256; i++)
		all[i] = (uint8_t)i;
	roundTrip(all);
}

TEST_CASE("original LZ round trip: multi-block (9 to 12 bit codes, mid-stream CLEAR)")
{
	const Bytes plain = pseudoText(300000);
	int clears = 0;
	const Bytes packed = encodeOriginal(plain, &clears);
	REQUIRE(clears >= 2);
	Bytes out(plain.size(), 0);
	REQUIRE(decode(out, packed, out.size()) == plain.size());
	CHECK(out == plain);

	// and a long run, where most codes are KwKwK
	roundTrip(Bytes(200000, 0));
}

TEST_CASE("original LZ: an empty stream (CLEAR EOF) decodes to nothing")
{
	Bytes out(4, 0xA5);
	CHECK(decode(out, encodeOriginal(Bytes()), out.size()) == 0);
	CHECK(out[0] == 0xA5);
}

TEST_CASE("original LZ: truncated input fails")
{
	const Bytes plain = pseudoText(20000);
	const Bytes packed = encodeOriginal(plain);
	Bytes out(plain.size(), 0);
	for (size_t cut : {packed.size() - 1, packed.size() / 2, (size_t)3, (size_t)1})
	{
		CAPTURE(cut);
		Bytes part(packed.begin(), packed.begin() + cut);
		CHECK(decode(out, part, out.size()) == 0);
	}
}

TEST_CASE("original LZ: output larger than the destination limit fails, nothing past it is written")
{
	const Bytes plain = pseudoText(20000);
	const Bytes packed = encodeOriginal(plain);
	const size_t limit = 12345;
	Bytes out(limit + 64, 0xA5);
	CHECK(decode(out, packed, limit) == 0);
	for (size_t i = limit; i < out.size(); i++)
		REQUIRE(out[i] == 0xA5);

	// an exact fit is fine
	Bytes exact(plain.size(), 0);
	CHECK(decode(exact, packed, plain.size()) == plain.size());
}

TEST_CASE("original LZ: corrupt codes fail")
{
	SUBCASE("a code past the next free one")
	{
		BitWriter w;
		w.put(kClear, 9);
		w.put('A', 9);
		w.put(300, 9);
		w.put(kEof, 9);
		Bytes out(64, 0);
		CHECK(decode(out, w.out, out.size()) == 0);
	}
	SUBCASE("a table code straight after CLEAR")
	{
		BitWriter w;
		w.put(kClear, 9);
		w.put(258, 9);
		w.put(kEof, 9);
		Bytes out(64, 0);
		CHECK(decode(out, w.out, out.size()) == 0);
	}
}

TEST_CASE("zlib data (what this port's LZCompress writes) still decodes")
{
	const Bytes plain = pseudoText(50000);
	Bytes packed(plain.size() * 2 + 64, 0);
	const size_t packedLen = LZCompress(packed.data(), const_cast<uint8_t *>(plain.data()), plain.size());
	REQUIRE(packedLen > 2);
	packed.resize(packedLen);
	CHECK((packed[0] & 0x0F) == 8);   // zlib CMF: deflate

	Bytes out(plain.size(), 0);
	CHECK(decode(out, packed, out.size()) == plain.size());
	CHECK(out == plain);
	Bytes tooShort(1000, 0);	// not `small`: a macro in the Windows headers
	CHECK(decode(tooShort, packed, tooShort.size()) == 0);
}

TEST_CASE("an LZ FST (0xFADDECAF) with original LZ, zlib and raw-stored entries reads through File::open")
{
	const Bytes lzPlain = pseudoText(70000);
	const Bytes zPlain = pseudoText(3000);
	const Bytes rawPlain = toBytes("stored as is: compression did not pay");
	Bytes zPacked(zPlain.size() * 2 + 64, 0);
	zPacked.resize(LZCompress(zPacked.data(), const_cast<uint8_t *>(zPlain.data()), zPlain.size()));

	const std::string fst = "lzt_scratch_orig.fst";
	writeFst(fst, FASTFILE_VERSION_LZ, {
		{"lzt_scratch/orig.bin", encodeOriginal(lzPlain), (uint32_t)lzPlain.size()},
		{"lzt_scratch/zlib.bin", zPacked, (uint32_t)zPlain.size()},
		{"lzt_scratch/raw.txt", rawPlain, (uint32_t)rawPlain.size()},
	});
	{
		FstRegistry reg(fst.c_str());
		REQUIRE(reg.ok);
		bool opened = false;
		CHECK(readEntry("lzt_scratch/orig.bin", &opened) == lzPlain);
		CHECK(opened);
		CHECK(readEntry("lzt_scratch/zlib.bin", &opened) == zPlain);
		CHECK(opened);
		CHECK(readEntry("lzt_scratch/raw.txt", &opened) == rawPlain);
		CHECK(opened);
	}
	remove(fst.c_str());
}

TEST_CASE("an uncompressed FST (0xCADDECAF, size == realSize) written by FastFile reads back")
{
	const std::string fst = "lzt_scratch_plain.fst";
	std::string payload = "plain FST entry, written raw by writeFast";
	{
		FastFile ff;
		REQUIRE(ff.create(fst.c_str(), /*compressed=*/false) == NO_ERR);
		REQUIRE(ff.reserve(1) == NO_ERR);
		REQUIRE(ff.writeFast("lzt_scratch/plain.txt", (void *)payload.data(), (int)payload.size()) == NO_ERR);
		ff.close();
	}
	{
		FstRegistry reg(fst.c_str());
		REQUIRE(reg.ok);
		bool opened = false;
		CHECK(readEntry("lzt_scratch/plain.txt", &opened) == Bytes(payload.begin(), payload.end()));
		CHECK(opened);
	}
	remove(fst.c_str());
}

} // TEST_SUITE("LZDecomp")
