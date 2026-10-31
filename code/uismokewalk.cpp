#include "uismokewalk.h"

#include <cstdio>
#include <cstring>

namespace {

const char* const SCREEN_NAMES[(int)UiScreen::Count] = {
	"mainmenu",
	"options-graphics",
	"options-audio",
	"options-gameplay",
	"options-hotkeys",
	"campaign",
	"singlemission",
	"missionselection",
	"briefing",
	"mechbay",
	"mechlab",
	"mechpurchase",
	"pilotready",
	"mission",
};

bool isOptions(UiScreen s)
{
	return s >= UiScreen::OptionsGraphics && s <= UiScreen::OptionsHotkeys;
}

} // namespace

const char* uiScreenName(UiScreen s)
{
	if (s < UiScreen::MainMenu || s >= UiScreen::Count)
		return "unknown";
	return SCREEN_NAMES[(int)s];
}

UiScreen uiScreenFromName(const char* name, size_t len)
{
	for (int i = 0; i < (int)UiScreen::Count; ++i)
		if (strlen(SCREEN_NAMES[i]) == len && strncmp(SCREEN_NAMES[i], name, len) == 0)
			return (UiScreen)i;
	return UiScreen::Unknown;
}

UiAction uiRoute(UiScreen from, UiScreen to)
{
	if (from == to)
		return UiAction::None;

	if (isOptions(from) && isOptions(to))
		return (UiAction)((int)UiAction::OptionsTab0 + (int)to - (int)UiScreen::OptionsGraphics);
	if (isOptions(from) && to == UiScreen::MainMenu)
		return UiAction::OptionsCancel;

	switch (from) {
	case UiScreen::MainMenu:
		// the options screen opens on its first tab
		if (to == UiScreen::OptionsGraphics)
			return UiAction::OpenOptions;
		if (to == UiScreen::CampaignSelect)
			return UiAction::NewCampaign;
		if (to == UiScreen::SingleMission)
			return UiAction::SingleMission;
		break;
	case UiScreen::CampaignSelect:
		// where OK leads depends on the campaign's first mission: the stock
		// campaign's mc2_01 skips logistics and loads straight away
		if (to == UiScreen::MissionSelection || to == UiScreen::Mission)
			return UiAction::DialogAccept;
		if (to == UiScreen::MainMenu)
			return UiAction::DialogCancel;
		break;
	case UiScreen::SingleMission:
		// a single mission skips mission selection
		if (to == UiScreen::Briefing)
			return UiAction::SingleAccept;
		if (to == UiScreen::MainMenu)
			return UiAction::SingleCancel;
		break;
	case UiScreen::MissionSelection:
		if (to == UiScreen::Briefing)
			return UiAction::Next;
		break;
	case UiScreen::Briefing:
		if (to == UiScreen::MechBay)
			return UiAction::Next;
		if (to == UiScreen::MissionSelection)
			return UiAction::Previous;
		break;
	case UiScreen::MechBay:
		if (to == UiScreen::PilotReady)
			return UiAction::Next;
		if (to == UiScreen::Briefing)
			return UiAction::Previous;
		if (to == UiScreen::MechLab)
			return UiAction::ChangeLoadout;
		if (to == UiScreen::MechPurchase)
			return UiAction::BuyMechs;
		break;
	case UiScreen::PilotReady:
		if (to == UiScreen::Mission)
			return UiAction::Next;
		if (to == UiScreen::MechBay)
			return UiAction::Previous;
		break;
	case UiScreen::MechLab:
	case UiScreen::MechPurchase:
		if (to == UiScreen::MechBay)
			return UiAction::Previous;
		break;
	default:
		break;
	}
	return UiAction::None;
}

bool UiSmokeWalk::parse(const char* text, char* err, size_t errSize)
{
	count = 0;
	index = 0;
	holding = false;
	held = 0;
	waited = 0.0f;
	state = State::Running;

	if (!text || !*text) {
		snprintf(err, errSize, "empty walk");
		return false;
	}

	const char* p = text;
	for (;;) {
		const char* comma = strchr(p, ',');
		const size_t len = comma ? (size_t)(comma - p) : strlen(p);
		const UiScreen s = uiScreenFromName(p, len);
		if (s == UiScreen::Unknown) {
			snprintf(err, errSize, "unknown screen '%.*s'", (int)len, p);
			count = 0;
			return false;
		}
		if (count == MAX_STEPS) {
			snprintf(err, errSize, "too many screens (at most %ld)", MAX_STEPS);
			count = 0;
			return false;
		}
		if (count == 0 && s != UiScreen::MainMenu) {
			snprintf(err, errSize, "a walk starts at mainmenu, not '%s'", uiScreenName(s));
			return false;
		}
		if (count > 0 && uiRoute(steps[count - 1], s) == UiAction::None) {
			snprintf(err, errSize, "no route from %s to %s", uiScreenName(steps[count - 1]),
					uiScreenName(s));
			count = 0;
			return false;
		}
		steps[count++] = s;
		if (!comma)
			break;
		p = comma + 1;
	}
	return true;
}

UiSmokeWalk::Result UiSmokeWalk::update(UiScreen current, float dt)
{
	if (count == 0 || state == State::Done)
		return { Event::Done, UiScreen::Unknown, UiAction::None };
	if (state == State::TimedOut)
		return { Event::Timeout, steps[index], UiAction::None };

	const UiScreen target = steps[index];
	if (!holding) {
		if (current == target) {
			holding = true;
			held = 0;
			return { Event::Shown, target, UiAction::None };
		}
		waited += dt;
		if (waited > timeoutSeconds) {
			state = State::TimedOut;
			return { Event::Timeout, target, UiAction::None };
		}
		return { Event::Waiting, target, UiAction::None };
	}

	if (++held < holdFrames)
		return { Event::Waiting, target, UiAction::None };

	holding = false;
	if (++index >= count) {
		state = State::Done;
		return { Event::Done, target, UiAction::None };
	}
	waited = 0.0f;
	return { Event::Act, steps[index], uiRoute(target, steps[index]) };
}
