//===========================================================================//
// UI smoke driver: walks the front-end screens for scripts/smoke.sh.
// See uismokedriver.h; the walk itself (screens, routes, timing) is the
// game-free UiSmokeWalk in uismokewalk.cpp.
//===========================================================================//

#include "uismokedriver.h"

#include "logistics.h"
#include "mission.h"
#include "missionbegin.h"
#include "mploadmap.h"
#include "mainmenu.h"
#include "optionsscreenwrapper.h"
#include "optionsarea.h"
#include "logisticsdialog.h"
#include "missionselectionscreen.h"
#include "missionbriefingscreen.h"
#include "mechbayscreen.h"
#include "mechlabscreen.h"
#include "mechpurchasescreen.h"
#include "pilotreadyscreen.h"
#include "abutton.h"
#include "timing.h"

#include <gameos.hpp>

#include <stdio.h>

// Button ids that only exist as #defines in the screens' .cpp files
static const long MM_MSG_NEW_CAMPAIGN = 90;		// mainmenu.cpp
static const long MM_MSG_OPTIONS = 95;			// mainmenu.cpp
static const long MM_MSG_SINGLE_MISSION = 98;	// mainmenu.cpp
static const long MSB_TAB0 = 200;				// optionsarea.cpp, tabs 200..203

namespace {

UiSmokeWalk g_walk;
bool g_started = false;
bool g_over = false;
long g_frames = 0;
UiScreen g_from = UiScreen::Unknown;	// the screen the last action left

void finish(const char* line)
{
	printf("[UISMOKE] %s\n", line);
	fflush(stdout);
	g_over = true;
	gos_TerminateApplication();
}

// Press a button the way a click does (aButton::update sends its parent the
// same message), but only if a player could: it must exist and be enabled.
const char* pressButton(LogisticsScreen* screen, long id)
{
	if (!screen)
		return "screen is gone";
	aButton* button = screen->getButton(id);
	if (!button)
		return "button not found";
	if (!button->isEnabled())
		return "button is disabled";
	screen->handleMessage(aMSG_LEFTMOUSEDOWN, id);
	return NULL;
}

} // namespace

UiScreen UiSmokeDriver::currentScreen()
{
	if (mission && mission->isActive() && mission->gameplayMarked)
		return UiScreen::Mission;

	MissionBegin* begin = logistics ? logistics->getMissionBegin() : NULL;
	if (!begin)
		return UiScreen::Unknown;

	if (begin->bSplash) {
		MainMenu* menu = begin->mainMenu;
		if (!menu || menu->introMovie || menu->getStatus() != LogisticsScreen::RUNNING)
			return UiScreen::Unknown;
		if (menu->bDrawBackground && !menu->introOver)
			return UiScreen::Unknown;	// the intro animation still plays

		if (menu->bOptions) {
			OptionsXScreen* options = menu->optionsScreenWrapper ?
				menu->optionsScreenWrapper->pOptionsScreen : NULL;
			if (!options || options->bDone || options->bShowWarning
					|| options->curTab < 0 || options->curTab > 3)
				return UiScreen::Unknown;
			return (UiScreen)((int)UiScreen::OptionsGraphics + options->curTab);
		}
		if (menu->bLoadCampaign) {
			LogisticsSaveDialog* dialog = LogisticsSaveDialog::instance();
			if (!menu->endAnim.isDone() || !dialog || dialog->isDone()
					|| dialog->getStatus() != LogisticsScreen::RUNNING)
				return UiScreen::Unknown;
			return UiScreen::CampaignSelect;
		}
		if (menu->bLoadSingle) {
			if (!menu->endAnim.isDone() || menu->singleLoadDlg.isDone()
					|| menu->singleLoadDlg.getStatus() != LogisticsScreen::RUNNING)
				return UiScreen::Unknown;
			return UiScreen::SingleMission;
		}
		if (menu->bSave || menu->bLoad || menu->promptToQuit
				|| menu->promptToDisconnect || menu->bLegal || menu->bHostLeftDlg
				|| MainMenu::bDrawMechlopedia)
			return UiScreen::Unknown;
		return UiScreen::MainMenu;
	}

	if (begin->curScreenX < 0 || begin->curScreenX >= MissionBegin::DIM_SCREEN_X
			|| begin->curScreenY < 0 || begin->curScreenY >= MissionBegin::DIM_SCREEN_Y)
		return UiScreen::Unknown;
	LogisticsScreen* screen = begin->screens[begin->curScreenX][begin->curScreenY];
	if (!screen || screen->getStatus() != LogisticsScreen::RUNNING)
		return UiScreen::Unknown;
	if (dynamic_cast<MissionSelectionScreen*>(screen))
		return UiScreen::MissionSelection;
	if (dynamic_cast<MissionBriefingScreen*>(screen))
		return UiScreen::Briefing;
	if (dynamic_cast<MechBayScreen*>(screen))
		return UiScreen::MechBay;
	if (dynamic_cast<MechLabScreen*>(screen))
		return UiScreen::MechLab;
	if (dynamic_cast<MechPurchaseScreen*>(screen))
		return UiScreen::MechPurchase;
	if (dynamic_cast<PilotReadyScreen*>(screen))
		return UiScreen::PilotReady;
	return UiScreen::Unknown;
}

