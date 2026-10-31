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

#endif // PE_STRINGS_H
