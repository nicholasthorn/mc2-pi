// tests/unit/test_campaignstart.cpp
//
// New Campaign starts a campaign at its first group's first mission. A file
// with no mission listed there (the planned mercenary mode's merc.fit has
// MissionCount 0) made LogisticsData::startNewCampaign read an unset mission
// name and crash.

#include "doctest.h"

#include "campaignstart.h"
#include "inifile.h"

#include <cstdio>

namespace
{

const char* kPath = "campaignstart_scratch.fit";

bool canStart(const char* text)
{
	FILE* fp = fopen(kPath, "wb");
	REQUIRE(fp != nullptr);
	fputs(text, fp);
	fclose(fp);

	FitIniFile file;
	REQUIRE(file.open(kPath) == NO_ERR);
	const bool result = campaignstart::hasFirstMission(file);
	file.close();
	remove(kPath);
	return result;
}

} // namespace

TEST_SUITE("CampaignStart")
{

TEST_CASE("a first group that lists a mission can start")
{
	CHECK(canStart("FITini\r\n[Campaign]\r\nl GroupCount = 1\r\n"
		"[Group0]\r\nl MissionCount = 1\r\n"
		"[Group0Mission0]\r\nst FileName = \"mc2_01\"\r\n"));
}

TEST_CASE("a first group with no mission cannot")
{
	CHECK_FALSE(canStart("FITini\r\n[Campaign]\r\nl GroupCount = 1\r\n"
		"[Group0]\r\nl MissionCount = 0\r\n"));
}

TEST_CASE("a mission count with no mission block or file name cannot")
{
	CHECK_FALSE(canStart("FITini\r\n[Campaign]\r\nl GroupCount = 1\r\n"
		"[Group0]\r\nl MissionCount = 1\r\n"));
	CHECK_FALSE(canStart("FITini\r\n[Campaign]\r\nl GroupCount = 1\r\n"
		"[Group0]\r\nl MissionCount = 1\r\n[Group0Mission0]\r\nb Mandatory = TRUE\r\n"));
}

TEST_CASE("a later mission of the first group is enough")
{
	CHECK(canStart("FITini\r\n[Campaign]\r\nl GroupCount = 1\r\n"
		"[Group0]\r\nl MissionCount = 2\r\n"
		"[Group0Mission1]\r\nst FileName = \"mc2_02\"\r\n"));
}

TEST_CASE("no group, or no campaign block, cannot")
{
	CHECK_FALSE(canStart("FITini\r\n[Campaign]\r\nl GroupCount = 0\r\n"));
	CHECK_FALSE(canStart("FITini\r\n[Campaign]\r\nl GroupCount = 1\r\n"));
	CHECK_FALSE(canStart("FITini\r\n[Group0]\r\nl MissionCount = 1\r\n"
		"[Group0Mission0]\r\nst FileName = \"mc2_01\"\r\n"));
}

} // TEST_SUITE
