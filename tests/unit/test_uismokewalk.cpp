// UiSmokeWalk (code/uismokewalk.cpp): the screen walk "-uismoke <walk>" plays
// through the front end. The driver (code/uismokedriver.cpp) reports the
// settled screen each frame and does what the walk says.
#include "doctest.h"

#include "uismokewalk.h"

#include <cstring>

namespace {

struct Run {
	UiSmokeWalk walk;
	char err[128] = "";
};

} // namespace

TEST_SUITE("UiSmokeWalk") {

TEST_CASE("screen names round-trip") {
	for (int i = 0; i < (int)UiScreen::Count; ++i) {
		const UiScreen s = (UiScreen)i;
		const char* name = uiScreenName(s);
		REQUIRE(name != nullptr);
		CHECK(uiScreenFromName(name, strlen(name)) == s);
	}
	CHECK(uiScreenFromName("mechbayx", 8) == UiScreen::Unknown);
	CHECK(uiScreenFromName("mech", 4) == UiScreen::Unknown);
	CHECK(std::strcmp(uiScreenName(UiScreen::Unknown), "unknown") == 0);
}

TEST_CASE("routes are the buttons a player presses") {
	CHECK(uiRoute(UiScreen::MainMenu, UiScreen::OptionsGraphics) == UiAction::OpenOptions);
	CHECK(uiRoute(UiScreen::OptionsGraphics, UiScreen::OptionsHotkeys) == UiAction::OptionsTab3);
	CHECK(uiRoute(UiScreen::OptionsHotkeys, UiScreen::OptionsAudio) == UiAction::OptionsTab1);
	CHECK(uiRoute(UiScreen::OptionsAudio, UiScreen::MainMenu) == UiAction::OptionsCancel);
	CHECK(uiRoute(UiScreen::MainMenu, UiScreen::CampaignSelect) == UiAction::NewCampaign);
	CHECK(uiRoute(UiScreen::CampaignSelect, UiScreen::MissionSelection) == UiAction::DialogAccept);
	CHECK(uiRoute(UiScreen::CampaignSelect, UiScreen::MainMenu) == UiAction::DialogCancel);
	CHECK(uiRoute(UiScreen::CampaignSelect, UiScreen::Mission) == UiAction::DialogAccept);
	CHECK(uiRoute(UiScreen::MainMenu, UiScreen::SingleMission) == UiAction::SingleMission);
	CHECK(uiRoute(UiScreen::SingleMission, UiScreen::Briefing) == UiAction::SingleAccept);
	CHECK(uiRoute(UiScreen::SingleMission, UiScreen::MainMenu) == UiAction::SingleCancel);
	CHECK(uiRoute(UiScreen::PilotReady, UiScreen::Mission) == UiAction::Next);
	CHECK(uiRoute(UiScreen::MissionSelection, UiScreen::Briefing) == UiAction::Next);
	CHECK(uiRoute(UiScreen::Briefing, UiScreen::MechBay) == UiAction::Next);
	CHECK(uiRoute(UiScreen::MechBay, UiScreen::PilotReady) == UiAction::Next);
	CHECK(uiRoute(UiScreen::PilotReady, UiScreen::MechBay) == UiAction::Previous);
	CHECK(uiRoute(UiScreen::MechBay, UiScreen::Briefing) == UiAction::Previous);
	CHECK(uiRoute(UiScreen::Briefing, UiScreen::MissionSelection) == UiAction::Previous);
	CHECK(uiRoute(UiScreen::MechBay, UiScreen::MechLab) == UiAction::ChangeLoadout);
	CHECK(uiRoute(UiScreen::MechLab, UiScreen::MechBay) == UiAction::Previous);
	CHECK(uiRoute(UiScreen::MechBay, UiScreen::MechPurchase) == UiAction::BuyMechs);
	// accept, not "Back": Back sells back everything bought on the screen
	CHECK(uiRoute(UiScreen::MechPurchase, UiScreen::MechBay) == UiAction::Next);

	// no button for these
	CHECK(uiRoute(UiScreen::MainMenu, UiScreen::MechBay) == UiAction::None);
	CHECK(uiRoute(UiScreen::MissionSelection, UiScreen::MechBay) == UiAction::None);
	CHECK(uiRoute(UiScreen::OptionsAudio, UiScreen::OptionsAudio) == UiAction::None);
	CHECK(uiRoute(UiScreen::MainMenu, UiScreen::MainMenu) == UiAction::None);
	CHECK(uiRoute(UiScreen::SingleMission, UiScreen::MissionSelection) == UiAction::None);
	// a mission leads only to the results, and they only back to the campaign
	for (int i = 0; i < (int)UiScreen::Count; ++i) {
		const UiScreen s = (UiScreen)i;
		const bool results = s == UiScreen::Salvage || s == UiScreen::PilotReview
				|| s == UiScreen::Promotion;
		CHECK(uiRoute(UiScreen::Mission, s) == (results ? UiAction::WinMission : UiAction::None));
	}
	CHECK(uiRoute(UiScreen::Salvage, UiScreen::PilotReview) == UiAction::SalvageDone);
	CHECK(uiRoute(UiScreen::Salvage, UiScreen::Promotion) == UiAction::SalvageDone);
	CHECK(uiRoute(UiScreen::Promotion, UiScreen::PilotReview) == UiAction::PromotionConfirm);
	CHECK(uiRoute(UiScreen::Promotion, UiScreen::MissionSelection) == UiAction::None);
	CHECK(uiRoute(UiScreen::Salvage, UiScreen::MissionSelection) == UiAction::SalvageDone);
	CHECK(uiRoute(UiScreen::PilotReview, UiScreen::MissionSelection) == UiAction::PilotReviewDone);
	CHECK(uiRoute(UiScreen::Salvage, UiScreen::Mission) == UiAction::None);
	CHECK(uiRoute(UiScreen::PilotReview, UiScreen::MainMenu) == UiAction::None);
	CHECK(uiRoute(UiScreen::Unknown, UiScreen::MainMenu) == UiAction::None);
}

TEST_CASE("in-screen actions belong to one screen each") {
	CHECK(uiScreenAction(UiScreen::MechPurchase, "buy", 3) == UiAction::BuyMech);
	CHECK(uiScreenAction(UiScreen::MechBay, "add", 3) == UiAction::AddToForce);
	CHECK(uiScreenAction(UiScreen::PilotReady, "pilot", 5) == UiAction::AssignPilot);
	CHECK(uiScreenAction(UiScreen::Promotion, "skill", 5) == UiAction::PickSkill);
	CHECK(uiScreenAction(UiScreen::MechBay, "buy", 3) == UiAction::None);
	CHECK(uiScreenAction(UiScreen::MechPurchase, "bu", 2) == UiAction::None);
	CHECK(uiScreenAction(UiScreen::MainMenu, "add", 3) == UiAction::None);
	CHECK(std::strcmp(uiActionName(UiAction::BuyMech), "buy") == 0);
	CHECK(std::strcmp(uiActionName(UiAction::Next), "") == 0);
}

TEST_CASE("parse reads screen+action steps") {
	Run r;
	REQUIRE(r.walk.parse("mainmenu,singlemission,briefing,mechbay,mechpurchase+buy,mechbay+add",
			r.err, sizeof(r.err)));
	CHECK(r.walk.numSteps() == 6);
	CHECK(r.walk.step(4) == UiScreen::MechPurchase);
	CHECK(r.walk.stepAction(4) == UiAction::BuyMech);
	CHECK(r.walk.step(5) == UiScreen::MechBay);
	CHECK(r.walk.stepAction(5) == UiAction::AddToForce);
	CHECK(r.walk.stepAction(3) == UiAction::None);

	CHECK_FALSE(r.walk.parse("mainmenu,singlemission,briefing,mechbay+buy", r.err, sizeof(r.err)));
	CHECK(std::strstr(r.err, "mechbay has no action 'buy'") != nullptr);
	CHECK_FALSE(r.walk.parse("mainmenu+", r.err, sizeof(r.err)));
	CHECK_FALSE(r.walk.parse("mainmenu,nosuch+buy", r.err, sizeof(r.err)));
	CHECK(std::strstr(r.err, "unknown screen 'nosuch'") != nullptr);

	REQUIRE(r.walk.parse("mainmenu,campaign,mission,salvage,pilotreview,missionselection",
			r.err, sizeof(r.err)));
	CHECK(r.walk.numSteps() == 6);
	CHECK(r.walk.step(3) == UiScreen::Salvage);
	// a promotion opens over pilot review; with no salvage, straight from the mission
	REQUIRE(r.walk.parse("mainmenu,campaign,mission,promotion+skill,pilotreview,missionselection",
			r.err, sizeof(r.err)));
	CHECK(r.walk.step(3) == UiScreen::Promotion);
	CHECK(r.walk.stepAction(3) == UiAction::PickSkill);
	CHECK_FALSE(r.walk.parse("mainmenu,campaign,mission,pilotreview+skill", r.err, sizeof(r.err)));
	CHECK_FALSE(r.walk.parse("mainmenu,campaign,mission,missionselection", r.err, sizeof(r.err)));
	CHECK(std::strstr(r.err, "no route from mission to missionselection") != nullptr);
}

TEST_CASE("an in-screen action is pressed after the hold, then held again") {
	Run r;
	REQUIRE(r.walk.parse("mainmenu,singlemission,briefing,mechbay+add,pilotready",
			r.err, sizeof(r.err)));
	r.walk.holdFrames = 2;
	// skip ahead to the mech bay
	const UiScreen before[] = { UiScreen::MainMenu, UiScreen::SingleMission, UiScreen::Briefing };
	for (UiScreen s : before) {
		CHECK(r.walk.update(s, 0.1f).event == UiSmokeWalk::Event::Shown);
		CHECK(r.walk.update(s, 0.1f).event == UiSmokeWalk::Event::Waiting);
		CHECK(r.walk.update(s, 0.1f).event == UiSmokeWalk::Event::Act);
	}
	CHECK(r.walk.update(UiScreen::MechBay, 0.1f).event == UiSmokeWalk::Event::Shown);
	CHECK(r.walk.update(UiScreen::MechBay, 0.1f).event == UiSmokeWalk::Event::Waiting);
	UiSmokeWalk::Result res = r.walk.update(UiScreen::MechBay, 0.1f);
	CHECK(res.event == UiSmokeWalk::Event::Act);
	CHECK(res.action == UiAction::AddToForce);
	CHECK(res.screen == UiScreen::MechBay);
	// held again, and must stay up
	CHECK(r.walk.update(UiScreen::MechBay, 0.1f).event == UiSmokeWalk::Event::Waiting);
	res = r.walk.update(UiScreen::MechBay, 0.1f);
	CHECK(res.event == UiSmokeWalk::Event::Act);
	CHECK(res.action == UiAction::Next);
	CHECK(res.screen == UiScreen::PilotReady);
}

TEST_CASE("parse accepts a valid walk") {
	Run r;
	REQUIRE(r.walk.parse("mainmenu,options-graphics,options-audio,mainmenu", r.err, sizeof(r.err)));
	CHECK(r.walk.numSteps() == 4);
	CHECK(r.walk.step(0) == UiScreen::MainMenu);
	CHECK(r.walk.step(1) == UiScreen::OptionsGraphics);
	CHECK(r.walk.step(2) == UiScreen::OptionsAudio);
	CHECK(r.walk.step(3) == UiScreen::MainMenu);
}

TEST_CASE("parse rejects bad walks with a reason") {
	Run r;
	CHECK_FALSE(r.walk.parse("", r.err, sizeof(r.err)));
	CHECK(std::strstr(r.err, "empty") != nullptr);

	CHECK_FALSE(r.walk.parse(nullptr, r.err, sizeof(r.err)));

	CHECK_FALSE(r.walk.parse("mechbay", r.err, sizeof(r.err)));
	CHECK(std::strstr(r.err, "mainmenu") != nullptr);

	CHECK_FALSE(r.walk.parse("mainmenu,nosuchscreen", r.err, sizeof(r.err)));
	CHECK(std::strstr(r.err, "nosuchscreen") != nullptr);

	CHECK_FALSE(r.walk.parse("mainmenu,mechbay", r.err, sizeof(r.err)));
	CHECK(std::strstr(r.err, "no route") != nullptr);

	CHECK_FALSE(r.walk.parse("mainmenu,", r.err, sizeof(r.err)));
	CHECK_FALSE(r.walk.parse("mainmenu,,options-audio", r.err, sizeof(r.err)));

	// longer than MAX_STEPS
	char longWalk[2048] = "mainmenu";
	for (long i = 0; i < UiSmokeWalk::MAX_STEPS; ++i)
		std::strcat(longWalk, (i % 2) ? ",mainmenu" : ",options-graphics");
	CHECK_FALSE(r.walk.parse(longWalk, r.err, sizeof(r.err)));
	CHECK(std::strstr(r.err, "too many") != nullptr);
}

TEST_CASE("update waits, shows, holds, acts and finishes") {
	Run r;
	REQUIRE(r.walk.parse("mainmenu,options-graphics", r.err, sizeof(r.err)));
	r.walk.holdFrames = 3;

	// loading: nothing settled yet
	UiSmokeWalk::Result res = r.walk.update(UiScreen::Unknown, 0.1f);
	CHECK(res.event == UiSmokeWalk::Event::Waiting);

	res = r.walk.update(UiScreen::MainMenu, 0.1f);
	CHECK(res.event == UiSmokeWalk::Event::Shown);
	CHECK(res.screen == UiScreen::MainMenu);

	// held for three frames, then the button that leads to the next screen
	CHECK(r.walk.update(UiScreen::MainMenu, 0.1f).event == UiSmokeWalk::Event::Waiting);
	CHECK(r.walk.update(UiScreen::MainMenu, 0.1f).event == UiSmokeWalk::Event::Waiting);
	res = r.walk.update(UiScreen::MainMenu, 0.1f);
	CHECK(res.event == UiSmokeWalk::Event::Act);
	CHECK(res.action == UiAction::OpenOptions);
	CHECK(res.screen == UiScreen::OptionsGraphics);

	// the old screen, then a transition, then the new one
	CHECK(r.walk.update(UiScreen::MainMenu, 0.1f).event == UiSmokeWalk::Event::Waiting);
	CHECK(r.walk.update(UiScreen::Unknown, 0.1f).event == UiSmokeWalk::Event::Waiting);
	res = r.walk.update(UiScreen::OptionsGraphics, 0.1f);
	CHECK(res.event == UiSmokeWalk::Event::Shown);
	CHECK(res.screen == UiScreen::OptionsGraphics);

	CHECK(r.walk.update(UiScreen::OptionsGraphics, 0.1f).event == UiSmokeWalk::Event::Waiting);
	CHECK(r.walk.update(UiScreen::OptionsGraphics, 0.1f).event == UiSmokeWalk::Event::Waiting);
	CHECK(r.walk.update(UiScreen::OptionsGraphics, 0.1f).event == UiSmokeWalk::Event::Done);
	// and stays done
	CHECK(r.walk.update(UiScreen::OptionsGraphics, 0.1f).event == UiSmokeWalk::Event::Done);
}

TEST_CASE("a hold counts frames, not time") {
	Run r;
	REQUIRE(r.walk.parse("mainmenu", r.err, sizeof(r.err)));
	r.walk.holdFrames = 2;
	r.walk.timeoutSeconds = 1.0f;
	CHECK(r.walk.update(UiScreen::MainMenu, 0.0f).event == UiSmokeWalk::Event::Shown);
	// long frames while holding are not a timeout
	CHECK(r.walk.update(UiScreen::MainMenu, 5.0f).event == UiSmokeWalk::Event::Waiting);
	CHECK(r.walk.update(UiScreen::MainMenu, 5.0f).event == UiSmokeWalk::Event::Done);
}

TEST_CASE("a screen that never settles times out") {
	Run r;
	REQUIRE(r.walk.parse("mainmenu,campaign", r.err, sizeof(r.err)));
	r.walk.holdFrames = 1;
	r.walk.timeoutSeconds = 1.0f;
	CHECK(r.walk.update(UiScreen::MainMenu, 0.5f).event == UiSmokeWalk::Event::Shown);
	CHECK(r.walk.update(UiScreen::MainMenu, 0.5f).event == UiSmokeWalk::Event::Act);
	CHECK(r.walk.update(UiScreen::Unknown, 0.5f).event == UiSmokeWalk::Event::Waiting);
	CHECK(r.walk.update(UiScreen::MainMenu, 0.4f).event == UiSmokeWalk::Event::Waiting);
	UiSmokeWalk::Result res = r.walk.update(UiScreen::Unknown, 0.2f);
	CHECK(res.event == UiSmokeWalk::Event::Timeout);
	CHECK(res.screen == UiScreen::CampaignSelect);
	// and stays finished
	CHECK(r.walk.update(UiScreen::CampaignSelect, 0.1f).event == UiSmokeWalk::Event::Timeout);
}

TEST_CASE("a screen that leaves during its hold fails the walk") {
	Run r;
	REQUIRE(r.walk.parse("mainmenu,options-graphics", r.err, sizeof(r.err)));
	r.walk.holdFrames = 3;
	CHECK(r.walk.update(UiScreen::MainMenu, 0.1f).event == UiSmokeWalk::Event::Shown);
	CHECK(r.walk.update(UiScreen::MainMenu, 0.1f).event == UiSmokeWalk::Event::Waiting);
	// a transition (or anything else) takes over: no button press follows
	UiSmokeWalk::Result res = r.walk.update(UiScreen::Unknown, 0.1f);
	CHECK(res.event == UiSmokeWalk::Event::Lost);
	CHECK(res.screen == UiScreen::MainMenu);
	// and stays failed
	CHECK(r.walk.update(UiScreen::MainMenu, 0.1f).event == UiSmokeWalk::Event::Lost);
}

TEST_CASE("the first screen times out too") {
	Run r;
	REQUIRE(r.walk.parse("mainmenu", r.err, sizeof(r.err)));
	r.walk.timeoutSeconds = 1.0f;
	CHECK(r.walk.update(UiScreen::Unknown, 0.6f).event == UiSmokeWalk::Event::Waiting);
	CHECK(r.walk.update(UiScreen::Unknown, 0.6f).event == UiSmokeWalk::Event::Timeout);
}

TEST_CASE("update before a successful parse is done at once") {
	UiSmokeWalk walk;
	CHECK(walk.update(UiScreen::MainMenu, 0.1f).event == UiSmokeWalk::Event::Done);
}

} // TEST_SUITE
