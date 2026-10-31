// tests/unit/test_file_inifile.cpp
//
// Runs the REAL mclib file-stack TUs (file.cpp / inifile.cpp / packet.cpp)
// game-free, on top of the malloc-backed heap shim (heap_shim.cpp).
// Ported from mc2-opengl-remastered tests/unit (its FitIniFile in-memory
// open cases are dropped: that overload only exists in the remaster).
//
// What this proves:
//   * FitIniFile disk parse -- the game's config spine, offline-verifiable.
//   * PacketFile write/read round-trip -- the .pak container used by object2.pak
//     et al., exercised end-to-end with no external data dependency.
//   * File loose-disk resolution (base-loose branch of File::open).

#include "doctest.h"

#include "file.h"
#include "inifile.h"
#include "packet.h"

#include <cstdio>
#include <cstring>
#include <string>

#ifndef MC2_TEST_FIXTURE_DIR
#error "MC2_TEST_FIXTURE_DIR must be defined by tests/unit/CMakeLists.txt"
#endif

namespace
{

std::string fixturePath (const char *name)
{
	std::string p(MC2_TEST_FIXTURE_DIR);
	p += "/";
	p += name;
	return p;
}

// Scratch-file path inside the build dir (cwd for `check`) or wherever the
// test binary is launched from. Lowercase on purpose: File::open lowercases
// paths (S_strlwr, file.cpp) before hitting the OS.
std::string scratchPath (const char *name)
{
	std::string p("mvh_scratch_");
	p += name;
	return p;
}

} // namespace

TEST_SUITE("FileStack")
{

TEST_CASE("FitIniFile parses a .fit fixture from disk")
{
	FitIniFile fit;
	const std::string path = fixturePath("harness_smoke.fit");
	REQUIRE(fit.open(path.c_str()) == NO_ERR);

	SUBCASE("block seek + typed reads")
	{
		REQUIRE(fit.seekBlock("TestBlock") == NO_ERR);

		long l = 0;
		CHECK(fit.readIdLong("TestLong", l) == NO_ERR);
		CHECK(l == 42);

		float f = 0.0f;
		CHECK(fit.readIdFloat("TestFloat", f) == NO_ERR);
		CHECK(f == doctest::Approx(3.5f));

		char s[64] = {0};
		CHECK(fit.readIdString("TestString", s, sizeof(s) - 1) == NO_ERR);
		CHECK(strcmp(s, "hello harness") == 0);
	}

	SUBCASE("second block is independently addressable")
	{
		REQUIRE(fit.seekBlock("SecondBlock") == NO_ERR);
		long v = 0;
		CHECK(fit.readIdLong("Another", v) == NO_ERR);
		CHECK(v == 7);
	}

	SUBCASE("missing block / missing var fail cleanly")
	{
		CHECK((unsigned long)fit.seekBlock("NoSuchBlock") == BLOCK_NOT_FOUND);
		REQUIRE(fit.seekBlock("TestBlock") == NO_ERR);
		long v = 0;
		CHECK(fit.readIdLong("NoSuchVar", v) != NO_ERR);
	}

	fit.close();
	CHECK(!fit.isOpen());
}

TEST_CASE("PacketFile write/read round-trip")
{
	const std::string path = scratchPath("roundtrip.pak");
	const char p0[] = "alpha";
	const char p1[] = "bravo-longer-payload";
	const char p2[] = "c";

	// --- write ---
	{
		PacketFile out;
		REQUIRE(out.create(path.c_str()) == NO_ERR);
		out.reserve(3);
		// writePacket returns bytes written, not NO_ERR.
		CHECK(out.writePacket(0, (MemoryPtr)p0, sizeof(p0)) == (int)sizeof(p0));
		CHECK(out.writePacket(1, (MemoryPtr)p1, sizeof(p1)) == (int)sizeof(p1));
		CHECK(out.writePacket(2, (MemoryPtr)p2, sizeof(p2)) == (int)sizeof(p2));
		out.close();
	}

	// --- read back ---
	{
		PacketFile in;
		REQUIRE(in.open(path.c_str()) == NO_ERR);
		CHECK(in.getNumPackets() == 3);

		REQUIRE(in.seekPacket(1) == NO_ERR);
		CHECK(in.getPacketSize() == (int)sizeof(p1));

		char buf[64] = {0};
		REQUIRE((size_t)in.getPacketSize() <= sizeof(buf));
		in.readPacket(1, (unsigned char *)buf);
		CHECK(strcmp(buf, p1) == 0);

		REQUIRE(in.seekPacket(0) == NO_ERR);
		CHECK(in.getPacketSize() == (int)sizeof(p0));
		memset(buf, 0, sizeof(buf));
		in.readPacket(0, (unsigned char *)buf);
		CHECK(strcmp(buf, p0) == 0);

		in.close();
	}

	remove(path.c_str());
}

TEST_CASE("File resolves and reads a loose disk file (base-loose branch)")
{
	File f;
	const std::string path = fixturePath("harness_smoke.fit");
	REQUIRE(f.open(path.c_str()) == NO_ERR);
	CHECK(f.getLength() > 0);

	char header[8] = {0};
	CHECK(f.read((MemoryPtr)header, 6) == 6);
	CHECK(strncmp(header, "FITini", 6) == 0);

	f.close();
	CHECK(!f.isOpen());
}

TEST_CASE("FitIniFile reads a FIT through a memory-buffer parent")
{
	// How ObjectTypeManager hands a built Omnitech mech type FIT to
	// ObjectType::init(FilePtr, size): a memory File as the parent.
	static const char fit[] =
		"FITini\r\n\r\n"
		"[ObjectClass]\r\nl ObjectTypeNum = 2\r\n\r\n"
		"[MechProfile]\r\nst ProfileName = \"uziel\"\r\nf explosionradius = 80\r\n\r\n"
		"FITend\r\n";
	char buffer[sizeof(fit)];
	memcpy(buffer, fit, sizeof(fit));
	const int len = (int)strlen(buffer);

	File mem;
	REQUIRE(mem.open(buffer, len) == NO_ERR);
	CHECK((mem.getMemoryView() == (MemoryPtr)buffer));

	{
		FitIniFile ini;
		REQUIRE(ini.open(&mem, len) == NO_ERR);
		REQUIRE(ini.seekBlock("ObjectClass") == NO_ERR);
		long typeNum = 0;
		CHECK(ini.readIdLong("ObjectTypeNum", typeNum) == NO_ERR);
		CHECK(typeNum == 2);
		REQUIRE(ini.seekBlock("MechProfile") == NO_ERR);
		char name[32] = {0};
		CHECK(ini.readIdString("ProfileName", name, 31) == NO_ERR);
		CHECK(strcmp(name, "uziel") == 0);
		float radius = 0.0f;
		CHECK(ini.readIdFloat("explosionradius", radius) == NO_ERR);
		CHECK(radius == doctest::Approx(80.0f));
		ini.close();
	}

	// the view is not freed: the caller's buffer is intact
	CHECK(memcmp(buffer, fit, sizeof(fit)) == 0);
	mem.close();
}

TEST_CASE("File::getMemoryView is NULL for a disk file")
{
	File f;
	REQUIRE(f.open(fixturePath("harness_smoke.fit").c_str()) == NO_ERR);
	CHECK((f.getMemoryView() == NULL));
	f.close();
}

} // TEST_SUITE("FileStack")
