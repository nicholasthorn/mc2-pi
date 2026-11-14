// mclib/mc2xart.h
//
// wolfman-MC2X renames the interface layouts the game opens by name:
// mcl_<x> is mc2x_<x> (a few drop a part as well, mcl_gn_deploymentteams is
// mc2x_deploymentteams), buttonlayout<w> is mc2x_buttonlayout<w> and
// mcui_keyref<x> is mc2x_keyref<x>; in the camera folder cameras.fit is
// mc2xcams.fit and colors.fit mc2xcols.fit. FitIniFile::open tries MC2X's
// name when a file in those folders is not there, so stock and Omnitech data
// never reach it. Names only, so it can be unit tested.

#ifndef MC2XART_H
#define MC2XART_H

#include <cstddef>

// MC2X's name for a stock art layout name (no folder, no extension) into
// out; false if MC2X has no rename for it or it does not fit.
bool mc2xArtName(const char* stockName, char* out, size_t outSize);

#endif
