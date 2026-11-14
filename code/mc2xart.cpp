// code/mc2xart.cpp: see mc2xart.h.

#include "mc2xart.h"

#include "inifile.h"
#include "paths.h"

#include <cstdio>
#include <cstring>

namespace
{

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

} // namespace

bool mc2xArtName(const char* stockName, char* out, size_t outSize)
{
	if (!stockName || !out || (outSize == 0))
		return false;
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

long openArtLayout(FitIniFile& file, const char* stockName, bool* usedMC2X)
{
	if (usedMC2X)
		*usedMC2X = false;
	char path[512];
	snprintf(path, sizeof(path), "%s%s.fit", artPath, stockName);
	const long result = file.open(path);
	if (result == NO_ERR)
		return NO_ERR;
	char mc2xName[128];
	if (!mc2xArtName(stockName, mc2xName, sizeof(mc2xName)))
		return result;
	snprintf(path, sizeof(path), "%s%s.fit", artPath, mc2xName);
	if (file.open(path) != NO_ERR)
		return result;
	if (usedMC2X)
		*usedMC2X = true;
	return NO_ERR;
}
