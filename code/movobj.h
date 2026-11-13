//---------------------------------------------------------------------------
// movobj.h: MechCommander Omnitech object types past object2.pak.
//
// Omnitech numbers its new object types from 1200 and lists them in
// data/objects/movobj.fit (see objtype.cpp). Kept apart from objtype.cpp so
// the logistics code, which the viewer builds without the object type
// manager, can use it.
//---------------------------------------------------------------------------

#ifndef MOVOBJ_H
#define MOVOBJ_H

class FitIniFile;
class PacketFile;

// Looks objTypeNum up in movobj.fit (opened on first use): its class and its
// lowercased CSV / .fit name. False if there is no movobj.fit or no entry.
bool findMovObjType (long objTypeNum, long& typeClass, char* name, long nameSize);

// Closes movobj.fit; the next lookup reopens it.
void closeMovObjFile (void);

// Opens the type FIT a vehicle, elemental or building reads its profile
// from: its packet in pak, or for an Omnitech number the loose .fit
// movobj.fit names (not a mech: those have only a CSV).
long openTypeProfile (long objTypeNum, FitIniFile& profile, PacketFile* pak);

// A building, turret or gate type's name id, from its type file: 0 (no
// string; 156 of Omnitech's own types have it) becomes an id for the file's
// [ObjectType] Name, so the hover label and the Mechlopedia show that
// instead of "missing string res" or nothing. Leaves the file in its
// [ObjectType] block.
long typeNameId (FitIniFile& typeFile, long nameId);

#endif
