// code/typename.h
//
// The name a building, turret or gate type shows (hover label, Mechlopedia)
// when its type file gives no string id. Uses only the FIT reader and the
// runtime string registry (utils/pe_strings), so it can be unit tested.

#ifndef TYPENAME_H
#define TYPENAME_H

class FitIniFile;

// A building, turret or gate type's name id, from its type file: 0 (no
// string; 156 of MechCommander Omnitech's own types have it) becomes an id
// for the file's [ObjectType] Name as display text (displayTypeName), keyed
// by its AppearanceName so two types with the same Name keep apart ids.
// Any other id comes back as it is, as does 0 with no Name. Leaves the file
// in its [ObjectType] block.
long typeNameId(FitIniFile& typeFile, long nameId);

// A type's Name as display text, in place: a trailing ".fit" dropped
// (Omnitech's DavionMediumWall.fit).
void displayTypeName(char* name);

#endif // TYPENAME_H
