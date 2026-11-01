//===========================================================================//
// UI smoke driver: walks the front-end screens for scripts/smoke.sh.
// See uismokedriver.h; the walk itself (screens, routes, timing) is the
// game-free UiSmokeWalk in uismokewalk.cpp.
//===========================================================================//

#include "uismokedriver.h"

// Engine headers in <>: found through the -isystem dirs CMakeLists.txt adds
// for this file, so -Wall -Wextra -Werror applies to this file only.
#include <logistics.h>
#include <mission.h>
#include <missionbegin.h>
#include <mploadmap.h>
#include <mainmenu.h>
#include <optionsscreenwrapper.h>
#include <optionsarea.h>
#include <logisticsdialog.h>
#include <missionselectionscreen.h>
#include <missionbriefingscreen.h>
#include <mechbayscreen.h>
#include <mechlabscreen.h>
#include <mechpurchasescreen.h>
#include <pilotreadyscreen.h>
#include <missionresults.h>
#include <salvagemecharea.h>
#include <pilotreviewarea.h>
#include <mechlopedia.h>
#include <abutton.h>
#include <aedit.h>

#include <gameos.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <chrono>

// mechcmd2.cpp: the options screen of a mission (Ctrl+O), and the flag that
// asks for it
extern OptionsScreenWrapper* optionsScreenWrapper;
extern bool bInvokeOptionsScreenFlag;

