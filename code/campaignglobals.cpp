// code/campaignglobals.cpp: see campaignglobals.h.

#include "campaignglobals.h"

#include "inifile.h"

#include <cstdio>

namespace campaignglobals
{

const char* const kBlockName = "MC2XCampaignGlobals";

namespace
{

int values[kCount] = {0, 0, 0, 0, 0};

void valueKey(long index, char* key, size_t keySize)
{
	snprintf(key, keySize, "Value%ld", index);
}

} // namespace

int get(long index)
{
	if ((index < 0) || (index >= kCount))
		return 0;
	return values[index];
}

void set(long index, int value)
{
	if ((index < 0) || (index >= kCount))
		return;
	values[index] = value;
}

void reset()
{
	for (long i = 0; i < kCount; i++)
		values[i] = 0;
}

void save(FitIniFile& file)
{
	bool any = false;
	for (long i = 0; i < kCount; i++)
		any = any || (values[i] != 0);
	if (!any)
		return;
	file.writeBlock(kBlockName);
	for (long i = 0; i < kCount; i++)
	{
		char key[16];
		valueKey(i, key, sizeof(key));
		file.writeIdLong(key, values[i]);
	}
}

void load(FitIniFile& file)
{
	reset();
	if (file.seekBlock(kBlockName) != NO_ERR)
		return;
	for (long i = 0; i < kCount; i++)
	{
		char key[16];
		valueKey(i, key, sizeof(key));
		long value = 0;
		if (file.readIdLong(key, value) == NO_ERR)
			values[i] = (int)value;
	}
}

} // namespace campaignglobals
