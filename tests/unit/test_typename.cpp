// tests/unit/test_typename.cpp
//
// typeNameId(): the name id a building, turret or gate type shows. MechCommander
// Omnitech gives 156 of its own types BuildingName = 0 and only an
// [ObjectType] Name, which then becomes the text of a runtime string id.

#include "doctest.h"

#include "file.h"
#include "inifile.h"
#include "typename.h"
#include "utils/pe_strings.h"

#include <cstring>
#include <string>

namespace
{

// The [ObjectType] block of a type FIT, as Omnitech's loose files have it.
std::string typeFit(const char* name, const char* appearance)
{
	std::string fit = "FITini\r\n\r\n[ObjectClass]\r\nl ObjectTypeNum=1\r\n\r\n[ObjectType]\r\n";
	if (name)
		fit += std::string("st Name=\"") + name + "\"\r\n";
	if (appearance)
		fit += std::string("st AppearanceName=\"") + appearance + "\"\r\n";
	fit += "l ExplosionObject=42\r\n\r\n[BuildingData]\r\nul DmgLevel=100\r\nl BuildingName=0\r\n\r\nFITend\r\n";
	return fit;
}

// typeNameId on a FIT held in memory, read from its [BuildingData] block as
// BuildingType::init does.
long nameIdFor(const std::string& fitText, long nameId)
{
	std::string buffer = fitText;
	File mem;
	REQUIRE(mem.open(&buffer[0], (int)buffer.size()) == NO_ERR);
	FitIniFile fit;
	REQUIRE(fit.open(&mem, (unsigned long)buffer.size()) == NO_ERR);
	REQUIRE(fit.seekBlock("BuildingData") == NO_ERR);
	return typeNameId(fit, nameId);
}

std::string textOf(long id)
{
	const char* text = findRuntimeString((uint32_t)id);
	return text ? std::string(text) : std::string("(none)");
}

} // namespace

TEST_SUITE("TypeName") {

TEST_CASE("typeNameId: name id 0 takes the type's Name")
{
	const long id = nameIdFor(typeFit("Petrol Station", "PetrolStation"), 0);
	CHECK(id >= (long)kRuntimeStringBase);
	CHECK(textOf(id) == "Petrol Station");

	// the same type loaded again (another mission) keeps its id
	CHECK(nameIdFor(typeFit("Petrol Station", "PetrolStation"), 0) == id);
}

TEST_CASE("typeNameId: two types with the same Name keep apart ids")
{
	// Omnitech's bridge_side1 and bridge_side1a are both "Bridge Support"
	const long first = nameIdFor(typeFit("Bridge Support", "bridge_side1"), 0);
	const long second = nameIdFor(typeFit("Bridge Support", "bridge_side1a"), 0);
	CHECK(first != second);
	CHECK(textOf(first) == "Bridge Support");
	CHECK(textOf(second) == "Bridge Support");
}

TEST_CASE("typeNameId: a Name that is a file name loses its .fit")
{
	// Omnitech's d_mediumwall in object2.pak
	const long id = nameIdFor(typeFit("DavionMediumWall.fit", "d_mediumwall"), 0);
	CHECK(textOf(id) == "DavionMediumWall");
}

TEST_CASE("typeNameId: other ids, and 0 with no Name, come back as they are")
{
	CHECK(nameIdFor(typeFit("Petrol Station", "PetrolStation"), 254) == 254);
	CHECK(nameIdFor(typeFit("Petrol Station", "PetrolStation"), 34241) == 34241);
	CHECK(nameIdFor(typeFit("Petrol Station", "PetrolStation"), -1) == -1);
	CHECK(nameIdFor(typeFit(NULL, "NoName"), 0) == 0);
	CHECK(nameIdFor(typeFit("", "EmptyName"), 0) == 0);
}

TEST_CASE("displayTypeName: drops a trailing .fit only")
{
	char a[] = "DavionMediumWall.fit";
	displayTypeName(a);
	CHECK(std::string(a) == "DavionMediumWall");
	char b[] = "Wall.FIT";
	displayTypeName(b);
	CHECK(std::string(b) == "Wall");
	char c[] = "Petrol Station";
	displayTypeName(c);
	CHECK(std::string(c) == "Petrol Station");
	char d[] = ".fit";
	displayTypeName(d);
	CHECK(std::string(d) == ".fit");
	char e[] = "Outfitter";
	displayTypeName(e);
	CHECK(std::string(e) == "Outfitter");
}

} // TEST_SUITE("TypeName")
