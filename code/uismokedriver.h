#ifndef UISMOKEDRIVER_H
#define UISMOKEDRIVER_H

#include "uismokewalk.h"

// Walks the front end for a UI smoke run ("-uismoke <walk>", scripts/smoke.sh):
// each frame it works out which screen has settled, prints
// "[SCREEN] <name> shown" when the walk's next screen appears, holds it, then
// presses the button that leads to the screen after (the same message a
// mouse click sends). Ends with "[UISMOKE] done ..." or "[UISMOKE] FAIL: ..."
// and quits the game. Called once per frame from DoGameLogic(), after the
// front end has updated. Does nothing without -uismoke.
//
// A class (not a namespace) so the front-end classes can make it a friend
// and let it read which screen is up without new accessors.
class UiSmokeDriver {
public:
	static void update ();

private:
	static UiScreen currentScreen ();
	static const char* press (UiAction action);
};

#endif // UISMOKEDRIVER_H
