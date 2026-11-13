// code/mechtypefit.h
//
// The type FIT for a MechCommander Omnitech mech type past object2.pak
// (movobj.fit numbers from 1200), which has only its CSV. Used by
// ObjectTypeManager (objtype.cpp); kept apart, with no game objects, so it
// can be unit tested.

#ifndef MECHTYPEFIT_H
#define MECHTYPEFIT_H

#include <cstddef>

class CSVFile;

// The move type a mech CSV gives: Omnitech's aircraft (copters, VTOLs,
// dropships, aerospace fighters) start "BaseChassisInfo,2,1", object class 2
// and MoveType 1 (air); every other CSV leaves those cells empty. 1 for air,
// 0 (ground) otherwise.
long chassisMoveType(CSVFile& csv);

// Writes the FIT every stock mech type reads: names from `name`, explosion
// object 41, destroyed object 101, extent 12, explosion radius 80, destruct
// and explosion damage by weight class, and `moveType` (0 ground, 1 air).
void buildMechTypeFit(char* fit, size_t fitSize, long objectTypeNum, const char* name,
					  long tonnage, long moveType);

#endif // MECHTYPEFIT_H
