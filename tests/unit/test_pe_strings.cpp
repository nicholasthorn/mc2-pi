// tests/unit/test_pe_strings.cpp
//
// readPeStringTable(): the string table of a Windows resource DLL, read so
// MechCommander Omnitech's mc2res.dll can supply the names of its new units.
// The PE file is built here: one section holding a resource tree with one
// RT_STRING block.

#include "doctest.h"

#include "utils/pe_strings.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace
{

struct Bytes {
	std::vector<uint8_t> b;
	explicit Bytes(size_t n) : b(n, 0) {}
	void u16(size_t at, uint16_t v) { b[at] = (uint8_t)v; b[at + 1] = (uint8_t)(v >> 8); }
	void u32(size_t at, uint32_t v) { for (int i = 0; i < 4; ++i) b[at + i] = (uint8_t)(v >> (8 * i)); }
};

const size_t kRaw = 0x200;		// section file offset
const uint32_t kVa = 0x1000;	// section RVA

// Block `blockId` (ids (blockId - 1) * 16 ..) with UTF-16 strings at the
// given slots; the others empty.
std::vector<uint8_t> makePe(uint32_t blockId, const std::map<int, std::u16string>& slots)
{
	Bytes f(0x400);
	f.b[0] = 'M'; f.b[1] = 'Z';
	const size_t pe = 0x40;
	f.u32(0x3c, (uint32_t)pe);
	f.u32(pe, 0x00004550);
	f.u16(pe + 4, 0x14c);		// i386
	f.u16(pe + 6, 1);			// one section
	f.u16(pe + 20, 224);		// PE32 optional header
	const size_t opt = pe + 24;
	f.u16(opt, 0x10b);
	f.u32(opt + 92, 16);		// NumberOfRvaAndSizes
	f.u32(opt + 96 + 16, kVa);	// resource directory
	f.u32(opt + 96 + 20, 0x200);
	const size_t sec = opt + 224;
	f.u32(sec + 8, 0x200);		// VirtualSize
	f.u32(sec + 12, kVa);
	f.u32(sec + 16, 0x200);		// SizeOfRawData
	f.u32(sec + 20, (uint32_t)kRaw);

	const size_t r = kRaw;
	f.u16(r + 14, 1);			// root: one id entry
	f.u32(r + 16, 6);			// RT_STRING
	f.u32(r + 20, 0x80000000u | 0x18);
	f.u16(r + 0x18 + 14, 1);
	f.u32(r + 0x18 + 16, blockId);
	f.u32(r + 0x18 + 20, 0x80000000u | 0x30);
	f.u16(r + 0x30 + 14, 1);
	f.u32(r + 0x30 + 16, 0x409);	// en-US
	f.u32(r + 0x30 + 20, 0x48);		// data entry, not a directory

	size_t p = r + 0x58;
	for (int i = 0; i < 16; ++i) {
		std::map<int, std::u16string>::const_iterator it = slots.find(i);
		const std::u16string s = it == slots.end() ? std::u16string() : it->second;
		f.u16(p, (uint16_t)s.size());
		p += 2;
		for (char16_t c : s) {
			f.u16(p, (uint16_t)c);
			p += 2;
		}
	}
	f.u32(r + 0x48, kVa + 0x58);
	f.u32(r + 0x48 + 4, (uint32_t)(p - (r + 0x58)));
	return f.b;
}

} // namespace

TEST_SUITE("PeStrings")
{

TEST_CASE("reads an RT_STRING block: ids, empty slots, Windows-1252")
{
	std::map<int, std::u16string> slots;
	slots[0] = u"Uziel";
	slots[3] = u"Caf\u00e9 \u2019 \u2014 \u4e2d";	// e-acute, quote, dash, unmappable
	slots[15] = u"Last";
	const std::vector<uint8_t> pe = makePe(2, slots);

	std::map<uint32_t, std::string> out;
	REQUIRE(readPeStringTable(pe.data(), pe.size(), out));
	CHECK(out.size() == 3);
	CHECK(out[16] == "Uziel");
	CHECK(out[19] == "Caf\xe9 \x92 \x97 ?");
	CHECK(out[31] == "Last");
	CHECK(out.count(17) == 0);
}

TEST_CASE("truncated or foreign data yields fewer strings, never a bad read")
{
	std::map<int, std::u16string> slots;
	slots[0] = u"Uziel";
	slots[9] = u"Further";
	const std::vector<uint8_t> pe = makePe(1, slots);

	// every truncation point: the reader must stay inside the buffer (ASan)
	for (size_t n = 0; n < kRaw + 0x80; ++n) {
		std::vector<uint8_t> cut(pe.begin(), pe.begin() + n);
		std::map<uint32_t, std::string> out;
		readPeStringTable(cut.data(), cut.size(), out);
		CHECK(out.size() <= 2);
	}

	const uint8_t junk[] = { 'F', 'I', 'T', 'i', 'n', 'i' };
	std::map<uint32_t, std::string> out;
	CHECK_FALSE(readPeStringTable(junk, sizeof(junk), out));
	CHECK_FALSE(readPeStringTable(NULL, 0, out));
	CHECK(out.empty());
}

} // TEST_SUITE("PeStrings")
