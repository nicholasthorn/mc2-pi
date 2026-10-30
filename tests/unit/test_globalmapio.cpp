// tests/unit/test_globalmapio.cpp
//
// globalmapio.h: the global pathing records of a mission .pak are read in
// the stock layout or Omnitech's wide one, and any other packet size is
// rejected before the read (it used to overflow the buffer and corrupt the
// heap). Packets go through a real PacketFile round-trip.
//
// The Stock* structs mirror the on-disk layouts in mclib/move.h; move.cpp
// static_asserts the same sizes on the real ones.

#include "doctest.h"

#include "packet.h"
#include "globalmapio.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace
{

#pragma pack(1)
struct StockArea {
	short		sectorR;
	short		sectorC;
	uint32_t	doors_Legacy32bitPtr;
	int			type;
	short		numDoors;
	int			ownerWID;
	char		teamID;
	bool		offMap;
	bool		open;
	uint32_t	cellsCovered_Legacy32bitPtr;
};

struct StockDoorInfo {
	short		doorIndex;
	char		doorSide;
};

struct StockDoorLink {
	short		doorIndex;
	char		doorSide;
	int			cost;
	int			openCost;
};

struct StockDoor {
	short		row;
	short		col;
	char		length;
	bool		open;
	char		teamID;
	short		area[2];
	short		areaCost[2];
	char		direction[2];
	short		numLinks[2];
	uint32_t	links_Legacy32bitPtr[2];
	int			cost;
	int			parent;
	int			fromAreaIndex;
	unsigned int flags;
	int			g;
	int			hPrime;
	int			fPrime;
};
#pragma pack()

static_assert(sizeof(StockArea) == 25, "mirrors move.h GlobalMapArea");
static_assert(sizeof(StockDoorInfo) == 3, "mirrors move.h DoorInfo");
static_assert(sizeof(StockDoorLink) == 11, "mirrors move.h DoorLink");
static_assert(sizeof(StockDoor) == 57, "mirrors move.h GlobalMapDoor");

// Writes one packet holding `bytes` and reopens the file for reading.
struct OnePacket {
	std::string path;
	PacketFile in;

	OnePacket (const char* name, const void* bytes, size_t n) : path(std::string("gmio_scratch_") + name) {
		PacketFile out;
		REQUIRE(out.create(path.c_str()) == NO_ERR);
		out.reserve(1);
		// returns bytes stored, which is less when the packet is compressed
		REQUIRE(out.writePacket(0, (MemoryPtr)bytes, (unsigned int)n) > 0);
		out.close();
		REQUIRE(in.open(path.c_str()) == NO_ERR);
	}
	~OnePacket () {
		in.close();
		std::remove(path.c_str());
	}
};

} // namespace

TEST_CASE("globalmapio: stock-layout areas are read unchanged")
{
	StockArea src[2] = {};
	src[0].sectorR = 3; src[0].sectorC = 4; src[0].numDoors = 2; src[0].ownerWID = 77;
	src[1].sectorR = 5; src[1].offMap = true; src[1].type = 4;
	OnePacket f("stock_areas.pak", src, sizeof(src));

	StockArea dst[2] = {};
	CHECK(readPathRecords(&f.in, 0, dst, 2, narrowGlobalMapArea<StockArea>));
	CHECK(std::memcmp(src, dst, sizeof(src)) == 0);
}

