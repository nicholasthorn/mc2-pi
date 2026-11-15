// mclib/mc2xart.h
//
// wolfman-MC2X renames the interface layouts the game opens by name:
// mcl_<x> is mc2x_<x> (a few drop a part as well, mcl_gn_deploymentteams is
// mc2x_deploymentteams), buttonlayout<w> is mc2x_buttonlayout<w> and
// mcui_keyref<x> is mc2x_keyref<x>; in the camera folder cameras.fit is
// mc2xcams.fit and colors.fit mc2xcols.fit; in the missions folder gamesys.fit
// is mc2xgsys.fit. FitIniFile::open tries MC2X's
// name when a file in those folders is not there, so stock and Omnitech data
// never reach it. Names only, so it can be unit tested.

#ifndef MC2XART_H
#define MC2XART_H

#include <cstddef>

// MC2X's name for a stock art layout name (no folder, no extension) into
// out; false if MC2X has no rename for it or it does not fit.
bool mc2xArtName(const char* stockName, char* out, size_t outSize);

// MC2X's name for a stock data file the game opens by name (name and
// extension, lower case, no folder): compbas.csv is mc2xcomp.csv,
// buildings.csv mc2x_inv.csv, pilots.csv mc2xpilots.csv, gamesys.fit
// mc2xgsys.fit, mc2.fx mc2x.fx, corebrain.abx mc2xcore.abx. False if it has
// none or it does not fit.
bool mc2xDataFileName(const char* stockFile, char* out, size_t outSize);

// The file name (no folder) to open in folder: stockFile, or MC2X's name for
// it when only that is there (on disk or in a fast file). Stock and Omnitech
// always have theirs.
void stockOrMC2XFileName(const char* folder, const char* stockFile, char* name, size_t nameSize);

#endif
