//---------------------------------------------------------------------------
// globalmapio.h: reading the global (area/door) pathing packets of a mission
// .pak, and its special-area footprints, in either the stock layout or
// MechCommander Omnitech's wide one.
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

// Special-area (gate/wall) footprints, the packet after the three global maps
struct WideGameObjectFootPrint {
	int32_t		cellPositionRow;
	int32_t		cellPositionCol;
	int32_t		preNumCells;
	int32_t		numCells;
	int32_t		cells[64][2];
};
#pragma pack()

static_assert(sizeof(WideGlobalMapArea) == 31, "Omnitech _GlobalMapArea is 31 bytes");
static_assert(sizeof(WideDoorInfo) == 5, "Omnitech _DoorInfo is 5 bytes");
static_assert(sizeof(WideDoorLink) == 13, "Omnitech _DoorLink is 13 bytes");
static_assert(sizeof(WideGlobalMapDoor) == 73, "Omnitech _GlobalMapDoor is 73 bytes");
static_assert(sizeof(WideGameObjectFootPrint) == 528, "Omnitech _GameObjectFootPrint is 528 bytes");

// The structs are packed, so their fields are unaligned: check values and
// assign them, never bind a reference to a field (undefined behaviour).
inline bool fitsShort (int32_t v) {
	return (v >= SHRT_MIN) && (v <= SHRT_MAX);
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
	if (!fitsShort(in))
		return false;
	out = (short)in;
	return true;
}

template <class Area>
bool narrowGlobalMapArea (Area& out, const WideGlobalMapArea& in) {
	if (!fitsShort(in.sectorR) || !fitsShort(in.sectorC) || !fitsShort(in.numDoors) || (in.numDoors < 0))
		return false;
	out.sectorR = (short)in.sectorR;
	out.sectorC = (short)in.sectorC;
	out.numDoors = (short)in.numDoors;
	out.doors_Legacy32bitPtr = in.doors_Legacy32bitPtr;
	out.type = (decltype(out.type))in.type;
	out.ownerWID = in.ownerWID;
	out.teamID = in.teamID;
	out.offMap = in.offMap;
	out.open = in.open;
	out.cellsCovered_Legacy32bitPtr = in.cellsCovered_Legacy32bitPtr;
	return true;
}

template <class Info>
bool narrowDoorInfo (Info& out, const WideDoorInfo& in) {
	if (!fitsShort(in.doorIndex))
		return false;
	out.doorIndex = (short)in.doorIndex;
	out.doorSide = in.doorSide;
	return true;
}

template <class Link>
bool narrowDoorLink (Link& out, const WideDoorLink& in) {
	if (!fitsShort(in.doorIndex))
		return false;
	out.doorIndex = (short)in.doorIndex;
	out.doorSide = in.doorSide;
	out.cost = in.cost;
	out.openCost = in.openCost;
	return true;
}

template <class Door>
bool narrowGlobalMapDoor (Door& out, const WideGlobalMapDoor& in) {
	if (!fitsShort(in.row) || !fitsShort(in.col))
		return false;
	for (int s = 0; s < 2; s++)
		if (!fitsShort(in.area[s]) || !fitsShort(in.areaCost[s]) || !fitsShort(in.numLinks[s]) || (in.numLinks[s] < 0))
			return false;
	out.row = (short)in.row;
	out.col = (short)in.col;
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
	for (int s = 0; s < 2; s++) {
		out.area[s] = (short)in.area[s];
		out.areaCost[s] = (short)in.areaCost[s];
		out.direction[s] = in.direction[s];
		out.numLinks[s] = (short)in.numLinks[s];
		out.links_Legacy32bitPtr[s] = in.links_Legacy32bitPtr[s];
	}
	return true;
}

// Cells past numCells are zeroed, like the stock in-memory footprints.
template <class FootPrint>
bool narrowGameObjectFootPrint (FootPrint& out, const WideGameObjectFootPrint& in) {
	const int32_t maxCells = (int32_t)(sizeof(out.cells) / sizeof(out.cells[0]));
	if (!fitsShort(in.cellPositionRow) || !fitsShort(in.cellPositionCol) || !fitsShort(in.preNumCells) ||
		(in.numCells < 0) || (in.numCells > maxCells))
		return false;
	for (int32_t i = 0; i < in.numCells; i++)
		if (!fitsShort(in.cells[i][0]) || !fitsShort(in.cells[i][1]))
			return false;
	out = FootPrint();
	out.cellPositionRow = (short)in.cellPositionRow;
	out.cellPositionCol = (short)in.cellPositionCol;
	out.preNumCells = (short)in.preNumCells;
	out.numCells = (short)in.numCells;
	for (int32_t i = 0; i < in.numCells; i++) {
		out.cells[i][0] = (short)in.cells[i][0];
		out.cells[i][1] = (short)in.cells[i][1];
	}
	return true;
}

#endif
