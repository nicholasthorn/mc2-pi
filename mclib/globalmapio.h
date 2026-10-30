//---------------------------------------------------------------------------
// globalmapio.h: reading the global (area/door) pathing packets of a mission
// .pak in either the stock layout or MechCommander Omnitech's wide one.
//
// Omnitech (change "magic 07032012") widened the 16-bit fields of the
// on-disk pathing records to 32 bits, and maps saved by its editor use that
// layout. The two are told apart by packet size, as Omnitech itself does.
// Wide records are narrowed into the stock structs, failing if a value does
// not fit. Any other size is rejected before reading, so a foreign or
// corrupt map cannot make readPacket write past the buffer.
//
// Self-contained (no move.h) so the unit tests can use it; the narrow*
// functions are templates over the stock struct. Layouts are from the 6.28
// debug symbols (MCORel.pdb); see planning/future-features/MOD-COMPAT.md.
//---------------------------------------------------------------------------

#ifndef GLOBALMAPIO_H
#define GLOBALMAPIO_H

#include <cstddef>
#include <cstdint>
#include <climits>

#pragma pack(1)
struct WideGlobalMapArea {
	int32_t		sectorR;
	int32_t		sectorC;
	uint32_t	doors_Legacy32bitPtr;
	int32_t		type;
	int32_t		numDoors;
	int32_t		ownerWID;
	char		teamID;
	bool		offMap;
	bool		open;
	uint32_t	cellsCovered_Legacy32bitPtr;
};

struct WideDoorInfo {
	int32_t		doorIndex;
	char		doorSide;
};

struct WideDoorLink {
	int32_t		doorIndex;
	char		doorSide;
	int32_t		cost;
	int32_t		openCost;
};

struct WideGlobalMapDoor {
	int32_t		row;
	int32_t		col;
	char		length;
	bool		open;
	char		teamID;
	int32_t		area[2];
	int32_t		areaCost[2];
	char		direction[2];
	int32_t		numLinks[2];
	uint32_t	links_Legacy32bitPtr[2];
	int32_t		cost;
	int32_t		parent;
	int32_t		fromAreaIndex;
	uint32_t	flags;
	int32_t		g;
	int32_t		hPrime;
	int32_t		fPrime;
};
#pragma pack()

static_assert(sizeof(WideGlobalMapArea) == 31, "Omnitech _GlobalMapArea is 31 bytes");
static_assert(sizeof(WideDoorInfo) == 5, "Omnitech _DoorInfo is 5 bytes");
static_assert(sizeof(WideDoorLink) == 13, "Omnitech _DoorLink is 13 bytes");
static_assert(sizeof(WideGlobalMapDoor) == 73, "Omnitech _GlobalMapDoor is 73 bytes");

inline bool narrowToShort (int32_t v, short& out) {
	if (v < SHRT_MIN || v > SHRT_MAX)
		return false;
	out = (short)v;
	return true;
}

//---------------------------------------------------------------------------
// Reads packet `packet`, which must hold `count` records, into `out`.
// Packet is any type with seekPacket/getPacketSize/readPacket (PacketFile).
// Returns false, leaving `out` partly written at worst, if the size matches
// neither layout, the read fails or a wide value does not fit.
// Wide records from index `scratchFrom` on are zeroed instead of narrowed:
// door link lists end in NUM_EXTRA_DOOR_LINKS slots the pathfinder fills at
// run time, and Omnitech's editor saves whatever memory was there.
template <class Packet, class Stock, class Wide>
bool readPathRecords (Packet* packetFile, long packet, Stock* out, long count,
					  bool (*narrow)(Stock&, const Wide&), long scratchFrom = LONG_MAX) {
	if (count < 0 || packetFile->seekPacket(packet) != 0)
		return false;
	const size_t size = (size_t)packetFile->getPacketSize();
	if (size == sizeof(Stock) * (size_t)count)
		return count == 0 || packetFile->readPacket(packet, (unsigned char*)out) > 0;
	if (size != sizeof(Wide) * (size_t)count)
		return false;
	Wide* wide = new Wide[count];
	bool ok = packetFile->readPacket(packet, (unsigned char*)wide) > 0;
	for (long i = 0; ok && i < count; i++) {
		if (i < scratchFrom)
			ok = narrow(out[i], wide[i]);
		else
			out[i] = Stock();
	}
	delete[] wide;
	return ok;
}

//---------------------------------------------------------------------------
// Cell-to-area map: short per cell in stock, 32-bit in Omnitech.

inline bool narrowAreaMapCell (short& out, const int32_t& in) {
	return narrowToShort(in, out);
}

template <class Area>
bool narrowGlobalMapArea (Area& out, const WideGlobalMapArea& in) {
	out.doors_Legacy32bitPtr = in.doors_Legacy32bitPtr;
	out.type = (decltype(out.type))in.type;
	out.ownerWID = in.ownerWID;
	out.teamID = in.teamID;
	out.offMap = in.offMap;
	out.open = in.open;
	out.cellsCovered_Legacy32bitPtr = in.cellsCovered_Legacy32bitPtr;
	return narrowToShort(in.sectorR, out.sectorR) &&
		   narrowToShort(in.sectorC, out.sectorC) &&
		   narrowToShort(in.numDoors, out.numDoors) && out.numDoors >= 0;
}

template <class Info>
bool narrowDoorInfo (Info& out, const WideDoorInfo& in) {
	out.doorSide = in.doorSide;
	return narrowToShort(in.doorIndex, out.doorIndex);
}

template <class Link>
bool narrowDoorLink (Link& out, const WideDoorLink& in) {
	out.doorSide = in.doorSide;
	out.cost = in.cost;
	out.openCost = in.openCost;
	return narrowToShort(in.doorIndex, out.doorIndex);
}

template <class Door>
bool narrowGlobalMapDoor (Door& out, const WideGlobalMapDoor& in) {
	out.length = in.length;
	out.open = in.open;
	out.teamID = in.teamID;
	out.cost = in.cost;
	out.parent = in.parent;
	out.fromAreaIndex = in.fromAreaIndex;
	out.flags = in.flags;
	out.g = in.g;
	out.hPrime = in.hPrime;
	out.fPrime = in.fPrime;
	bool ok = narrowToShort(in.row, out.row) && narrowToShort(in.col, out.col);
	for (int s = 0; ok && s < 2; s++) {
		out.direction[s] = in.direction[s];
		out.links_Legacy32bitPtr[s] = in.links_Legacy32bitPtr[s];
		ok = narrowToShort(in.area[s], out.area[s]) &&
			 narrowToShort(in.areaCost[s], out.areaCost[s]) &&
			 narrowToShort(in.numLinks[s], out.numLinks[s]) && out.numLinks[s] >= 0;
	}
	return ok;
}

#endif
