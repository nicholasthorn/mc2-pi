#include "campaignstart.h"

#include "inifile.h"

#include <cstdio>

namespace campaignstart {

bool hasFirstMission(FitIniFile& campaign)
{
	long groups = 0;
	if (campaign.seekBlock("Campaign") != NO_ERR || campaign.readIdLong("GroupCount", groups) != NO_ERR
		|| groups < 1)
		return false;

	long missions = 0;
	if (campaign.seekBlock("Group0") != NO_ERR || campaign.readIdLong("MissionCount", missions) != NO_ERR)
		return false;

	char block[48];
	char fileName[256];
	for (long j = 0; j < missions; j++) {
		snprintf(block, sizeof(block), "Group0Mission%ld", j);
		if (campaign.seekBlock(block) == NO_ERR
			&& campaign.readIdString("FileName", fileName, sizeof(fileName)) == NO_ERR)
			return true;
	}
	return false;
}

} // namespace campaignstart
