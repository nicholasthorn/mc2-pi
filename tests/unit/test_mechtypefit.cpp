// tests/unit/test_mechtypefit.cpp
//
// The type FIT built for a MechCommander Omnitech mech that has only its CSV.
// Omnitech marks its aircraft (copters, VTOLs, dropships, fighters) in the
// CSV's first row, "BaseChassisInfo,2,1"; the FIT must then say MoveType 1,
// or the game runs them as ground units.

#include "doctest.h"

#include "csvfile.h"
#include "file.h"
#include "inifile.h"
#include "mechtypefit.h"

#include <cstdio>
#include <cstring>

namespace
{

long moveTypeOf(const char* firstRow)
{
	const char* path = "mechtypefit_scratch.csv";
	FILE* fp = fopen(path, "wb");
	REQUIRE(fp != nullptr);
	// a trailing column and row: the last of each cannot be read
	fprintf(fp, "%s\r\nMechName,Heavy Copter,,,x\r\nTonnage,50,,,x\r\n// end\r\n", firstRow);
	fclose(fp);
	CSVFile csv;
	REQUIRE(csv.open(path) == NO_ERR);
	const long moveType = chassisMoveType(csv);
	csv.close();
	remove(path);
	return moveType;
}

} // namespace

TEST_SUITE("MechTypeFit") {

TEST_CASE("chassisMoveType: Omnitech's aircraft CSVs say air, the rest ground")
{
	CHECK(moveTypeOf("BaseChassisInfo,2,1,,x") == 1);		// heavycopter.csv
	CHECK(moveTypeOf("BaseChassisInfo,,,,x") == 0);			// atlas.csv
	CHECK(moveTypeOf("BaseChassisInfo,2,0,,x") == 0);
	CHECK(moveTypeOf("BaseChassisInfo,2,7,,x") == 0);		// no such move type
}

TEST_CASE("buildMechTypeFit: the FIT reads back with the move type and damage by weight")
{
	char fit[1024];
	buildMechTypeFit(fit, sizeof(fit), 2, "mediumcopter", 35, 1);

	File mem;
	REQUIRE(mem.open(fit, (int)strlen(fit)) == NO_ERR);
	FitIniFile ini;
	REQUIRE(ini.open(&mem, (unsigned long)strlen(fit)) == NO_ERR);

	REQUIRE(ini.seekBlock("ObjectClass") == NO_ERR);
	long typeNum = 0;
	CHECK(ini.readIdLong("ObjectTypeNum", typeNum) == NO_ERR);
	CHECK(typeNum == 2);

	REQUIRE(ini.seekBlock("MechProfile") == NO_ERR);
	char profile[64] = {0};
	CHECK(ini.readIdString("ProfileName", profile, 63) == NO_ERR);
	CHECK(strcmp(profile, "mediumcopter") == 0);
	long moveType = -1;
	CHECK(ini.readIdLong("MoveType", moveType) == NO_ERR);
	CHECK(moveType == 1);
	float destruct = 0.0f;
	CHECK(ini.readIdFloat("destructdamage", destruct) == NO_ERR);
	CHECK(destruct == doctest::Approx(35.0f));		// under 40 tons

	char ground[1024];
	buildMechTypeFit(ground, sizeof(ground), 2, "uziel", 85, 0);
	CHECK(strstr(ground, "l MoveType = 0\r\n") != nullptr);
	CHECK(strstr(ground, "f destructdamage = 65\r\n") != nullptr);		// 80 tons and up
}

} // TEST_SUITE("MechTypeFit")