namespace {

UiSmokeWalk g_walk;
bool g_started = false;
bool g_over = false;
long g_frames = 0;
std::chrono::steady_clock::time_point g_last;	// previous update()
UiScreen g_from = UiScreen::Unknown;	// the screen the last action left

void finish(const char* line, bool failed)
{
	printf("[UISMOKE] %s\n", line);
	fflush(stdout);
	g_over = true;
	if (failed)
		gos_SetExitCode(1);
	gos_TerminateApplication();
}

// SalvageMechScreen::handleMessage's "Done" (its id in mcui_mr_layout.fit)
const long SALVAGE_DONE = 101;

// Mechlopedia's buttons (ENCYCLO_* in mechlopedia.cpp): six tabs, then
// "Main Menu"
const long MECHLOPEDIA_FIRST_TAB = 130;
const long MECHLOPEDIA_EXIT = 136;

// the save game the walk writes (in smoke.sh's temporary config directory)
// and loads again
const char* const WALK_SAVE = "uismoke";

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

// The logistics screen MissionBegin shows now, or NULL (main menu up, or
// curScreenX/Y off the grid).
LogisticsScreen* UiSmokeDriver::gridScreen(MissionBegin* begin)
{
	if (begin->bSplash
			|| begin->curScreenX < 0 || begin->curScreenX >= MissionBegin::DIM_SCREEN_X
			|| begin->curScreenY < 0 || begin->curScreenY >= MissionBegin::DIM_SCREEN_Y)
		return NULL;
	return begin->screens[begin->curScreenX][begin->curScreenY];
}

// The mission results MissionResults shows after a campaign mission, or NULL.
MissionResults* UiSmokeDriver::results()
{
	if (!logistics || logistics->logisticsState != log_RESULTS)
		return NULL;
	return logistics->missionResults;
}

// The options screen up now: the main menu's, or the one a mission opens.
OptionsXScreen* UiSmokeDriver::optionsScreen(MainMenu* menu)
{
	if (menu && menu->bOptions && menu->optionsScreenWrapper)
		return menu->optionsScreenWrapper->pOptionsScreen;
	if (::optionsScreenWrapper && !::optionsScreenWrapper->isDone())
		return ::optionsScreenWrapper->pOptionsScreen;
	return NULL;
}

// A settled options tab, or Unknown.
UiScreen UiSmokeDriver::optionsTab(OptionsXScreen* options)
{
	if (!options || options->bDone || options->bShowWarning
			|| options->curTab < 0 || options->curTab > 3)
		return UiScreen::Unknown;
	return (UiScreen)((int)UiScreen::OptionsGraphics + options->curTab);
}

UiScreen UiSmokeDriver::currentScreen()
{
	if (MissionResults* r = results()) {
		// salvage first, then pilot review once salvage's Done was pressed
		if (SalvageMechScreen* salvage = r->pSalvageScreen) {
			if (salvage->bDone || salvage->getStatus() != LogisticsScreen::RUNNING)
				return UiScreen::Unknown;
			return UiScreen::Salvage;
		}
		PilotReviewScreen* review = r->pPilotScreen;
		if (!review || !r->bPilotStarted || review->bDone)
			return UiScreen::Unknown;
		// a promotion opens over pilot review as that pilot's entry slides in
		if (PilotPromotionArea* promotion = PilotReviewScreen::s_curPromotion) {
			if (promotion->bDone || !promotion->leftInfo.isDone() || !promotion->rightInfo.isDone())
				return UiScreen::Unknown;
			return UiScreen::Promotion;
		}
		// Done only works once the screen and its pilot list have slid in
		if (!review->entryAnim.isDone() || !review->pilotListBox.isDone())
			return UiScreen::Unknown;
		return UiScreen::PilotReview;
	}

	if (mission && mission->isActive() && mission->gameplayMarked) {
		// the mission's options screen sits over it
		if (::optionsScreenWrapper && !::optionsScreenWrapper->isDone())
			return optionsTab(::optionsScreenWrapper->pOptionsScreen);
		return UiScreen::Mission;
	}

	MissionBegin* begin = logistics ? logistics->getMissionBegin() : NULL;
	if (!begin)
		return UiScreen::Unknown;

	if (begin->bSplash) {
		MainMenu* menu = begin->mainMenu;
		if (!menu || menu->introMovie || menu->getStatus() != LogisticsScreen::RUNNING)
			return UiScreen::Unknown;
		if (menu->bDrawBackground && !menu->introOver)
			return UiScreen::Unknown;	// the intro animation still plays

		if (menu->bOptions)
			return optionsTab(optionsScreen(menu));
		if (MainMenu::bDrawMechlopedia) {
			// drawn once the main menu has faded out (MainMenu::render)
			Mechlopedia* pedia = menu->mechlopedia;
			const bool drawn = menu->fadeTime > menu->fadeOutTime || !menu->fadeOutTime;
			if (!pedia || !drawn || pedia->getStatus() != LogisticsScreen::RUNNING
					|| pedia->currentScreen < 0 || pedia->currentScreen > 5)
				return UiScreen::Unknown;
			return (UiScreen)((int)UiScreen::MechlopediaMechs + pedia->currentScreen);
		}
		if (menu->bLoadCampaign || menu->bSave || menu->bLoad) {
			// one dialog, three uses
			LogisticsSaveDialog* dialog = LogisticsSaveDialog::instance();
			if (!menu->endAnim.isDone() || !dialog || dialog->isDone()
					|| dialog->getStatus() != LogisticsScreen::RUNNING
					|| dialog->bPromptOverwrite || dialog->bDeletePrompt)
				return UiScreen::Unknown;
			return menu->bSave ? UiScreen::Save
					: menu->bLoad ? UiScreen::Load : UiScreen::CampaignSelect;
		}
		if (menu->bLoadSingle) {
			if (!menu->endAnim.isDone() || menu->singleLoadDlg.isDone()
					|| menu->singleLoadDlg.getStatus() != LogisticsScreen::RUNNING)
				return UiScreen::Unknown;
			return UiScreen::SingleMission;
		}
		if (menu->promptToQuit || menu->promptToDisconnect || menu->bLegal
				|| menu->bHostLeftDlg)
			return UiScreen::Unknown;
		return UiScreen::MainMenu;
	}

	LogisticsScreen* screen = gridScreen(begin);
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
	OptionsXScreen* options = optionsScreen(menu);
	LogisticsScreen* screen = gridScreen(begin);

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
	case UiAction::MissionOptions:
		bInvokeOptionsScreenFlag = true;	// what Ctrl+O sets (mechcmd2.cpp)
		return NULL;
	case UiAction::OpenMechlopedia:
		return pressButton(menu, MM_MSG_ENCYCLOPEDIA);
	case UiAction::MechlopediaTab0:
	case UiAction::MechlopediaTab1:
	case UiAction::MechlopediaTab2:
	case UiAction::MechlopediaTab3:
	case UiAction::MechlopediaTab4:
	case UiAction::MechlopediaTab5:
		return pressButton(menu ? menu->mechlopedia : NULL,
				MECHLOPEDIA_FIRST_TAB + (int)action - (int)UiAction::MechlopediaTab0);
	case UiAction::MechlopediaExit:
		return pressButton(menu ? menu->mechlopedia : NULL, MECHLOPEDIA_EXIT);
	case UiAction::MainMenu:
		return pressButton(screen, MB_MSG_MAINMENU);
	case UiAction::ReturnToGame:
		// the button stays on at the start menu, where it does nothing
		if (menu && menu->bDrawBackground)
			return "no game to return to (the start menu)";
		return pressButton(menu, MM_MSG_RETURN_TO_GAME);
	case UiAction::OpenSave:
		// savePath is the player's config unless smoke.sh made a throwaway one
		if (!getenv("MC2_UISMOKE_SAVES"))
			return "saving needs smoke.sh's throwaway config directory (MC2_UISMOKE_SAVES)";
		return pressButton(menu, MM_MSG_SAVE);
	case UiAction::OpenLoad:
		return pressButton(menu, MM_MSG_LOAD);
	case UiAction::SaveAccept:
	case UiAction::LoadAccept:
		return pressButton(LogisticsSaveDialog::instance(), LogisticsScreen::YES);
	case UiAction::WinMission:
		// a single mission goes back to the main menu when it ends, and the
		// campaign's last one plays the final video instead
		if (LogisticsData::instance->isSingleMission())
			return "only a campaign mission shows the results";
		if (LogisticsData::instance->campaignOver())
			return "the campaign's last mission shows no results";
		scenarioResult = mis_PLAYER_WIN_BIG;	// what Mission::update's win cheat sets
		return NULL;
	case UiAction::SalvageDone:
		return pressButton(results() ? results()->pSalvageScreen : NULL, SALVAGE_DONE);
	case UiAction::PilotReviewDone:
		// its Done button has no id: the screen's update() sends aMSG_DONE
		// for a click on any enabled button
		if (PilotReviewScreen* review = results() ? results()->pPilotScreen : NULL) {
			if (review->buttonCount < 1 || !review->buttons[0].isEnabled())
				return "button is disabled";
			review->handleMessage(aMSG_DONE, aMSG_DONE);
			return NULL;
		}
		return "screen is gone";
	case UiAction::PromotionConfirm:
		// its update() sends aMSG_DONE for a press of its confirm button,
		// which is enabled once a skill is checked
		if (PilotPromotionArea* promotion = results() ? PilotReviewScreen::s_curPromotion : NULL) {
			if (promotion->areaRight.buttonCount < 1 || !promotion->areaRight.buttons[0].isEnabled())
				return "button is disabled";
			promotion->handleMessage(aMSG_DONE, aMSG_DONE);
			return NULL;
		}
		return "screen is gone";
	// in-screen actions: each acts on what the screen has selected, which
	// its begin() sets (the first mech, the first pilot); buy picks its own
	// variant, below
	case UiAction::BuyMech:
		// the first variant may not fit the mission's drop weight
		// (MechCommander Omnitech's a_0_2 takes 65 t and lists an Atlas
		// first), and then "add" in the mech bay stays off: pick one that fits
		if (MechPurchaseScreen* purchase = MechPurchaseScreen::instance()) {
			MechListBox& list = purchase->variantListBox;
			long i = 0;
			for (; i < list.GetItemCount(); i++) {
				LogisticsMech* mech = ((MechListBoxItem*)list.GetItem(i))->getMech();
				if (LogisticsData::instance->canAddMechToForceGroup(mech)
						&& NO_ERR == LogisticsData::instance->canPurchaseMech(mech->getVariant()))
					break;
			}
			if (i == list.GetItemCount())
				return "no mech fits the drop weight and the C-bills";
			list.SelectItem(i);
		}
		return pressButton(screen, MB_MSG_ADD);
	case UiAction::AddToForce:
		return pressButton(screen, MB_MSG_ADD);
	case UiAction::AssignPilot:
		return pressButton(screen, PR_MSG_ADD);
	case UiAction::PickSkill:
		// the list holds the skills the pilot's new rank allows and it does
		// not have yet, under a heading per rank
		if (PilotPromotionArea* promotion = results() ? PilotReviewScreen::s_curPromotion : NULL) {
			aListBox& list = promotion->skillListBox;
			for (long i = 0; i < list.GetItemCount(); i++)
				if (SpecialtyListItem* skill = dynamic_cast<SpecialtyListItem*>(list.GetItem(i))) {
					skill->setCheck(true);
					return NULL;
				}
			return "no skill to pick";
		}
		return "screen is gone";
	case UiAction::NameSave:
		// a new name: the dialog suggests the first save in the list, and
		// saving over one asks first. Save is enabled for a name from the
		// next update() on.
		if (LogisticsSaveDialog* dialog = LogisticsSaveDialog::instance()) {
			dialog->edits[0].setEntry(WALK_SAVE);
			return NULL;
		}
		return "screen is gone";
	case UiAction::PickSave:
		// the list holds every save in the config directory: pick the walk's
		// own as a click would (selected, its name in the box, as listed: the
		// file name). Load is enabled for it from the next update() on.
		if (LogisticsSaveDialog* dialog = LogisticsSaveDialog::instance()) {
			aListBox& list = dialog->gameListBox;
			for (long i = 0; i < list.GetItemCount(); i++) {
				const char* name = ((aTextListItem*)list.GetItem(i))->getText();
				if (0 == S_stricmp(name, WALK_SAVE)) {
					list.SelectItem(i);
					dialog->edits[0].setEntry(name);
					return NULL;
				}
			}
			return "the walk's save is not in the list (save+name first)";
		}
		return "screen is gone";
	case UiAction::PickTutorial:
		// the new campaign dialog lists the campaigns by their display name,
		// with the file name hidden: "campaign", then "tutorial"
		if (LogisticsSaveDialog* dialog = LogisticsSaveDialog::instance()) {
			aListBox& list = dialog->gameListBox;
			for (long i = 0; i < list.GetItemCount(); i++) {
				aLocalizedListItem* item = (aLocalizedListItem*)list.GetItem(i);
				if (0 == S_stricmp(item->getHiddenText(), "tutorial")) {
					list.SelectItem(i);
					dialog->edits[0].setEntry(item->getText());
					return NULL;
				}
			}
			return "no tutorial in the list";
		}
		return "screen is gone";
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
			finish(line, true);
			return;
		}
		printf("[UISMOKE] walk of %ld screens: %s\n", g_walk.numSteps(), text);
		fflush(stdout);
		g_last = std::chrono::steady_clock::now();
	}

	// real time, not frameLength: the game caps that at 0.25 s a frame, so on
	// a slow (sanitized) run the walk's timeout would come after smoke.sh's
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	const float dt = std::chrono::duration<float>(now - g_last).count();
	g_last = now;

	++g_frames;
	const UiScreen current = currentScreen();
	const UiSmokeWalk::Result r = g_walk.update(current, dt);
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
		if (*uiActionName(r.action)) {
			// in-screen action: the screen stays
			if (const char* why = press(r.action)) {
				snprintf(line, sizeof(line), "FAIL: cannot %s on %s: %s",
						uiActionName(r.action), uiScreenName(r.screen), why);
				finish(line, true);
			} else {
				printf("[ACTION] %s+%s\n", uiScreenName(r.screen), uiActionName(r.action));
				fflush(stdout);
			}
		} else if (const char* why = press(r.action)) {
			snprintf(line, sizeof(line), "FAIL: cannot go from %s to %s: %s",
					uiScreenName(g_from), uiScreenName(r.screen), why);
			finish(line, true);
		}
		break;
	case UiSmokeWalk::Event::Done:
		snprintf(line, sizeof(line), "done: %ld screens in %ld frames",
				g_walk.numSteps(), g_frames);
		finish(line, false);
		break;
	case UiSmokeWalk::Event::Timeout:
		snprintf(line, sizeof(line), "FAIL: %s did not appear within %.0f s (on %s)",
				uiScreenName(r.screen), g_walk.timeoutSeconds, uiScreenName(current));
		finish(line, true);
		break;
	case UiSmokeWalk::Event::Lost:
		snprintf(line, sizeof(line), "FAIL: %s went away before its hold ended (now on %s)",
				uiScreenName(r.screen), uiScreenName(current));
		finish(line, true);
		break;
	}
}
