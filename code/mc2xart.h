// code/mc2xart.h
//
// wolfman-MC2X renames the interface layouts the game opens by name:
// mcl_<x> is mc2x_<x>, buttonlayout<w> is mc2x_buttonlayout<w> and
// mcui_keyref<x> is mc2x_keyref<x>. The game tries MC2X's name only when the
// stock file is not there, so stock and Omnitech data never reach it. Uses
// only the FIT reader, so it can be unit tested.

#ifndef MC2XART_H
#define MC2XART_H

#include <cstddef>

class FitIniFile;

// MC2X's name for a stock art layout name (no folder, no extension) into
// out; false if MC2X has no rename for it or it does not fit.
bool mc2xArtName(const char* stockName, char* out, size_t outSize);

// Opens artPath + stockName + ".fit", or MC2X's name for it when that is not
// there. Returns the stock open's error if neither opens; *usedMC2X (when
// given) says which one did.
long openArtLayout(FitIniFile& file, const char* stockName, bool* usedMC2X = nullptr);

#endif
