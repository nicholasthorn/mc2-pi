// mclib/mc2xart.cpp: see mc2xart.h.

#include "mc2xart.h"

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
