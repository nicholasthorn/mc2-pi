#ifndef CAMPAIGNSTART_H
#define CAMPAIGNSTART_H

// Whether a campaign file can begin a new campaign. LogisticsMissionInfo::init
// starts at the first group, and LogisticsData::startNewCampaign picks that
// group's first mission: [Group0]'s MissionCount and a [Group0MissionN] block
// with a FileName. Game-free so it can be unit tested.

class FitIniFile;

namespace campaignstart {

bool hasFirstMission(FitIniFile& campaign);

} // namespace campaignstart

#endif // CAMPAIGNSTART_H