TEST_CASE("globalmapio: Omnitech wide areas, doors and links are narrowed")
{
	WideGlobalMapArea area = {};
	area.sectorR = 12; area.sectorC = 34; area.type = 2; area.numDoors = 3;
	area.ownerWID = 500; area.teamID = 1; area.open = true;
	{
		OnePacket f("wide_area.pak", &area, sizeof(area));
		StockArea out = {};
		REQUIRE(readPathRecords(&f.in, 0, &out, 1, narrowGlobalMapArea<StockArea>));
		CHECK(+(out.sectorR) == 12);
		CHECK(+(out.sectorC) == 34);
		CHECK(+(out.type) == 2);
		CHECK(+(out.numDoors) == 3);
		CHECK(+(out.ownerWID) == 500);
		CHECK(+(out.teamID) == 1);
		CHECK(bool(out.open));
		CHECK_FALSE(bool(out.offMap));
	}

	WideGlobalMapDoor door = {};
	door.row = 100; door.col = 200; door.length = 4; door.teamID = -1;
	door.area[0] = 7; door.area[1] = 9; door.areaCost[1] = 3;
	door.direction[0] = 2; door.numLinks[0] = 5; door.numLinks[1] = 6;
	door.cost = 1000; door.fPrime = -4;
	{
		OnePacket f("wide_door.pak", &door, sizeof(door));
		StockDoor out = {};
		REQUIRE(readPathRecords(&f.in, 0, &out, 1, narrowGlobalMapDoor<StockDoor>));
		CHECK(+(out.row) == 100);
		CHECK(+(out.col) == 200);
		CHECK(+(out.length) == 4);
		CHECK(+(out.teamID) == -1);
		CHECK(+(out.area[0]) == 7);
		CHECK(+(out.area[1]) == 9);
		CHECK(+(out.areaCost[1]) == 3);
		CHECK(+(out.direction[0]) == 2);
		CHECK(+(out.numLinks[0]) == 5);
		CHECK(+(out.numLinks[1]) == 6);
		CHECK(+(out.cost) == 1000);
		CHECK(+(out.fPrime) == -4);
	}

	WideDoorLink links[2] = {};
	links[0].doorIndex = 1; links[0].doorSide = 1; links[0].cost = 10; links[0].openCost = 20;
	links[1].doorIndex = 4000;
	{
		OnePacket f("wide_links.pak", links, sizeof(links));
		StockDoorLink out[2] = {};
		REQUIRE(readPathRecords(&f.in, 0, out, 2, narrowDoorLink<StockDoorLink>));
		CHECK(+(out[0].doorIndex) == 1);
		CHECK(+(out[0].doorSide) == 1);
		CHECK(+(out[0].cost) == 10);
		CHECK(+(out[0].openCost) == 20);
		CHECK(+(out[1].doorIndex) == 4000);
	}

	WideDoorInfo info = {};
	info.doorIndex = 321; info.doorSide = 1;
	{
		OnePacket f("wide_info.pak", &info, sizeof(info));
		StockDoorInfo out = {};
		REQUIRE(readPathRecords(&f.in, 0, &out, 1, narrowDoorInfo<StockDoorInfo>));
		CHECK(+(out.doorIndex) == 321);
		CHECK(+(out.doorSide) == 1);
	}

	const int32_t cells[4] = {0, 1, -1, 30000};
	{
		OnePacket f("wide_cells.pak", cells, sizeof(cells));
		short out[4] = {};
		REQUIRE(readPathRecords(&f.in, 0, out, 4, narrowAreaMapCell));
		CHECK(out[0] == 0);
		CHECK(out[2] == -1);
		CHECK(out[3] == 30000);
	}
}

TEST_CASE("globalmapio: a packet of any other size is rejected before reading")
{
	// 3 stock door infos are 9 bytes, 3 wide ones 15: 12 is neither
	const unsigned char junk[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
	OnePacket f("junk.pak", junk, sizeof(junk));

	// one record more than the caller expects would also have overflowed
	StockDoorInfo out[3 + 1];
	std::memset(out, 0x5A, sizeof(out));
	CHECK_FALSE(readPathRecords(&f.in, 0, out, 3, narrowDoorInfo<StockDoorInfo>));
	const unsigned char* raw = (const unsigned char*)out;
	for (size_t i = 0; i < sizeof(out); i++)
		REQUIRE(raw[i] == 0x5A);
}

TEST_CASE("globalmapio: wide values that do not fit the stock fields are rejected")
{
	WideGlobalMapArea area = {};
	area.numDoors = 40000;
	OnePacket f("wide_toobig.pak", &area, sizeof(area));
	StockArea out = {};
	CHECK_FALSE(readPathRecords(&f.in, 0, &out, 1, narrowGlobalMapArea<StockArea>));

	WideDoorInfo info = {};
	info.doorIndex = -40000;
	StockDoorInfo infoOut = {};
	CHECK_FALSE(narrowDoorInfo(infoOut, info));
}

TEST_CASE("globalmapio: trailing door-link scratch slots are zeroed, not checked")
{
	// A side with no links: just the NUM_EXTRA_DOOR_LINKS scratch slots,
	// saved by Omnitech's editor with leftover memory (Clan Wars mission 1)
	WideDoorLink links[3] = {};
	links[0].doorIndex = 12; links[0].cost = 5;
	links[1].doorIndex = 725537101; links[1].doorSide = -62; links[1].cost = 49109;
	links[2].doorIndex = -1;
	OnePacket f("wide_scratch.pak", links, sizeof(links));

	StockDoorLink out[3];
	std::memset(out, 0x5A, sizeof(out));
	REQUIRE(readPathRecords(&f.in, 0, out, 3, narrowDoorLink<StockDoorLink>, 1));
	CHECK(+(out[0].doorIndex) == 12);
	CHECK(+(out[0].cost) == 5);
	CHECK(+(out[1].doorIndex) == 0);
	CHECK(+(out[1].cost) == 0);
	CHECK(+(out[2].doorIndex) == 0);

	// without the scratch hint the garbage index is rejected
	CHECK_FALSE(readPathRecords(&f.in, 0, out, 3, narrowDoorLink<StockDoorLink>));
}
