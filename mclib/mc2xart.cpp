// mclib/mc2xart.cpp: see mc2xart.h.

#include "mc2xart.h"

#include "file.h"

#include <cstdio>
#include <cstring>

namespace
{

// Whole names MC2X shortened as well as renamed (contents the same as
// stock's, checked against R70 and REAP3).
struct Whole
{
	const char* stockName;
	const char* mc2xName;
};

const Whole kWholeNames[] = {
	{ "mcl_cm_opplayer", "mc2x_opplayer" },
	{ "mcl_gn_availablemechentry", "mc2x_availablemechentry" },
	{ "mcl_gn_deploymentteams", "mc2x_deploymentteams" },
	{ "mcl_pr_availablepilotentry", "mc2x_availablepilotentry" },
	{ "mcl_mp_loadmap", "mc2x_loadmap" },
	{ "mcl_mp_loadmap_list0", "mc2x_loadmap_list0" },
	{ "mcl_mp_mapinfo", "mc2x_mapinfo" },
	// data/cameras
	{ "cameras", "mc2xcams" },
	{ "colors", "mc2xcols" },
	// data/missions
	{ "gamesys", "mc2xgsys" },
};

struct Rename
{
	const char* stockPrefix;
	const char* mc2xPrefix;
};

const Rename kRenames[] = {
	{ "mcui_keyref", "mc2x_keyref" },
	{ "buttonlayout", "mc2x_buttonlayout" },
	{ "mcl_", "mc2x_" },
};

bool copyName(const char* name, char* out, size_t outSize)
{
	const int written = snprintf(out, outSize, "%s", name);
	return (written > 0) && ((size_t)written < outSize);
}

} // namespace

// Data files MC2X renamed (contents in stock's formats, checked against R70
// and REAP3).
const Whole kDataFiles[] = {
	{ "compbas.csv", "mc2xcomp.csv" },
	{ "buildings.csv", "mc2x_inv.csv" },
	{ "pilots.csv", "mc2xpilots.csv" },
	{ "gamesys.fit", "mc2xgsys.fit" },
	{ "mc2.fx", "mc2x.fx" },
	{ "corebrain.abx", "mc2xcore.abx" },
};

bool mc2xDataFileName(const char* stockFile, char* out, size_t outSize)
{
	if (!stockFile || !out || (outSize == 0))
		return false;
	for (const Whole& file : kDataFiles)
		if (strcmp(stockFile, file.stockName) == 0)
			return copyName(file.mc2xName, out, outSize);
	return false;
}

void stockOrMC2XFileName(const char* folder, const char* stockFile, char* name, size_t nameSize)
{
	snprintf(name, nameSize, "%s", stockFile);
	char path[1024];
	snprintf(path, sizeof(path), "%s%s", folder, stockFile);
	if (fileExists(path))
		return;
	char mc2x[128];
	if (!mc2xDataFileName(stockFile, mc2x, sizeof(mc2x)))
		return;
	snprintf(path, sizeof(path), "%s%s", folder, mc2x);
	if (fileExists(path))
		snprintf(name, nameSize, "%s", mc2x);
}

bool mc2xArtName(const char* stockName, char* out, size_t outSize)
{
	if (!stockName || !out || (outSize == 0))
		return false;
	for (const Whole& whole : kWholeNames)
		if (strcmp(stockName, whole.stockName) == 0)
			return copyName(whole.mc2xName, out, outSize);
	for (const Rename& rename : kRenames)
	{
		const size_t prefixLength = strlen(rename.stockPrefix);
		if (strncmp(stockName, rename.stockPrefix, prefixLength) != 0)
			continue;
		const char* rest = stockName + prefixLength;
		// "mcl_" alone names nothing
		if ((rest[0] == '\0') && (rename.stockPrefix[prefixLength - 1] == '_'))
			return false;
		const int written = snprintf(out, outSize, "%s%s", rename.mc2xPrefix, rest);
		return (written > 0) && ((size_t)written < outSize);
	}
	return false;
}
