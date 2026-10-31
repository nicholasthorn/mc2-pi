#ifndef UISMOKEWALK_H
#define UISMOKEWALK_H

#include <cstddef>

// The front-end screens a UI smoke walk ("-uismoke <walk>", scripts/smoke.sh)
// can visit. Unknown means "no screen is settled": a transition animation,
// a fade, or a dialog the walk does not know.
enum class UiScreen {
	Unknown = -1,
	MainMenu,
	OptionsGraphics,	// the four tabs of the options screen (OptionsXScreen)
	OptionsAudio,
	OptionsGameplay,
	OptionsHotkeys,
	CampaignSelect,		// "new campaign" dialog (LogisticsSaveDialog::beginCampaign)
	SingleMission,		// "single mission" dialog (MPLoadMap::beginSingleMission)
	MissionSelection,
	Briefing,
	MechBay,
	MechLab,
	MechPurchase,
	PilotReady,
	Mission,			// gameplay has started (the "[MISSION]" marker); no way out
	Count
};

// What the driver does to leave one screen for the next: the button a player
// would press. The driver (code/uismokedriver.cpp) maps these to button ids.
enum class UiAction {
	None,
	OpenOptions,		// main menu "Options"
	OptionsTab0,		// options screen tab buttons
	OptionsTab1,
	OptionsTab2,
	OptionsTab3,
	OptionsCancel,		// options screen "Cancel": reloads prefs, writes nothing
	NewCampaign,		// main menu "New Campaign"
	DialogAccept,		// campaign dialog "OK" (first entry: the campaign)
	DialogCancel,
	SingleMission,		// main menu "Single Mission"
	SingleAccept,		// single mission dialog "OK" (first mission in the list)
	SingleCancel,
	Next,				// logistics screen "Next" / pilot ready "Launch"
	Previous,			// logistics screen "Back" / mech lab "Cancel"
	ChangeLoadout,		// mech bay -> mech lab
	BuyMechs			// mech bay -> mech purchase
};

const char* uiScreenName(UiScreen s);
UiScreen uiScreenFromName(const char* name, size_t len);

// The button that takes the player from one screen straight to the other, or
// UiAction::None if there is no such button.
UiAction uiRoute(UiScreen from, UiScreen to);

// A walk through the front end: the screens to visit, in order. Pure logic
// (no game, GL or SDL) so it can be unit tested; the driver tells it each
// frame which screen is settled, and it says what to do.
class UiSmokeWalk {
public:
	static const long MAX_STEPS = 64;
	static const long DEFAULT_HOLD_FRAMES = 60;
	static constexpr float DEFAULT_TIMEOUT_SECONDS = 60.0f;

	// "mainmenu,options-graphics,mainmenu,...": comma separated screen names.
	// The first must be mainmenu (where the game starts) and each pair of
	// neighbours must have a route. On failure returns false with the reason
	// in err.
	bool parse(const char* text, char* err, size_t errSize);

	long numSteps() const { return count; }
	UiScreen step(long i) const { return steps[i]; }

	enum class Event {
		Waiting,	// nothing to do this frame
		Shown,		// screen() just settled: print its marker
		Act,		// press action()
		Done,		// every screen was shown and held
		Timeout		// screen() did not appear in time
	};

	struct Result {
		Event event;
		UiScreen screen;	// Shown / Timeout: the screen; Act: the one it leads to
		UiAction action;	// Act only
	};

	// Called once per frame with the settled screen (or Unknown) and the
	// frame's game time in seconds. Each screen is held for holdFrames frames
	// once it settles; a screen that does not settle within timeoutSeconds of
	// the action that asked for it ends the walk with Timeout.
	Result update(UiScreen current, float dt);

	long holdFrames = DEFAULT_HOLD_FRAMES;
	float timeoutSeconds = DEFAULT_TIMEOUT_SECONDS;

private:
	UiScreen steps[MAX_STEPS] = {};
	long count = 0;
	long index = 0;			// the step being waited for or held
	bool holding = false;
	long held = 0;			// frames held so far
	float waited = 0.0f;	// seconds waited so far
	enum class State { Running, Done, TimedOut } state = State::Running;
};

#endif // UISMOKEWALK_H
