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
	"salvage",
	"pilotreview",
	"promotion",
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
		if (to == UiScreen::MechBay)
			return UiAction::Previous;	// Cancel
		break;
	case UiScreen::MechPurchase:
		// Accept: "Back" cancels, selling back everything bought here
		if (to == UiScreen::MechBay)
			return UiAction::Next;
		break;
	case UiScreen::Mission:
		// the mission's data can skip either results screen, and a promotion
		// opens over pilot review as it comes up
		if (to == UiScreen::Salvage || to == UiScreen::PilotReview || to == UiScreen::Promotion)
			return UiAction::WinMission;
		break;
	case UiScreen::Salvage:
		if (to == UiScreen::PilotReview || to == UiScreen::Promotion
				|| to == UiScreen::MissionSelection)
			return UiAction::SalvageDone;
		break;
	case UiScreen::PilotReview:
		// on to the campaign's next mission
		if (to == UiScreen::MissionSelection)
			return UiAction::PilotReviewDone;
		break;
	case UiScreen::Promotion:
		if (to == UiScreen::PilotReview)
			return UiAction::PromotionConfirm;
		break;
	default:
		break;
	}
	return UiAction::None;
}

namespace {

struct ScreenAction {
	UiScreen screen;
	const char* name;
	UiAction action;
};

const ScreenAction SCREEN_ACTIONS[] = {
	{ UiScreen::MechPurchase, "buy", UiAction::BuyMech },
	{ UiScreen::MechBay, "add", UiAction::AddToForce },
	{ UiScreen::PilotReady, "pilot", UiAction::AssignPilot },
	{ UiScreen::Promotion, "skill", UiAction::PickSkill },
};

} // namespace

UiAction uiScreenAction(UiScreen s, const char* name, size_t len)
{
	for (const ScreenAction& a : SCREEN_ACTIONS)
		if (a.screen == s && strlen(a.name) == len && strncmp(a.name, name, len) == 0)
			return a.action;
	return UiAction::None;
}

const char* uiActionName(UiAction action)
{
	for (const ScreenAction& a : SCREEN_ACTIONS)
		if (a.action == action)
			return a.name;
	return "";
}

bool UiSmokeWalk::parse(const char* text, char* err, size_t errSize)
{
	count = 0;
	index = 0;
	holding = false;
	acted = false;
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
		// "<screen>" or "<screen>+<action>"
		const char* plus = (const char*)memchr(p, '+', len);
		const size_t nameLen = plus ? (size_t)(plus - p) : len;
		const UiScreen s = uiScreenFromName(p, nameLen);
		if (s == UiScreen::Unknown) {
			snprintf(err, errSize, "unknown screen '%.*s'", (int)nameLen, p);
			count = 0;
			return false;
		}
		UiAction action = UiAction::None;
		if (plus) {
			const size_t actionLen = len - nameLen - 1;
			action = uiScreenAction(s, plus + 1, actionLen);
			if (action == UiAction::None) {
				snprintf(err, errSize, "%s has no action '%.*s'", uiScreenName(s),
						(int)actionLen, plus + 1);
				count = 0;
				return false;
			}
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
		if (count > 0 && uiRoute(steps[count - 1].screen, s) == UiAction::None) {
			snprintf(err, errSize, "no route from %s to %s",
					uiScreenName(steps[count - 1].screen), uiScreenName(s));
			count = 0;
			return false;
		}
		steps[count].screen = s;
		steps[count].action = action;
		++count;
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
		return { Event::Timeout, steps[index].screen, UiAction::None };
	if (state == State::Lost)
		return { Event::Lost, steps[index].screen, UiAction::None };

	const Step& step = steps[index];
	if (!holding) {
		if (current == step.screen) {
			holding = true;
			acted = false;
			held = 0;
			return { Event::Shown, step.screen, UiAction::None };
		}
		waited += dt;
		if (waited > timeoutSeconds) {
			state = State::TimedOut;
			return { Event::Timeout, step.screen, UiAction::None };
		}
		return { Event::Waiting, step.screen, UiAction::None };
	}

	// the button at the end of the hold belongs to this screen: if something
	// else took over meanwhile, pressing it would act on the wrong screen
	if (current != step.screen) {
		state = State::Lost;
		return { Event::Lost, step.screen, UiAction::None };
	}
	if (++held < holdFrames)
		return { Event::Waiting, step.screen, UiAction::None };

	// an in-screen action, then a second hold to let the screen show its effect
	if (step.action != UiAction::None && !acted) {
		acted = true;
		held = 0;
		return { Event::Act, step.screen, step.action };
	}

	holding = false;
	if (++index >= count) {
		state = State::Done;
		return { Event::Done, step.screen, UiAction::None };
	}
	waited = 0.0f;
	return { Event::Act, steps[index].screen, uiRoute(step.screen, steps[index].screen) };
}
