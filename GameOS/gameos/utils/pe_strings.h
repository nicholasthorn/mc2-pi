#ifndef PE_STRINGS_H
#define PE_STRINGS_H

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>

// Reads the string table (RT_STRING resources) of a Windows PE file (.dll or
// .exe, PE32 or PE32+) held in memory, as the game's resource DLLs store
// their text: id -> string. Text is converted from UTF-16 to Windows-1252,
// the encoding of the stock string table; anything else becomes '?'. The
// first language of each block is used. Every offset is bounds-checked, so a
// truncated or foreign file just yields fewer strings. Returns false if the
// data is not a PE file with a resource section. GL-free so it can be unit
// tested.
bool readPeStringTable(const uint8_t* data, size_t size,
                       std::map<uint32_t, std::string>& out);

// True for what a string table holds for an id it has no text for: "undefined",
// in any case, alone or as "undefined_<something>".
bool isPlaceholderString(const char* text);

// Text the data names a thing by where it gives no string id (Omnitech's
// building, turret and gate types with a name id of 0 carry only their
// [ObjectType] Name), given an id that gos_GetResourceString finds. Ids start
// at kRuntimeStringBase, past the 16 bits a string table's ids have, so they
// never meet one. The same key and text keep their id; the same text under
// another key (another type with the same Name) gets its own, so a list that
// finds entries by id tells them apart. The text stays put for the life of
// the program. No text (NULL or "") gets 0.
const uint32_t kRuntimeStringBase = 0x10000;
uint32_t addRuntimeString(const char* text, const char* key = "");

// The text addRuntimeString gave this id, or NULL.
const char* findRuntimeString(uint32_t id);

#endif // PE_STRINGS_H
