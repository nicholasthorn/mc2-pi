// tests/unit/test_campaignglobals.cpp
//
// wolfman-MC2X's campaign variables (getcampaignglobalvar /
// setcampaignglobalvar): five integers a campaign's scripts keep across its
// missions, saved with the campaign. A save written with them all 0 has no
// block for them, so stock and Omnitech saves are unchanged.

#include "doctest.h"

#include "campaignglobals.h"
#include "inifile.h"

#include <cstdio>

namespace
{

const char* kPath = "campaignglobals_scratch.fit";

bool fileHasBlock(const char* block)
{
	FitIniFile file;
	if (file.open(kPath) != NO_ERR)
		return false;
	const bool found = (file.seekBlock(block) == NO_ERR);
	file.close();
	return found;
}

} // namespace

TEST_SUITE("CampaignGlobals")
{

TEST_CASE("get and set within 0 to 4; any other index reads 0 and is not stored")
{
	campaignglobals::reset();
	campaignglobals::set(0, 3);
	campaignglobals::set(4, -7);
	campaignglobals::set(5, 9);
	campaignglobals::set(-1, 9);
	CHECK(campaignglobals::get(0) == 3);
	CHECK(campaignglobals::get(1) == 0);
	CHECK(campaignglobals::get(4) == -7);
	CHECK(campaignglobals::get(5) == 0);
	CHECK(campaignglobals::get(-1) == 0);
	campaignglobals::reset();
	CHECK(campaignglobals::get(0) == 0);
}

TEST_CASE("save and load round trip; all 0 writes nothing")
{
	campaignglobals::reset();
	{
		FitIniFile out;
		REQUIRE(out.create(kPath) == NO_ERR);
		out.writeBlock("General");
		out.writeIdLong("Something", 1);
		campaignglobals::save(out);
		out.close();
	}
	CHECK_FALSE(fileHasBlock(campaignglobals::kBlockName));

	campaignglobals::set(1, 42);
	campaignglobals::set(3, -5);
	{
		FitIniFile out;
		REQUIRE(out.create(kPath) == NO_ERR);
		campaignglobals::save(out);
		out.close();
	}
	CHECK(fileHasBlock(campaignglobals::kBlockName));

	campaignglobals::set(1, 99);
	campaignglobals::set(0, 7);
	{
		FitIniFile in;
		REQUIRE(in.open(kPath) == NO_ERR);
		campaignglobals::load(in);
		in.close();
	}
	CHECK(campaignglobals::get(0) == 0);
	CHECK(campaignglobals::get(1) == 42);
	CHECK(campaignglobals::get(3) == -5);

	// a save without the block (stock, Omnitech, older) loads as all 0
	{
		FitIniFile out;
		REQUIRE(out.create(kPath) == NO_ERR);
		out.writeBlock("General");
		out.writeIdLong("Something", 1);
		out.close();
	}
	{
		FitIniFile in;
		REQUIRE(in.open(kPath) == NO_ERR);
		campaignglobals::load(in);
		in.close();
	}
	CHECK(campaignglobals::get(1) == 0);
	CHECK(campaignglobals::get(3) == 0);
	std::remove(kPath);
}

}
