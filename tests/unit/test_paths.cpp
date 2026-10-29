// tests/unit/test_paths.cpp
//
// mclib/paths.cpp path globals, and File::open's handling of the Windows-style
// paths that the game data stores (backslashes, mixed case). The globals test
// is ported from mc2-opengl-remastered's test_paths_mod.cpp; its mod-loading
// cases are dropped (the mods/ feature only exists in the remaster).

#include "doctest.h"

#include "file.h"
#include "paths.h"

#include <cstdio>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <direct.h>
#define mvh_mkdir(p) _mkdir(p)
#define mvh_rmdir(p) _rmdir(p)
#else
#include <sys/stat.h>
#include <unistd.h>
#define mvh_mkdir(p) mkdir(p, 0755)
#define mvh_rmdir(p) rmdir(p)
#endif

namespace
{

// Created in the test working directory (the build tree) and removed again.
// Lowercase on disk on purpose: File::open lowercases every name it is given.
const char *kDir     = "mvh_scratch_paths";
const char *kFile    = "mvh_scratch_paths/entry.txt";
const char *kPayload = "path-conversion";

void writeScratch ()
{
	mvh_mkdir(kDir);
	FILE *fp = fopen(kFile, "wb");
	REQUIRE(fp != nullptr);
	fwrite(kPayload, 1, strlen(kPayload), fp);
	fclose(fp);
}

void removeScratch ()
{
	remove(kFile);
	mvh_rmdir(kDir);
}

std::string readAll (File &f)
{
	const long len = (long)f.getLength();
	std::string out(len > 0 ? (size_t)len : 0u, '\0');
	if (len > 0) { f.seek(0); f.read((MemoryPtr)&out[0], len); }
	return out;
}

} // namespace

TEST_SUITE("Paths")
{

TEST_CASE("paths.cpp exposes the engine's real path string globals")
{
	// PATH_SEPARATOR is '/' on every platform: LINUX_BUILD is always defined.
	CHECK(strncmp(objectPath, "data/objects/", 13) == 0);
	CHECK(strncmp(artPath,    "data/art/",     9) == 0);
	CHECK(strncmp(missionPath,"data/missions/",14) == 0);
}

TEST_CASE("File::open resolves Windows-style names from data files")
{
	writeScratch();

	SUBCASE("backslash separators are converted")
	{
		File f;
		REQUIRE(f.open("mvh_scratch_paths\\entry.txt") == NO_ERR);
		CHECK(readAll(f) == kPayload);
		f.close();
	}

	SUBCASE("mixed case is folded to the lowercase name on disk")
	{
		File f;
		REQUIRE(f.open("MVH_Scratch_Paths\\Entry.TXT") == NO_ERR);
		CHECK(readAll(f) == kPayload);
		f.close();
	}

	SUBCASE("doNotLower keeps the case as given")
	{
		// Only meaningful on a case-sensitive filesystem.
#ifndef _WIN32
		File f;
		CHECK(f.open("MVH_Scratch_Paths/Entry.TXT", READ, 50, true) != NO_ERR);
#endif
	}

	removeScratch();
}

} // TEST_SUITE("Paths")