// Returns NULL, or why the button could not be pressed.
const char* UiSmokeDriver::press(UiAction action)
{
	MissionBegin* begin = logistics ? logistics->getMissionBegin() : NULL;
	if (!begin)
		return "no front end";
	MainMenu* menu = begin->mainMenu;
	OptionsXScreen* options = (menu && menu->optionsScreenWrapper) ?
		menu->optionsScreenWrapper->pOptionsScreen : NULL;
	LogisticsScreen* screen = NULL;
	if (!begin->bSplash && begin->curScreenX >= 0 && begin->curScreenY >= 0)
		screen = begin->screens[begin->curScreenX][begin->curScreenY];

	switch (action) {
	case UiAction::OpenOptions:
		return pressButton(menu, MM_MSG_OPTIONS);
	case UiAction::NewCampaign:
		return pressButton(menu, MM_MSG_NEW_CAMPAIGN);
	case UiAction::OptionsTab0:
	case UiAction::OptionsTab1:
	case UiAction::OptionsTab2:
	case UiAction::OptionsTab3:
		return pressButton(options, MSB_TAB0 + (int)action - (int)UiAction::OptionsTab0);
	case UiAction::OptionsCancel:
		// Cancel, not OK: OK saves the prefs into the user's config
		return pressButton(options, LogisticsScreen::NO);
	case UiAction::DialogAccept:
		return pressButton(LogisticsSaveDialog::instance(), LogisticsScreen::YES);
	case UiAction::DialogCancel:
		return pressButton(LogisticsSaveDialog::instance(), LogisticsScreen::NO);
	case UiAction::SingleMission:
		return pressButton(menu, MM_MSG_SINGLE_MISSION);
	case UiAction::SingleAccept:
		if (menu) {
			// The list holds every mission the newest campaign save has
			// reached; take the last one (smoke.sh's save fixture makes it
			// mc2_02, the first with purchasing and the mech lab). With
			// nothing selected OK just goes back to the main menu.
			MPLoadMap& dialog = menu->singleLoadDlg;
			const long count = dialog.mapList.GetItemCount();
			if (count <= 0)
				return "no mission to pick (the list comes from campaign saves)";
			dialog.mapList.SelectItem(count - 1);
			dialog.updateMapInfo();
		}
		return pressButton(menu ? &menu->singleLoadDlg : NULL, LogisticsScreen::YES);
	case UiAction::SingleCancel:
		return pressButton(menu ? &menu->singleLoadDlg : NULL, LogisticsScreen::NO);
	case UiAction::Next:
		return pressButton(screen, MB_MSG_NEXT);
	case UiAction::Previous:
		return pressButton(screen, MB_MSG_PREV);
	case UiAction::ChangeLoadout:
		return pressButton(screen, MB_MSG_CHANGE_LOADOUT);
	case UiAction::BuyMechs:
		return pressButton(screen, MB_MSG_BUY);
	case UiAction::None:
		break;
	}
	return "no button for this step";
}

void UiSmokeDriver::update()
{
	const char* text = gos_GetUiSmokeWalk();
	if (!text || g_over)
		return;

	char line[256];
	if (!g_started) {
		g_started = true;
		char err[128];
		if (!g_walk.parse(text, err, sizeof(err))) {
			snprintf(line, sizeof(line), "FAIL: bad walk \"%s\": %s", text, err);
			finish(line);
			return;
		}
		printf("[UISMOKE] walk of %ld screens: %s\n", g_walk.numSteps(), text);
		fflush(stdout);
	}

	++g_frames;
	const UiScreen current = currentScreen();
	const UiSmokeWalk::Result r = g_walk.update(current, frameLength);
	switch (r.event) {
	case UiSmokeWalk::Event::Waiting:
		break;
	case UiSmokeWalk::Event::Shown:
		// scripts/smoke.sh checks these, in order, against the walk
		printf("[SCREEN] %s shown\n", uiScreenName(r.screen));
		fflush(stdout);
		g_from = r.screen;
		break;
	case UiSmokeWalk::Event::Act:
		if (const char* why = press(r.action)) {
			snprintf(line, sizeof(line), "FAIL: cannot go from %s to %s: %s",
					uiScreenName(g_from), uiScreenName(r.screen), why);
			finish(line);
		}
		break;
	case UiSmokeWalk::Event::Done:
		snprintf(line, sizeof(line), "done: %ld screens in %ld frames",
				g_walk.numSteps(), g_frames);
		finish(line);
		break;
	case UiSmokeWalk::Event::Timeout:
		snprintf(line, sizeof(line), "FAIL: %s did not appear within %.0f s (on %s)",
				uiScreenName(r.screen), g_walk.timeoutSeconds, uiScreenName(current));
		finish(line);
		break;
	}
}
